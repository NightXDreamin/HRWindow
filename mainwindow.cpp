// mainwindow.cpp (v2.0 最终网络版)
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("PHP 招聘信息编辑器 v2.0");
    ui->groupBox->setEnabled(false);
    ui->saveButton->setEnabled(false);
    ui->addButton->setEnabled(true);  // 允许一开始就添加职位
    ui->deleteButton->setEnabled(false);
    ui->fileGetButton->setText("刷新列表"); // 将按钮文本改为“刷新”

    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onServerReply);

    loadJobsFromServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// “刷新”按钮的逻辑
void MainWindow::on_fileGetButton_clicked()
{
    loadJobsFromServer();
}

// 保存按钮的逻辑：将数据发送到服务器
void MainWindow::on_saveButton_clicked()
{
    // 简单的输入验证
    for (const auto& job : m_jobs) {
        if (job.title.trimmed().isEmpty()) {
            QMessageBox::critical(this, "保存失败", "职位标题不能为空！");
            return;
        }
    }

    // --- 1. 将 m_jobs 数据转换为 JSON ---
    QJsonArray jobsArray;
    for (const auto& job : m_jobs) {
        QJsonObject jobObj;
        jobObj["title"] = job.title;
        jobObj["quota"] = job.quota;
        jobObj["salary"] = job.salaryEnd.isEmpty() ? job.salaryStart : job.salaryStart + " - " + job.salaryEnd;
        jobObj["requirements"] = job.requirements;
        jobsArray.append(jobObj);
    }
    QJsonDocument doc(jobsArray);
    // 使用 Compact 参数生成不带换行的紧凑JSON字符串
    QString jsonDataString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // --- 2. 准备POST请求 ---
    // 请将这里的URL替换成您自己的API地址
    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);
    // 设置请求头，表明我们发送的是表单数据
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // --- 3. 准备要发送的POST数据 ---
    QUrlQuery postData;
    postData.addQueryItem("action", "save_jobs");
    postData.addQueryItem("key", m_apiKey); // 使用我们定义好的密钥
    postData.addQueryItem("data", jsonDataString);

    // --- 4. 发送数据并更新UI ---
    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
    ui->statusbar->showMessage("正在保存到服务器...");
    ui->saveButton->setEnabled(false); // 防止重复点击
}


void MainWindow::loadJobsFromServer()
{
    ui->statusbar->showMessage("正在从服务器加载数据...");
    // 请将这里的URL替换成您自己的API地址
    QUrl url("https://tianyuhuanbao.com/api.php?action=get_jobs");
    QNetworkRequest request(url);
    m_networkManager->get(request);
}

// 唯一的网络响应处理函数，能区分是“获取”的响应还是“保存”的响应
void MainWindow::onServerReply(QNetworkReply *reply)
{
    // 首先，检查网络请求自身是否出错
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "无法连接到服务器或API出错: " + reply->errorString());
        ui->statusbar->showMessage("服务器连接失败！");
        // --- 修正：检查 reply->operation() ---
        if(reply->operation() == QNetworkAccessManager::PostOperation) {
            ui->saveButton->setEnabled(true); // 如果是保存操作失败，恢复按钮
        }
        reply->deleteLater();
        return;
    }

    // 读取服务器返回的所有原始数据
    QByteArray responseData = reply->readAll();

    // --- 修正：使用 reply->operation() 来判断请求类型 ---
    QNetworkAccessManager::Operation operation = reply->operation();

    // --- A. 如果是“获取职位”(GET)操作的响应 ---
    if (operation == QNetworkAccessManager::GetOperation) {
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            m_jobs.clear();
            QJsonObject rootObj = doc.object();
            if (rootObj["status"].toString() == "success") {
                QJsonArray jobsArray = rootObj["data"].toArray();
                for (const QJsonValue &value : jobsArray) {
                    QJsonObject jobObj = value.toObject();
                    Job currentJob;
                    currentJob.title = jobObj["title"].toString();
                    currentJob.quota = jobObj["quota"].toString();
                    currentJob.requirements = jobObj["requirements"].toString();
                    const QString salaryText = jobObj["salary"].toString();
                    const QStringList parts = salaryText.split('-', Qt::SkipEmptyParts);
                    if (parts.size() == 2) {
                        currentJob.salaryStart = parts.at(0).trimmed();
                        currentJob.salaryEnd   = parts.at(1).trimmed();
                    } else {
                        currentJob.salaryStart = salaryText;
                        currentJob.salaryEnd.clear();
                    }
                    m_jobs.append(currentJob);
                }
                updateJobListWidget();
                ui->statusbar->showMessage("数据已从服务器加载。", 3000);
            } else {
                QString errorMessage = rootObj["message"].toString();
                QMessageBox::critical(this, "API错误", "获取数据失败: " + errorMessage);
                ui->statusbar->showMessage("API返回错误！");
            }
        } else { // 如果返回的不是一个JSON对象
            QMessageBox::critical(this, "数据错误", "从服务器返回的数据格式无效 (非JSON Object)。");
        }
    }
    // --- B. 如果是“保存职位”(POST)操作的响应 ---
    else if (operation == QNetworkAccessManager::PostOperation) {
        QString logFilePath = QCoreApplication::applicationDirPath() + "/server_response_log.txt";
        QFile logFile(logFilePath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logFile.write(responseData);
            logFile.close();
            QMessageBox::information(this, "收到响应",
                                     "已收到服务器的“保存”操作响应！\n\n"
                                     "详细的调试日志已保存到程序目录下的\n"
                                     "server_response_log.txt 文件中，请查看。");
        } else {
            QMessageBox::warning(this, "日志写入失败",
                                 "收到服务器响应，但无法将调试日志写入本地文件。\n"
                                 "尝试写入路径: " + logFilePath);
        }

        ui->saveButton->setEnabled(true);
        ui->statusbar->clearMessage();
    }

    reply->deleteLater();
}

