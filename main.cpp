// main.cpp

#include "mainwindow.h"
#include "loginwindow.h" // <-- 包含我们新创建的登录对话框的头文件
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 先创建并显示登录对话框
    LoginDialog loginDialog;
    // 使用 exec() 以模态方式显示对话框，程序会在这里暂停，直到对话框被关闭
    int result = loginDialog.exec();

    // 2. 检查登录对话框的返回结果
    if (result == QDialog::Accepted) {
        // 如果用户点击了“登录”并登录成功 (我们稍后会实现这个逻辑)
        // 那么就创建并显示主窗口
        MainWindow w;
        w.show();
        // 启动主程序的事件循环
        return a.exec();
    } else {
        // 如果用户关闭了对话框或点击了“取消”，则直接退出程序
        return 0;
    }
}
