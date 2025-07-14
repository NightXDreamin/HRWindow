#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <QWidget>

namespace Ui {
class jobmanager;
}

class jobmanager : public QWidget
{
    Q_OBJECT

public:
    explicit jobmanager(QWidget *parent = nullptr);
    ~jobmanager();

private:
    Ui::jobmanager *ui;
};

#endif // JOBMANAGER_H
