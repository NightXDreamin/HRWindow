// mainwindow.cpp (重构后)
#include "mainwindow.h"
#include "ui_mainwindow.h"

// 引入所有管理模块的头文件
#include "jobmanager.h"
#include "productmanager.h"
#include "casemanager.h"

MainWindow::MainWindow(const QString &username, const QString &sessionKey, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sessionKey(sessionKey) // 保存会话密钥
{
    ui->setupUi(this);
    setWindowTitle("网站内容管理系统 - 欢迎您, " + username);

    // --- 核心逻辑：创建并装载各个管理面板 ---

    // 1. 创建招聘管理面板的实例，并将密钥传递给它
    JobManager *jobManager = new JobManager(m_sessionKey, this);

    // 2. 创建产品管理面板的实例 (未来您将在这里传递密钥)
    //ProductManager *productManager = new ProductManager(this);

    // 3. 创建案例管理面板的实例 (同上)
    //CaseManager *caseManager = new CaseManager(this);


    // --- 将各个面板添加到主窗口的 QTabWidget 控件中 ---
    // (请确保您在 mainwindow.ui 中已经放置了一个 QTabWidget，并将其 objectName 设为 "tabWidget")
    ui->tabWidget->addTab(jobManager, "招聘管理");
    //ui->tabWidget->addTab(productManager, "产品管理");
    //ui->tabWidget->addTab(caseManager, "案例管理");
}

MainWindow::~MainWindow()
{
    delete ui;
}
