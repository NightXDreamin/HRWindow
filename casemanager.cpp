// casemanager.cpp
#include "casemanager.h"
#include "ui_casemanager.h" // 引入UI头文件

CaseManager::CaseManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CaseManager),
    m_sessionKey(sessionKey)
{
    ui->setupUi(this);
}

CaseManager::~CaseManager()
{
    delete ui;
}
