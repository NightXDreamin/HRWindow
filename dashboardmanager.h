// dashboardmanager.h
#ifndef DASHBOARDMANAGER_H
#define DASHBOARDMANAGER_H

#include <QWidget>
#include "datastructures.h" // 引入我们定义的数据结构

namespace Ui {
class DashboardManager;
}

class DashboardManager : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardManager(QWidget *parent = nullptr);
    ~DashboardManager();

public slots:
    // 一个公共槽函数，用于接收从MainWindow传递过来的统计数据
    void updateStats(const DashboardStats &stats);

private slots:
    // 响应“刷新”按钮的点击事件
    void on_refreshButton_clicked();

signals:
    // 定义一个信号，用于在用户点击刷新时，通知MainWindow去服务器获取新数据
    void requestRefreshAllData();

private:
    Ui::DashboardManager *ui;
};

#endif // DASHBOARDMANAGER_H
