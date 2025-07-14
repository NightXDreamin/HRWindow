#ifndef CASEMANAGER_H
#define CASEMANAGER_H

#include <QWidget>
#include "datastructures.h"

namespace Ui {
class CaseManager;
}

class casemanager : public QWidget
{
    Q_OBJECT

public:
    explicit casemanager(QWidget *parent = nullptr);
    ~casemanager();

private:
    Ui::CaseManager *ui;
};

#endif // CASEMANAGER_H
