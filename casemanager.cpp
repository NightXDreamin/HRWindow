#include "casemanager.h"
#include "ui_casemanager.h"

casemanager::casemanager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::caseManager)
{
    ui->setupUi(this);
}

casemanager::~casemanager()
{
    delete ui;
}
