// loginwindow.cpp
#include "loginwindow.h"
#include "ui_loginwindow.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    setWindowTitle("管理员登录");

    m_networkManager = new QNetworkAccessManager(this);
    // 将网络请求完成的信号连接到我们的处理函数上
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LoginWindow::onLoginReply);

    // 将UI中的OK按钮（我们可将其文本改为“登录”）的点击信号连接到accept()槽
    // accept()会关闭对话框并返回QDialog::Accepted
    // connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LoginWindow::on_passwordInputButton_clicked);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

// “输入”按钮被点击时触发
void LoginWindow::on_passwordInputButton_clicked()
{
    const QString password = ui->passwordInputLine->text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "密码不能为空！");
        return;
    }

    // 禁用UI，防止重复点击，并给出提示
    ui->passwordInputLine->setEnabled(false);
    ui->passwordInputButton->setEnabled(false);
    ui->noticeTxt->setText("正在登录，请稍候...");

    // 准备POST请求
    QUrl url("https://tianyuhuanbao.com/api.php"); // <--- 替换成您的API地址
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // 准备要发送的数据
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

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "无法连接服务器: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // 解析返回的JSON数据
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        QMessageBox::critical(this, "响应错误", "服务器返回了无效的数据格式。");
        reply->deleteLater();
        return;
    }

    // 检查登录状态
    QJsonObject obj = doc.object();
    if (obj["status"].toString() == "success") {
        // 登录成功！
        // 保存从服务器获取的会话密钥和用户名
        m_sessionKey = obj["session_key"].toString();
        m_username = obj["username"].toString();

        QMessageBox::information(this, "登录成功", "欢迎回来, " + m_username + "！");

        // 调用 accept() 来关闭登录对话框，并告诉 main.cpp 登录成功了
        accept();
    } else {
        // 登录失败，显示服务器返回的错误信息
        QMessageBox::warning(this, "登录失败", obj["message"].toString());
    }

    reply->deleteLater();
}

// Getter函数实现
QString LoginWindow::sessionKey() const
{
    return m_sessionKey;
}

QString LoginWindow::username() const
{
    return m_username;
}
