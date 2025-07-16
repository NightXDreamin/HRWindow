// dashboardmanager.cpp
#include "dashboardmanager.h"
#include "ui_dashboardmanager.h"

DashboardManager::DashboardManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DashboardManager)
{
    ui->setupUi(this);
    // 将UI中的刷新按钮的 clicked() 信号，连接到我们自己的槽函数上
    // 请确保您在UI设计器中，将刷新按钮的 objectName 设为 refreshButton
    connect(ui->refreshButton, &QPushButton::clicked, this, &DashboardManager::on_refreshButton_clicked);
}

DashboardManager::~DashboardManager()
{
    delete ui;
}

// 实现更新UI的槽函数
void DashboardManager::updateStats(const DashboardStats &stats)
{
    // 将统计数据设置到对应的QLabel上
    // 请确保您在UI中已放置这些QLabel并设置了正确的objectName
    ui->jobsCountLabel->setText(QString::number(stats.totalJobsCount) + " 个");
    ui->productsCountLabel->setText(QString::number(stats.totalProductsCount) + " 个");
    ui->casesCountLabel->setText(QString::number(stats.totalCasesCount) + " 个");
    ui->quotaCountLabel->setText(QString::number(stats.totalRecruitmentQuota) + " 人");
    ui->serverTimeLabel->setText(stats.serverTime);
}

// 实现按钮点击的槽函数
void DashboardManager::on_refreshButton_clicked()
{
    // 当用户点击刷新时，发射一个信号，通知MainWindow该干活了
    emit requestRefreshAllData();
}
