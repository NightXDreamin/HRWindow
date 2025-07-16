// mainwindow.h (修正版)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// 向前声明，避免引入过多头文件
class QNetworkAccessManager;
class QNetworkReply;
class QCloseEvent;
class JobManager;         // 使用向前声明，而不是包含头文件
class ProductManager;
class CaseManager;
class DashboardManager;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &username, const QString &sessionKey, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onServerReply(QNetworkReply *reply);
    void refreshAllData();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *m_networkManager;
    QString m_sessionKey;

    // 保存对各个管理面板的指针
    JobManager* m_jobManager;
    ProductManager* m_productManager;
    CaseManager* m_caseManager;
    DashboardManager* m_dashboardManager; // 新增Dashboard指针
};

#endif // MAINWINDOW_H
