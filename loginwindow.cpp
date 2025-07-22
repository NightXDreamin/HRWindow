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
    // 恢复UI
    ui->passwordInputLine->setEnabled(true);
    ui->passwordInputButton->setEnabled(true);
    ui->noticeTxt->setText("请输入密码");

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "无法连接服务器: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        QMessageBox::critical(this, "响应错误", "服务器返回了无效的数据格式。");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QString status = obj["status"].toString();
    QString message = obj["message"].toString();

    if (status == "success") {
        // --- 核心修正：增加对 session_key 的判断 ---
        if (obj.contains("session_key")) {
            // 如果包含 session_key，这一定是一次成功的“登录”响应
            m_sessionKey = obj["session_key"].toString();
            m_username = obj["username"].toString();
            QMessageBox::information(this, "登录成功", "欢迎回来, " + m_username + "！");
            accept(); // 关闭对话框，通知main.cpp成功
        } else {
            // 如果不包含 session_key，这就是一次成功的“强制清除”或其他操作的响应
            QMessageBox::information(this, "操作成功", message);
        }
    } else {
        // status 是 "error"，说明操作失败
        QMessageBox::warning(this, "操作失败", message);
    }

    reply->deleteLater();
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
