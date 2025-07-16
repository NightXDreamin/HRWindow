// jobmanager.h
#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QWidget>
#include <QNetworkAccessManager>
#include "datastructures.h"
#include <QNetworkReply>
#include <QListWidgetItem>

namespace Ui {
class JobManager;
}

class JobManager : public QWidget
{
    Q_OBJECT

public slots:
    // [新功能] 一个公共的槽函数，用于接收从MainWindow传递过来的最新职位数据
    void updateData(const QList<Job> &jobs);

public:
    explicit JobManager(const QString &sessionKey, QWidget *parent = nullptr);
    ~JobManager();

private slots:
    // 所有招聘相关的槽函数都在这里
    void on_refreshButton_clicked();
    void on_saveButton_clicked();
    void on_addButton_clicked();
    void on_deleteButton_clicked();

    void on_jobListWidget_currentItemChanged(QListWidgetItem *current,
                                             QListWidgetItem *previous);

    void on_titleEdit_textChanged(const QString &text);
    void on_quotaEdit_textChanged(const QString &text);
    void on_startsalaryEdit_textChanged(const QString &text);
    void on_endsalaryEdit_textChanged(const QString &text);
    void on_requirementEdit_textChanged();

    void onServerReply(QNetworkReply *reply);

private:
    Ui::JobManager *ui;
    QList<Job> m_jobs;
    QNetworkAccessManager *m_networkManager;
    const QString m_sessionKey; // 每个模块自己保存会话密钥

    // 所有招聘相关的私有函数
    void loadJobsFromServer();
    void populateForm(int index);
    void updateJobListWidget();
    void clearForm();
};

#endif // JOBMANAGER_H
