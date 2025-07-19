// jobmanager.cpp (最终完整功能版)
#include "jobmanager.h"
#include "ui_jobmanager.h"

#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>

JobManager::JobManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JobManager),
    m_networkManager(new QNetworkAccessManager(this)),
    m_sessionKey(sessionKey)
{
    ui->setupUi(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &JobManager::onSaveReply);
    ui->groupBox->setEnabled(false);
    ui->deleteButton->setEnabled(false);
    ui->saveButton->setEnabled(false);
}

JobManager::~JobManager()
{
    delete ui;
}

void JobManager::updateData(const QList<Job> &jobs)
{
    m_jobs = jobs;
    updateJobListWidget();
}

void JobManager::on_saveButton_clicked()
{
    QJsonArray jobsArray;
    for (const auto &job : m_jobs) {
        QJsonObject jobObj;
        jobObj["title"] = job.title;
        jobObj["quota"] = job.quota;
        jobObj["salary"] = job.salaryEnd.isEmpty() ? job.salaryStart : job.salaryStart.trimmed() + " - " + job.salaryEnd.trimmed();
        jobObj["requirements"] = job.requirements;
        jobsArray.append(jobObj);
    }
    QJsonDocument doc(jobsArray);
    QString jsonDataString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery postData;
    postData.addQueryItem("action", "save_jobs");
    postData.addQueryItem("key", m_sessionKey);
    postData.addQueryItem("data", jsonDataString);

    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());

    ui->labelStatus->setText("正在保存职位信息...");
    ui->saveButton->setEnabled(false);
}

void JobManager::onSaveReply(QNetworkReply *reply)
{
    ui->saveButton->setEnabled(true);
    ui->labelStatus->clear();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "保存请求失败: " + reply->errorString());
    } else {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        auto obj = doc.object();
        if (obj["status"].toString() == "success") {
            QMessageBox::information(this, "保存成功", "职位信息已成功更新到服务器。");
        } else {
            QMessageBox::critical(this, "保存失败", "服务器返回错误: " + obj["message"].toString());
        }
    }
    reply->deleteLater();
}

// --- [核心修正] 以下是完整的UI交互逻辑实现 ---

void JobManager::updateJobListWidget()
{
    ui->jobListWidget->blockSignals(true);
    ui->jobListWidget->clear();
    for (const auto &j: m_jobs)
        ui->jobListWidget->addItem(j.title);
    ui->jobListWidget->blockSignals(false);

    bool hasJobs = !m_jobs.isEmpty();
    ui->deleteButton->setEnabled(hasJobs);
    ui->saveButton->setEnabled(true);

    if (hasJobs) {
        ui->jobListWidget->setCurrentRow(0); // 默认选中第一项
    } else {
        clearForm();
        ui->groupBox->setEnabled(false);
    }
}

void JobManager::on_jobListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    int idx = ui->jobListWidget->row(current);
    ui->groupBox->setEnabled(idx >= 0);
    if (idx >= 0) {
        populateForm(idx);
    } else {
        clearForm();
    }
}

void JobManager::populateForm(int index)
{
    if (index < 0 || index >= m_jobs.count()) return;

    // 断开信号，防止在用代码填充表单时，触发textChanged信号，造成不必要的数据更新
    QObject::disconnect(ui->titleEdit, &QLineEdit::textChanged, this, &JobManager::on_titleEdit_textChanged);
    QObject::disconnect(ui->quotaEdit, &QLineEdit::textChanged, this, &JobManager::on_quotaEdit_textChanged);
    QObject::disconnect(ui->startsalaryEdit, &QLineEdit::textChanged, this, &JobManager::on_startsalaryEdit_textChanged);
    QObject::disconnect(ui->endsalaryEdit, &QLineEdit::textChanged, this, &JobManager::on_endsalaryEdit_textChanged);
    QObject::disconnect(ui->requirementEdit, &QPlainTextEdit::textChanged, this, &JobManager::on_requirementEdit_textChanged);

    // 从 m_jobs 列表中取出对应的数据
    const Job &j = m_jobs[index];

    // 将数据设置到UI控件上
    ui->titleEdit->setText(j.title);
    ui->quotaEdit->setText(j.quota);
    ui->startsalaryEdit->setText(j.salaryStart);
    ui->endsalaryEdit->setText(j.salaryEnd);
    ui->requirementEdit->setPlainText(j.requirements);

    // 填充完毕后，重新连接信号，以便响应用户的编辑操作
    connect(ui->titleEdit,       &QLineEdit::textChanged,      this, &JobManager::on_titleEdit_textChanged);
    connect(ui->quotaEdit,       &QLineEdit::textChanged,      this, &JobManager::on_quotaEdit_textChanged);
    connect(ui->startsalaryEdit, &QLineEdit::textChanged,      this, &JobManager::on_startsalaryEdit_textChanged);
    connect(ui->endsalaryEdit,   &QLineEdit::textChanged,      this, &JobManager::on_endsalaryEdit_textChanged);
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

void JobManager::on_addButton_clicked()
{
    Job j;
    j.title = "新职位 - 请修改";
    j.quota = "若干";
    j.salaryStart = "面议";
    m_jobs.append(j);
    updateJobListWidget();
    ui->jobListWidget->setCurrentRow(m_jobs.count() - 1);
}

void JobManager::on_deleteButton_clicked()
{
    int row = ui->jobListWidget->currentRow();
    if (row < 0) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除", "您确定要删除职位 “" + m_jobs[row].title + "” 吗？\n此操作将立即影响服务器数据！",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_jobs.removeAt(row);
        updateJobListWidget();
    }
}

void JobManager::on_titleEdit_textChanged(const QString &text)
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) {
        m_jobs[idx].title = text;
        ui->jobListWidget->item(idx)->setText(text);
    }
}

void JobManager::on_quotaEdit_textChanged(const QString &text)
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) m_jobs[idx].quota = text;
}

void JobManager::on_startsalaryEdit_textChanged(const QString &text)
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) m_jobs[idx].salaryStart = text;
}

void JobManager::on_endsalaryEdit_textChanged(const QString &text)
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) m_jobs[idx].salaryEnd = text;
}

void JobManager::on_requirementEdit_textChanged()
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) m_jobs[idx].requirements = ui->requirementEdit->toPlainText();
}
