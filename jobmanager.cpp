// jobmanager.cpp
#include "jobmanager.h"
#include "ui_jobmanager.h" // 确保这里是小写的 "ui_jobmanager.h"

// 引入所有需要的Qt类
#include <QMessageBox>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// JobManager 构造函数
JobManager::JobManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JobManager), // 使用大写的 Ui::JobManager
    m_sessionKey(sessionKey) // 保存从主窗口传递过来的会话密钥
{
    ui->setupUi(this);

    // 初始化网络管理器，并连接信号
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &JobManager::onServerReply);

    // 初始状态设置
    ui->groupBox->setEnabled(false);
    ui->deleteButton->setEnabled(false);
    ui->saveButton->setEnabled(false);

    // 程序一加载就从服务器获取职位列表
    loadJobsFromServer();
}

// JobManager 析构函数
JobManager::~JobManager()
{
    delete ui;
}

// --- 网络请求逻辑 ---

void JobManager::loadJobsFromServer()
{
    ui->labelStatus->setText("正在从服务器加载职位列表...");
    QUrl url("http://tianyuhuanbao.com/api.php?action=get_jobs"); // 请替换成您的真实URL
    QNetworkRequest request(url);
    m_networkManager->get(request);
}

// 当“刷新”按钮被点击时
void JobManager::on_refreshButton_clicked()
{
    loadJobsFromServer();
}

// 当“保存”按钮被点击时
void JobManager::on_saveButton_clicked()
{
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
    QString jsonDataString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    // --- 2. 准备POST请求 ---
    QUrl url("http://tianyuhuanbao.com/api.php"); // 请替换成您的真实URL
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // --- 3. 准备POST数据，现在包含了会话密钥 ---
    QUrlQuery postData;
    postData.addQueryItem("action", "save_jobs");
    postData.addQueryItem("key", m_sessionKey); // 使用会话密钥进行安全验证
    postData.addQueryItem("data", jsonDataString);

    // --- 4. 发送数据 ---
    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
    ui->labelStatus->setText("正在保存职位信息到服务器...");
    ui->saveButton->setEnabled(false);
}

// --- 网络响应处理 ---

void JobManager::onServerReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "请求失败: " + reply->errorString());
        ui->labelStatus->setText("网络错误！");
        if(reply->operation() == QNetworkAccessManager::PostOperation) ui->saveButton->setEnabled(true);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    // 根据操作类型分别处理
    QNetworkAccessManager::Operation operation = reply->operation();

    if (operation == QNetworkAccessManager::GetOperation) { // 处理获取数据的响应
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
            ui->labelStatus->setText("职位列表已更新。");
        } else {
            QMessageBox::critical(this, "API错误", "获取职位失败: " + rootObj["message"].toString());
        }
    } else if (operation == QNetworkAccessManager::PostOperation) { // 处理保存数据的响应
        QJsonObject rootObj = doc.object();
        if (rootObj["status"].toString() == "success") {
            QMessageBox::information(this, "成功", "职位信息已成功保存到服务器！");
            ui->labelStatus->setText("保存成功！");
        } else {
            QMessageBox::critical(this, "保存失败", "服务器返回错误: " + rootObj["message"].toString());
        }
        ui->saveButton->setEnabled(true);
    }

    reply->deleteLater();
}


// --- UI更新与交互逻辑 (与之前版本基本相同) ---

void JobManager::updateJobListWidget()
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

void JobManager::populateForm(int index)
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
    connect(ui->titleEdit, &QLineEdit::textChanged, this, &JobManager::on_titleEdit_textChanged);
    connect(ui->quotaEdit, &QLineEdit::textChanged, this, &JobManager::on_quotaEdit_textChanged);
    connect(ui->startsalaryEdit, &QLineEdit::textChanged, this, &JobManager::on_startsalaryEdit_textChanged);
    connect(ui->endsalaryEdit, &QLineEdit::textChanged, this, &JobManager::on_endsalaryEdit_textChanged);
    connect(ui->requirementEdit, &QPlainTextEdit::textChanged, this, &JobManager::on_requirementEdit_textChanged);
}

void JobManager::clearForm()
{
    ui->titleEdit->clear();
    ui->quotaEdit->clear();
    ui->startsalaryEdit->clear();
    ui->endsalaryEdit->clear();
    ui->requirementEdit->clear();
}


void JobManager::on_jobListWidget_currentItemChanged(QListWidgetItem *current,
                                                     QListWidgetItem *previous)
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


// --- 增、删、改的槽函数 ---

void JobManager::on_addButton_clicked()
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

void JobManager::on_deleteButton_clicked()
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

void JobManager::on_titleEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].title = text;
}
void JobManager::on_quotaEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].quota = text;
}
void JobManager::on_startsalaryEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].salaryStart = text;
}
void JobManager::on_endsalaryEdit_textChanged(const QString &text) {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].salaryEnd = text;
}
void JobManager::on_requirementEdit_textChanged() {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) m_jobs[index].requirements = ui->requirementEdit->toPlainText();
}

void JobManager::updateData(const QList<Job> &jobs)
{
    // 用从主窗口接收到的新数据，更新自己的数据列表
    m_jobs = jobs;
    // 调用自己的函数来刷新UI界面
    updateJobListWidget();
}
