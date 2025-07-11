// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QListWidgetItem>

// 定义职位数据结构，保持不变
struct Job {
    QString title;
    QString quota;
    QString salaryStart;
    QString salaryEnd;
    QString requirements;
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- 槽函数已根据新UI的 objectName 更新 ---
    void on_fileGetButton_clicked(); // 对应 “选择文件” 按钮
    void on_saveButton_clicked();    // 对应 “保存文件” 按钮
    void on_addButton_clicked();     // 对应 “增加职位” 按钮
    void on_deleteButton_clicked();  // 对应 “删除职位” 按钮

    // 当列表中的选中项改变时触发
    void on_jobListWidget_currentItemChanged(QListWidgetItem* current,
                                             QListWidgetItem* previous);

    // 当编辑框内容改变时，自动更新数据
    void on_titleEdit_textChanged(const QString &text);
    void on_quotaEdit_textChanged(const QString &text);
    void on_startsalaryEdit_textChanged(const QString &text); // 对应起始薪资输入框
    void on_endsalaryEdit_textChanged(const QString &text);   // 对应结束薪资输入框
    void on_requirementEdit_textChanged(); // 对应 QPlainTextEdit


private:
    Ui::MainWindow *ui;

    QList<Job> m_jobs;
    QString m_originalFileContent;
    QString m_phpFilePath;

    // 核心函数声明保持不变
    void parsePhpContent(const QString& content);
    void populateForm(int index);
    void updateJobListWidget();
    void clearForm();
    void loadJobsFromFile(const QString& filePath);
};
#endif // MAINWINDOW_H
