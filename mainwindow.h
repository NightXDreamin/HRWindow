// mainwindow.h (v2.0 最终网络版)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Job 结构体保持不变
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
    void on_fileGetButton_clicked(); // 该按钮功能将变为“刷新”
    void on_saveButton_clicked();
    void on_addButton_clicked();
    void on_deleteButton_clicked();

    void on_jobListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

    void on_titleEdit_textChanged(const QString &text);
    void on_quotaEdit_textChanged(const QString &text);
    void on_startsalaryEdit_textChanged(const QString &text);
    void on_endsalaryEdit_textChanged(const QString &text);
    void on_requirementEdit_textChanged();

    // 唯一的网络响应处理槽函数
    void onServerReply(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QList<Job> m_jobs;
    QNetworkAccessManager *m_networkManager;

    // --- 新增：在这里存放您的API密钥，方便管理 ---
    const QString m_apiKey = "JingJing8859!";

    // 函数声明
    void populateForm(int index);
    void updateJobListWidget();
    void clearForm();
    void loadJobsFromServer();

    // 不再需要以下成员和函数：
    // QString m_originalFileContent;
    // QString m_phpFilePath;
    // parsePhpContent()
};
#endif // MAINWINDOW_H
