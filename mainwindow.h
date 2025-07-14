// mainwindow.h (重构后)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 构造函数现在需要接收登录信息
    explicit MainWindow(const QString &username, const QString &sessionKey, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // MainWindow 只需保存会话密钥，以便传递给子模块
    QString m_sessionKey;
};

#endif // MAINWINDOW_H
