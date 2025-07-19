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

void MainWindow::refreshAllData()
{
    ui->statusbar->showMessage("正在从服务器同步所有数据...");
    QUrl url("https://tianyuhuanbao.com/api.php?action=get_all_data");
    QNetworkRequest request(url);
    m_networkManager->get(request);
}

// --- [核心升级] 增加了详细的调试输出和健壮性检查 ---
void MainWindow::onServerReply(QNetworkReply *reply)
{
    qDebug() << "--- onServerReply triggered ---";

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network Error:" << reply->errorString();
        QMessageBox::critical(this, "网络错误", "请求失败: " + reply->errorString());
        ui->statusbar->showMessage("网络错误！");
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    // 我们不再打印完整的原始数据，因为它太长了
    qDebug() << "Received" << responseData.size() << "bytes from server.";

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "JSON parsing failed: Document is null or not an object.";
        QMessageBox::critical(this, "数据格式错误", "服务器返回的数据不是有效的JSON对象。");
        ui->statusbar->showMessage("数据格式错误！");
        reply->deleteLater();
        return;
    }

    QJsonObject rootObj = doc.object();
    if (rootObj["status"].toString() != "success") {
        QString errorMessage = rootObj["message"].toString("未知错误");
        qDebug() << "API Error:" << errorMessage;
        QMessageBox::critical(this, "API错误", "获取数据失败: " + errorMessage);
        ui->statusbar->showMessage("API返回错误！");
        reply->deleteLater();
        return;
    }

    qDebug() << "API status is success. Starting data parsing...";

    // --- 关键诊断区 ---
    if (!rootObj.contains("data") || !rootObj["data"].isObject()) {
        qDebug() << "CRITICAL ERROR: 'data' field is missing or is not an object!";
        ui->statusbar->showMessage("数据结构错误：缺少 'data' 对象。");
        reply->deleteLater();
        return;
    }
    QJsonObject data = rootObj["data"].toObject();

    // 检查 'jobs' 字段
    if (!data.contains("jobs")) {
        qDebug() << "CRITICAL ERROR: 'jobs' field is missing in 'data' object!";
    } else {
        QJsonValue jobsValue = data.value("jobs");
        qDebug() << "'jobs' field found. Its type is:" << jobsValue.type()
                 << "(Note: Array is type 4)";

        if (jobsValue.isArray()) {
            QList<Job> jobs;
            QJsonArray jobsArray = jobsValue.toArray();
            qDebug() << "Successfully parsed 'jobs' as an array. Size:" << jobsArray.size();

            for (const QJsonValue &value : jobsArray) {
                // ... (解析单个Job的逻辑) ...
                if (value.isObject()){
                    QJsonObject jobObj = value.toObject();
                    Job currentJob;
                    currentJob.title = jobObj["title"].toString();
                    currentJob.title       = jobObj["title"].toString();
                    currentJob.quota       = jobObj["quota"].toString();
                    currentJob.salaryStart = jobObj["salaryStart"].toString();
                    currentJob.salaryEnd   = jobObj["salaryEnd"].toString();
                    currentJob.requirements= jobObj["requirements"].toString();
                    jobs.append(currentJob);
                }
            }
            m_jobManager->updateData(jobs);
            qDebug() << "Jobs data updated with" << jobs.count() << "items.";
        } else {
            qDebug() << "ERROR: 'jobs' field is NOT an array!";
        }
    }

    // ... (解析stats数据的逻辑) ...

    ui->statusbar->showMessage("所有数据已同步！", 3000);
    qDebug() << "--- Sync finished ---";

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
