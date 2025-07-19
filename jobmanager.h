// jobmanager.h (最终定稿版)
#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QWidget>
#include "datastructures.h" // 包含 struct Job 的定义
#include <QList>

// 向前声明，以减少头文件依赖
class QNetworkAccessManager;
class QNetworkReply;
class QListWidgetItem;

namespace Ui {
class JobManager;
}

class JobManager : public QWidget
{
    Q_OBJECT

public:
    explicit JobManager(const QString &sessionKey, QWidget *parent = nullptr);
    ~JobManager();

public slots:
    /**
     * @brief updateData 是该模块的核心入口。
     * @param jobs 由MainWindow在获取到最新数据后调用，传入完整的职位列表。
     */
    void updateData(const QList<Job> &jobs);

private slots:
    // UI 交互
    void on_addButton_clicked();
    void on_deleteButton_clicked();
    void on_jobListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    // 编辑框内容改动
    void on_titleEdit_textChanged(const QString &text);
    void on_quotaEdit_textChanged(const QString &text);
    void on_startsalaryEdit_textChanged(const QString &text);
    void on_endsalaryEdit_textChanged(const QString &text);
    void on_requirementEdit_textChanged();

    // 保存功能
    void on_saveButton_clicked();
    void onSaveReply(QNetworkReply *reply);

private:
    Ui::JobManager *ui;
    QList<Job>     m_jobs;
    QNetworkAccessManager *m_networkManager;
    const QString  m_sessionKey;

    // 纯 UI 更新函数
    void updateJobListWidget();
    void populateForm(int index);
    void clearForm();
};

#endif // JOBMANAGER_H
