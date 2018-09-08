#include "loglist.h"
#include "ui_loglist.h"

LogList::LogList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogList)
{
    ui->setupUi(this);
}

LogList::~LogList()
{
    delete ui;
}
