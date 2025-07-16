// productmanager.h
#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <QWidget>
#include "datastructures.h" // 引入数据结构定义

namespace Ui {
class ProductManager; // 假设UI类名是ProductManager
}

class ProductManager : public QWidget
{
    Q_OBJECT

public:
    // 构造函数现在接收会话密钥
    explicit ProductManager(const QString &sessionKey, QWidget *parent = nullptr);
    ~ProductManager();

private:
    Ui::ProductManager *ui;
    const QString m_sessionKey;
    // ... 未来这里会添加产品列表、网络管理器等 ...
};

#endif // PRODUCTMANAGER_H
