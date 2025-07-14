#ifndef CASEMANAGER_H
#define CASEMANAGER_H

#include <QWidget>

namespace Ui {
class caseManager;
}

class casemanager : public QWidget
{
    Q_OBJECT

public:
    explicit casemanager(QWidget *parent = nullptr);
    ~casemanager();

private:
    Ui::caseManager *ui;
};

#endif // CASEMANAGER_H