void MainWindow::updateJobListWidget()
{
    ui->jobListWidget->blockSignals(true);
    ui->jobListWidget->clear();
    for (const auto& job : m_jobs) {
        ui->jobListWidget->addItem(job.title);
    }
    ui->jobListWidget->blockSignals(false);
    if (!m_jobs.isEmpty()) {
        ui->jobListWidget->setCurrentRow(0);
        ui->deleteButton->setEnabled(true);
        ui->saveButton->setEnabled(true);
    } else {
        clearForm();
        ui->groupBox->setEnabled(false);
        ui->deleteButton->setEnabled(false);
        ui->saveButton->setEnabled(false);
    }
}

void MainWindow::on_jobListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    int index = ui->jobListWidget->row(current);
    if (index < 0) {
        ui->groupBox->setEnabled(false);
        clearForm();
        return;
    }
    ui->groupBox->setEnabled(true);
    populateForm(index);
}

void MainWindow::populateForm(int index)
{
    if (index < 0 || index >= m_jobs.count()) return;
    QObject::disconnect(ui->titleEdit, nullptr, this, nullptr);
    QObject::disconnect(ui->quotaEdit, nullptr, this, nullptr);
    QObject::disconnect(ui->startsalaryEdit, nullptr, this, nullptr);
    QObject::disconnect(ui->endsalaryEdit, nullptr, this, nullptr);
    QObject::disconnect(ui->requirementEdit, nullptr, this, nullptr);
    const auto& job = m_jobs[index];
    ui->titleEdit->setText(job.title);
    ui->quotaEdit->setText(job.quota);
    ui->startsalaryEdit->setText(job.salaryStart);
    ui->endsalaryEdit->setText(job.salaryEnd);
    ui->requirementEdit->setPlainText(job.requirements);
    connect(ui->titleEdit, &QLineEdit::textChanged, this, &MainWindow::on_titleEdit_textChanged);
    connect(ui->quotaEdit, &QLineEdit::textChanged, this, &MainWindow::on_quotaEdit_textChanged);
    connect(ui->startsalaryEdit, &QLineEdit::textChanged, this, &MainWindow::on_startsalaryEdit_textChanged);
    connect(ui->endsalaryEdit, &QLineEdit::textChanged, this, &MainWindow::on_endsalaryEdit_textChanged);
    connect(ui->requirementEdit, &QPlainTextEdit::textChanged, this, &MainWindow::on_requirementEdit_textChanged);
}

void MainWindow::clearForm()
{
    ui->titleEdit->clear();
    ui->quotaEdit->clear();
    ui->startsalaryEdit->clear();
    ui->endsalaryEdit->clear();
    ui->requirementEdit->clear();
}

void MainWindow::on_titleEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].title = text;
}
void MainWindow::on_quotaEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].quota = text;
}
void MainWindow::on_startsalaryEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].salaryStart = text;
}
void MainWindow::on_endsalaryEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].salaryEnd = text;
}
void MainWindow::on_requirementEdit_textChanged() {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].requirements = ui->requirementEdit->toPlainText();
}

void MainWindow::on_addButton_clicked()
{
    Job newJob;
    newJob.title = "新职位 - 请修改";
    newJob.salaryStart = "面议";
    newJob.salaryEnd = "";
    newJob.quota = "若干";
    newJob.requirements = "请填写要求1\n请填写要求2";
    m_jobs.append(newJob);
    updateJobListWidget();
    ui->jobListWidget->setCurrentRow(m_jobs.count() - 1);
}

void MainWindow::on_deleteButton_clicked()
{
    int index = ui->jobListWidget->currentRow();
    if (index == -1) {
        QMessageBox::information(this, "提示", "请先在左侧列表中选择一个要删除的职位。");
        return;
    }
    auto reply = QMessageBox::question(this, "确认删除", "您确定要删除职位 \"" + m_jobs[index].title + "\" 吗？", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_jobs.removeAt(index);
        updateJobListWidget();
    }
}
