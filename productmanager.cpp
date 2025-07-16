// productmanager.cpp
#include "productmanager.h"
#include "ui_productmanager.h" // 引入UI头文件

ProductManager::ProductManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProductManager),
    m_sessionKey(sessionKey)
{
    ui->setupUi(this);
}

ProductManager::~ProductManager()
{
    delete ui;
}
