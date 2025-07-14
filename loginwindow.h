// loginwindow.h
#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    // 公共函数，用于在登录成功后，让主程序获取会话密钥和用户名
    QString sessionKey() const;
    QString username() const;

private slots:
    // 响应“输入”按钮的点击事件 (请确保您的按钮objectName是 passwordInputButton)
    void on_passwordInputButton_clicked();

    // 处理服务器返回的登录结果
    void onLoginReply(QNetworkReply *reply);

private:
    Ui::LoginWindow *ui;
    QNetworkAccessManager *m_networkManager;

    // 用于存储登录成功后从服务器获取的信息
    QString m_sessionKey;
    QString m_username;
};

#endif // LOGINWINDOW_H
