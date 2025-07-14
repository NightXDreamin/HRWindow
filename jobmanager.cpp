#include "jobmanager.h"
#include "ui_jobmanager.h"

jobmanager::jobmanager(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::jobmanager)
{
    ui->setupUi(this);
}

jobmanager::~jobmanager()
{
    delete ui;
}
