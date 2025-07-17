// jobmanager.cpp
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
#include <QLabel>
#include <QTimer>

JobManager::JobManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::JobManager),
    m_networkManager(new QNetworkAccessManager(this)),
    m_sessionKey(sessionKey)
{
    ui->setupUi(this);

    // 只有“保存”操作需要走网络
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this,            &JobManager::onSaveReply);

    // 初始状态：按钮禁用，表单清空
    ui->groupBox->setEnabled(false);
    ui->deleteButton->setEnabled(false);
    ui->saveButton->setEnabled(false);
}

JobManager::~JobManager()
{
    delete ui;
}

// ----------------------------------------------------------------
// | 由 MainWindow 驱动，给新的职位列表数据，然后刷新界面       |
// ----------------------------------------------------------------
void JobManager::updateData(const QList<Job> &jobs)
{
    m_jobs = jobs;
    updateJobListWidget();
}

// ----------------------------------------------------------------
// | “添加” / “删除” 及编辑框改动，只更新本地 m_jobs 和界面         |
// ----------------------------------------------------------------
void JobManager::on_addButton_clicked()
{
    Job j;
    j.title = "新职位 - 请修改";
    j.quota = "若干";
    j.salaryStart = "面议";
    j.salaryEnd.clear();
    j.requirements = "请填写要求1\n请填写要求2";
    m_jobs.append(j);
    updateJobListWidget();
    ui->jobListWidget->setCurrentRow(m_jobs.count() - 1);
}

void JobManager::on_deleteButton_clicked()
{
    int row = ui->jobListWidget->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选择一个职位再删除。");
        return;
    }
    m_jobs.removeAt(row);
    updateJobListWidget();
}

void JobManager::on_jobListWidget_currentItemChanged(QListWidgetItem *current,
                                                     QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    int idx = ui->jobListWidget->row(current);
    if (idx < 0) {
        ui->groupBox->setEnabled(false);
    } else {
        ui->groupBox->setEnabled(true);
        populateForm(idx);
    }
}

void JobManager::on_titleEdit_textChanged(const QString &text)
{
    int idx = ui->jobListWidget->currentRow();
    if (idx >= 0) m_jobs[idx].title = text;
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

// ----------------------------------------------------------------
// | “保存” 按钮：打包 m_jobs 为 JSON，POST 到服务器             |
// ----------------------------------------------------------------
void JobManager::on_saveButton_clicked()
{
    // 构建 JSON 数组
    QJsonArray arr;
    for (const auto &j: m_jobs) {
        QJsonObject o;
        o["title"]        = j.title;
        o["quota"]        = j.quota;
        o["salary"]       = j.salaryEnd.isEmpty()
                          ? j.salaryStart
                          : j.salaryStart + " - " + j.salaryEnd;
        o["requirements"] = j.requirements;
        arr.append(o);
    }
    QJsonDocument doc(arr);
    QByteArray   body = doc.toJson(QJsonDocument::Compact);

    QUrl url("https://your.domain/api.php");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/json"); // 建议用 JSON

    // 在 HTTP body 或 header 中带上会话密钥
    req.setRawHeader("X-Session-Key", m_sessionKey.toUtf8());
    m_networkManager->post(req, body);

    ui->labelStatus->setText("正在保存职位…");
    ui->saveButton->setEnabled(false);
}

// ----------------------------------------------------------------
// | 只处理“保存”操作的响应，GET 操作交给 MainWindow           |
// ----------------------------------------------------------------
void JobManager::onSaveReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", reply->errorString());
    } else {
        auto bytes = reply->readAll();
        auto doc   = QJsonDocument::fromJson(bytes);
        auto obj   = doc.object();
        if (obj["status"].toString() == "success") {
            QMessageBox::information(this, "保存成功", "职位已保存到服务器。");
        } else {
            QMessageBox::critical(this, "保存失败", obj["message"].toString());
        }
    }
    ui->saveButton->setEnabled(true);
    ui->labelStatus->clear();
    reply->deleteLater();
}

// ----------------------------------------------------------------
// | UI 更新部分，从 m_jobs 同步到界面                          |
// ----------------------------------------------------------------
void JobManager::updateJobListWidget()
{
    ui->jobListWidget->blockSignals(true);
    ui->jobListWidget->clear();
    for (auto &j: m_jobs)
        ui->jobListWidget->addItem(j.title);
    ui->jobListWidget->blockSignals(false);

    bool ok = !m_jobs.isEmpty();
    ui->groupBox->setEnabled(ok);
    ui->deleteButton->setEnabled(ok);
    ui->saveButton->setEnabled(ok);

    if (ok)
        ui->jobListWidget->setCurrentRow(0);
    else
        clearForm();
}

void JobManager::populateForm(int index)
{
    if (index < 0 || index >= m_jobs.count()) return;

    // 先断开旧连接，防止编辑时立刻写回 m_jobs
    auto disconnectAll = [this](){
        disconnect(ui->titleEdit,       nullptr, this, nullptr);
        disconnect(ui->quotaEdit,       nullptr, this, nullptr);
        disconnect(ui->startsalaryEdit, nullptr, this, nullptr);
        disconnect(ui->endsalaryEdit,   nullptr, this, nullptr);
        disconnect(ui->requirementEdit, nullptr, this, nullptr);
    };
    disconnectAll();

    const Job &j = m_jobs[index];
    ui->titleEdit->setText(j.title);
    ui->quotaEdit->setText(j.quota);
    ui->startsalaryEdit->setText(j.salaryStart);
    ui->endsalaryEdit->setText(j.salaryEnd);
    ui->requirementEdit->setPlainText(j.requirements);

    // 重新连接
    connect(ui->titleEdit,       &QLineEdit::textChanged,        this, &JobManager::on_titleEdit_textChanged);
    connect(ui->quotaEdit,       &QLineEdit::textChanged,        this, &JobManager::on_quotaEdit_textChanged);
    connect(ui->startsalaryEdit, &QLineEdit::textChanged,        this, &JobManager::on_startsalaryEdit_textChanged);
    connect(ui->endsalaryEdit,   &QLineEdit::textChanged,        this, &JobManager::on_endsalaryEdit_textChanged);
    connect(ui->requirementEdit, &QPlainTextEdit::textChanged,   this, &JobManager::on_requirementEdit_textChanged);
}

void JobManager::clearForm()
{
    ui->titleEdit->clear();
    ui->quotaEdit->clear();
    ui->startsalaryEdit->clear();
    ui->endsalaryEdit->clear();
    ui->requirementEdit->clear();
}
