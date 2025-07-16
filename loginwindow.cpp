// loginwindow.cpp
#include "loginwindow.h"
#include "ui_loginwindow.h"

// 引入所有需要的Qt类
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QInputDialog>

LoginWindow::LoginWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    setWindowTitle("管理员登录");

    // 1. 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);

    // 2. 连接网络请求完成的信号到我们的处理槽函数上
    //    一旦网络请求有任何结果返回，onLoginReply函数就会被自动调用
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LoginWindow::onLoginReply);

    // 我们可以直接在UI设计器里将按钮的clicked()信号连接到on_passwordInputButton_clicked()槽
    // 如果没有，也可以在这里手动连接
    // connect(ui->passwordInputButton, &QPushButton::clicked, this, &LoginWindow::on_passwordInputButton_clicked);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

// 当“登录”按钮被点击时，此函数被触发
void LoginWindow::on_passwordInputButton_clicked()
{
    const QString password = ui->passwordInputLine->text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "密码不能为空！");
        return;
    }

    // 禁用UI，防止用户在等待时重复点击，并给出提示
    ui->passwordInputLine->setEnabled(false);
    ui->passwordInputButton->setEnabled(false);
    ui->noticeTxt->setText("正在登录，请稍候...");

    // --- 准备网络请求 ---
    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);

    // 设置请求头，表明我们发送的是表单数据
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // 准备要通过POST方法发送的数据
    QUrlQuery postData;
    postData.addQueryItem("action", "login");
    postData.addQueryItem("password", password);

    // 发送POST请求
    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
}

// 当服务器返回响应时，此函数被自动调用
void LoginWindow::onLoginReply(QNetworkReply *reply)
{

    QByteArray raw = reply->readAll();
    qDebug() << "[LoginReply] raw =" << raw;
    // — 然后再做 JSON 解析 —
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, "JSON 解析错误",
                              err.errorString() + "\n\n原始数据:\n" + QString::fromUtf8(raw));
        reply->deleteLater();
        return;
    }

    // 首先，恢复UI，让用户可以重新输入
    ui->passwordInputLine->setEnabled(true);
    ui->passwordInputButton->setEnabled(true);
    ui->noticeTxt->setText("请输入密码");

    // 检查是否有网络层面的错误 (比如服务器没开，网址写错等)
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "无法连接服务器: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // 读取服务器返回的JSON数据


    // 解析JSON，检查登录状态
    QJsonObject obj = doc.object();
    if (obj["status"].toString() == "success") {
        // --- 登录成功！---
        // 1. 保存从服务器获取的会话密钥和用户名
        m_sessionKey = obj["session_key"].toString();
        m_username = obj["username"].toString();

        // 2. 给用户一个积极的反馈
        QMessageBox::information(this, "登录成功", "欢迎回来, " + m_username + "！");

        // 3. 调用 accept() 来关闭登录对话框，并告诉 main.cpp 登录成功了
        accept();
    } else {
        // 登录失败，显示服务器返回的错误信息
        QMessageBox::warning(this, "登录失败", obj["message"].toString());

        QMessageBox::information(this, "服务器消息", obj["message"].toString());
    }

    reply->deleteLater(); // 务必在最后释放reply对象，防止内存泄漏
}

// 这两个函数让 main.cpp 可以在登录成功后获取到密钥和用户名
QString LoginWindow::sessionKey() const
{
    return m_sessionKey;
}

QString LoginWindow::username() const
{
    return m_username;
}

void LoginWindow::on_forceLogoutButton_clicked()
{
    // 弹出一个输入框，要求用户输入管理员密码
    bool ok;
    QString password = QInputDialog::getText(this, "强制清除确认",
                                             "这是一个紧急操作，会强制踢出当前在线的用户。\n请输入您的管理员密码以确认：",
                                             QLineEdit::Password,
                                             QString(), &ok);

    if (ok && !password.isEmpty()) {
        // 用户输入了密码并点击了OK
        ui->noticeTxt->setText("正在发送强制清除请求...");

        QUrl url("https://tianyuhuanbao.com/api.php"); // 您的API地址
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QUrlQuery postData;
        postData.addQueryItem("action", "force_clear_lock");
        postData.addQueryItem("password", password); // 发送用户输入的密码

        m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
    }
}
