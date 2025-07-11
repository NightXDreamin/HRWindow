// mainwindow.cpp (适配简化版PHP的最终版本)
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDir>
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("PHP 招聘信息编辑器");
    ui->groupBox->setEnabled(false);
    ui->saveButton->setEnabled(false);
    ui->addButton->setEnabled(false);
    ui->deleteButton->setEnabled(false);

    QString appDir = QCoreApplication::applicationDirPath();
    QString autoLoadPath = QDir(appDir).filePath("career.php");

    if (QFile::exists(autoLoadPath)) {
        loadJobsFromFile(autoLoadPath);
    } else {
        ui->filePathLabel->setText("未在程序目录找到 career.php，请手动选择文件。");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_fileGetButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择 career.php 文件", "", "PHP Files (*.php)");
    if (!filePath.isEmpty()) {
        loadJobsFromFile(filePath);
    }
}

void MainWindow::loadJobsFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开文件:\n" + filePath);
        return;
    }

    QByteArray fileBytes = file.readAll();
    file.close();
    m_originalFileContent = QTextCodec::codecForName("UTF-8")->toUnicode(fileBytes);

    m_phpFilePath = filePath;
    ui->filePathLabel->setText(m_phpFilePath);

    parsePhpContent(m_originalFileContent);
    updateJobListWidget();

    ui->saveButton->setEnabled(true);
    ui->addButton->setEnabled(true);
    ui->deleteButton->setEnabled(true);
}


// --- 已重写为更简单、更稳健的版本 ---
void MainWindow::parsePhpContent(const QString& content)
{
    m_jobs.clear();

    QRegularExpression jobsArrayRe("\\$jobs\\s*=\\s*\\[(.*)\\]\\s*;",
                                   QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch arrayMatch = jobsArrayRe.match(content);
    if (!arrayMatch.hasMatch()) {
        QMessageBox::warning(this, "解析失败", "在文件中未找到 $jobs 数组。");
        return;
    }
    QString jobsContent = arrayMatch.captured(1);

    // 因为不再有嵌套括号，这个简单的正则现在可以完美工作了
    QRegularExpression jobBlockRe("\\[(.*?)\\]", QRegularExpression::DotMatchesEverythingOption);
    auto it = jobBlockRe.globalMatch(jobsContent);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString block = match.captured(1);
        Job currentJob;

        currentJob.title = QRegularExpression("\"title\"\\s*=>\\s*\"([^\"]*)\"")
                               .match(block).captured(1).trimmed();

        currentJob.quota = QRegularExpression("\"quota\"\\s*=>\\s*\"([^\"]*)\"")
                               .match(block).captured(1).trimmed();

        const QString salaryText = QRegularExpression("\"salary\"\\s*=>\\s*\"([^\"]*)\"")
                                       .match(block).captured(1).trimmed();
        const QStringList parts = salaryText.split('-', Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            currentJob.salaryStart = parts.at(0).trimmed();
            currentJob.salaryEnd   = parts.at(1).trimmed();
        } else {
            currentJob.salaryStart = salaryText;
            currentJob.salaryEnd.clear();
        }

        // --- 关键修改：直接将 requirements 作为一整个字符串来解析 ---
        currentJob.requirements = QRegularExpression("\"requirements\"\\s*=>\\s*\"(.*?)\"",
                                                     QRegularExpression::DotMatchesEverythingOption)
                                      .match(block).captured(1);

        if (!currentJob.title.isEmpty()) {
            m_jobs.append(currentJob);
        }
    }
}



void MainWindow::updateJobListWidget()
{
    ui->jobListWidget->blockSignals(true);
    ui->jobListWidget->clear();
    for (const auto& job : m_jobs) {
        ui->jobListWidget->addItem(job.title);
    }
    ui->jobListWidget->blockSignals(false);
    if (!m_jobs.isEmpty())
        ui->jobListWidget->setCurrentRow(0);
    else {
        clearForm();
        ui->groupBox->setEnabled(false);
    }
}

void MainWindow::on_jobListWidget_currentItemChanged(QListWidgetItem* current,
                                                     QListWidgetItem* previous)
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

// --- 已修改以适配新的 Job 结构 ---
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
    // 直接设置文本，因为 requirements 本身就是包含换行的字符串
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
    if (index != -1) {
        m_jobs[index].title = text;
        ui->jobListWidget->currentItem()->setText(text);
    }
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
// --- 已修改以适配新的 Job 结构 ---
void MainWindow::on_requirementEdit_textChanged() {
    int index = ui->jobListWidget->currentRow();
    if (index != -1) {
        // 直接获取所有文本
        m_jobs[index].requirements = ui->requirementEdit->toPlainText();
    }
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
    ui->jobListWidget->addItem(newJob.title);
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

// --- 已修改以适配新的 Job 结构 ---
void MainWindow::on_saveButton_clicked()
{
    for (const auto& job : m_jobs) {
        if (job.title.trimmed().isEmpty() || job.quota.trimmed().isEmpty() || job.salaryStart.trimmed().isEmpty()) {
            QMessageBox::critical(this, "保存失败", "存在职位信息不完整（标题、名额、起始薪资不能为空），请检查！");
            return;
        }
    }

    QString newJobsArrayString = "    $jobs = [\n";
    for (int i = 0; i < m_jobs.size(); ++i) {
        const auto& job = m_jobs[i];
        QString salaryString;
        if (!job.salaryEnd.trimmed().isEmpty()) {
            salaryString = job.salaryStart.trimmed() + " - " + job.salaryEnd.trimmed();
        } else {
            salaryString = job.salaryStart.trimmed();
        }

        // --- 关键修改：处理 requirements 字符串中的特殊字符，以便能正确写入PHP文件 ---
        QString escapedReqs = job.requirements;
        escapedReqs.replace('\\', "\\\\"); // 先转义反斜杠
        escapedReqs.replace('"', "\\\""); // 再转义双引号

        newJobsArrayString += "        [\n";
        newJobsArrayString += QString("            \"title\" => \"%1\",\n").arg(job.title);
        newJobsArrayString += QString("            \"quota\" => \"%1\",\n").arg(job.quota);
        newJobsArrayString += QString("            \"salary\" => \"%1\",\n").arg(salaryString);
        // 直接将处理过的多行字符串写入
        newJobsArrayString += QString("            \"requirements\" => \"%1\"\n").arg(escapedReqs);
        newJobsArrayString += "        ]";
        if (i < m_jobs.size() - 1) {
            newJobsArrayString += ",\n";
        }
    }
    newJobsArrayString += "\n    ];";

    QString newFileContent = m_originalFileContent;
    QRegularExpression jobsArrayRegex(R"(\$jobs\s*=\s*\[.*\];)", QRegularExpression::DotMatchesEverythingOption);
    newFileContent.replace(jobsArrayRegex, newJobsArrayString);

    QFile file(m_phpFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(this, "保存失败", "无法写入文件！请检查文件是否被占用或权限不足。");
        return;
    }
    QTextStream out(&file);
    out << newFileContent;
    file.close();

    m_originalFileContent = newFileContent;
    QMessageBox::information(this, "成功", "招聘信息已成功保存到\n" + m_phpFilePath);
}
