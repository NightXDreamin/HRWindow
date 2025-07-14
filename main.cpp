// main.cpp
#include "mainwindow.h"
#include "loginwindow.h" // 您的登录窗口类名可能是 LoginDialog，请按需修改
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LoginWindow loginDialog; // 使用您自己的类名 LoginWindow 或 LoginDialog
    if (loginDialog.exec() == QDialog::Accepted) {
        // 登录成功后，创建一个主窗口实例
        // 关键修改：通过构造函数，将密钥和用户名传递给主窗口
        MainWindow w(loginDialog.username(), loginDialog.sessionKey());
        w.show();
        return a.exec();
    } else {
        // 用户取消登录，程序直接退出
        return 0;
    }
}
