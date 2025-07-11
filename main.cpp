#include "mainwindow.h"  // 包含您的主窗口头文件
#include <QApplication>    // 包含Qt应用程序的核心类

int main(int argc, char *argv[])
{
    // 1. 创建一个Qt应用程序对象，这是每个Qt程序都必须有的
    QApplication a(argc, argv);

    // 2. 创建您的主窗口的一个实例
    MainWindow w;

    // 3. 显示主窗口
    w.show();

    // 4. 启动应用程序的事件循环，并等待程序退出
    //    程序会在这里一直运行，直到用户关闭窗口
    return a.exec();
}
