// mainwindow.cpp [最终功能版]
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "jobmanager.h"
#include "productmanager.h"
#include "casemanager.h"
#include "dashboardmanager.h"
#include "datastructures.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QCloseEvent>

MainWindow::MainWindow(const QString &username, const QString &sessionKey, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sessionKey(sessionKey)
{
    ui->setupUi(this);
    setWindowTitle("网站内容管理系统 v3.0 - 欢迎您, " + username);

    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onServerReply);

    m_dashboardManager = new DashboardManager(this);
    m_jobManager = new JobManager(m_sessionKey, this);
    m_productManager = new ProductManager(m_sessionKey, this);
    m_caseManager = new CaseManager(m_sessionKey, this);

    ui->tabWidget->insertTab(0, m_dashboardManager, "数据中心");
    ui->tabWidget->addTab(m_jobManager, "招聘管理");
    ui->tabWidget->addTab(m_productManager, "产品管理");
    ui->tabWidget->addTab(m_caseManager, "案例管理");

    connect(m_dashboardManager, &DashboardManager::requestRefreshAllData, this, &MainWindow::refreshAllData);

    refreshAllData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- 实现获取数据的网络请求 ---
void MainWindow::refreshAllData()
{
    ui->statusbar->showMessage("正在从服务器同步所有数据...");
    // !!! 请将这里的URL替换成您自己真实的api.php地址 !!!
    QUrl url("https://tianyuhuanbao.com/api.php?action=get_all_data");
    QNetworkRequest request(url);
    m_networkManager->get(request);
}

// --- 实现完整的网络响应处理与数据分发 ---
void MainWindow::onServerReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "请求失败: " + reply->errorString());
        ui->statusbar->showMessage("网络错误！");
        reply->deleteLater();
        return;
    }

    QNetworkAccessManager::Operation operation = reply->operation();

    if (operation == QNetworkAccessManager::GetOperation) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();
            if (rootObj["status"].toString() == "success") {
                QJsonObject data = rootObj["data"].toObject();

                // 1. 解析职位数据并分发给 JobManager
                QList<Job> jobs;
                QJsonArray jobsArray = data["jobs"].toArray();
                for (const QJsonValue &value : jobsArray) {
                    QJsonObject jobObj = value.toObject();
                    Job currentJob;
                    currentJob.title = jobObj["title"].toString();
                    currentJob.quota = jobObj["quota"].toString();
                    currentJob.requirements = jobObj["requirements"].toString();
                    const QString salaryText = jobObj["salary"].toString();
                    const QStringList parts = salaryText.split('-', Qt::SkipEmptyParts);
                    if (parts.size() == 2) {
                        currentJob.salaryStart = parts.at(0).trimmed();
                        currentJob.salaryEnd   = parts.at(1).trimmed();
                    } else {
                        currentJob.salaryStart = salaryText;
                        currentJob.salaryEnd.clear();
                    }
                    jobs.append(currentJob);
                }
                m_jobManager->updateData(jobs);

                // 2. 解析统计数据并分发给 DashboardManager
                DashboardStats stats;
                QJsonObject statsObj = data["stats"].toObject();
                stats.totalJobsCount = statsObj["total_jobs_count"].toInt();
                stats.totalProductsCount = statsObj["total_products_count"].toInt();
                stats.totalCasesCount = statsObj["total_cases_count"].toInt();
                stats.totalRecruitmentQuota = statsObj["total_recruitment_quota"].toInt();
                stats.serverTime = statsObj["server_time"].toString();
                m_dashboardManager->updateStats(stats);

                ui->statusbar->showMessage("所有数据已同步！", 3000);
            } else {
                QMessageBox::critical(this, "API错误", "获取数据失败: " + rootObj["message"].toString());
            }
        }
    }

    reply->deleteLater();
}


// --- 实现带登出请求的窗口关闭事件 ---
void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "退出程序",
                                                               "您确定要退出吗？\n退出后您在服务器上的会话将被注销。",
                                                               QMessageBox::Cancel | QMessageBox::Yes,
                                                               QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore(); // 用户不想退出，忽略关闭事件
    } else {
        // 用户确认退出，向服务器发送logout请求
        QUrl url("https://tianyuhuanbao.com/api.php");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QUrlQuery postData;
        postData.addQueryItem("action", "logout");
        postData.addQueryItem("key", m_sessionKey); // 带上会话密钥

        m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());

        event->accept(); // 允许窗口关闭
    }
}
