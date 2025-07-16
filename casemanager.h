// casemanager.h
#ifndef CASEMANAGER_H
#define CASEMANAGER_H

#include <QWidget>
#include "datastructures.h" // 引入数据结构定义

namespace Ui {
class CaseManager; // 假设UI类名是CaseManager
}

class CaseManager : public QWidget
{
    Q_OBJECT

public:
    // 构造函数现在接收会话密钥
    explicit CaseManager(const QString &sessionKey, QWidget *parent = nullptr);
    ~CaseManager();

private:
    Ui::CaseManager *ui;
    const QString m_sessionKey;
    // ... 未来这里会添加案例列表、网络管理器等 ...
};

#endif // CASEMANAGER_H
