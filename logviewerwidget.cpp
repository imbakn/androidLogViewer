#include "logviewerwidget.h"
#include "ui_logviewerwidget.h"

LogViewerWidget::LogViewerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogViewerWidget)
{
    ui->setupUi(this);
}

LogViewerWidget::~LogViewerWidget()
{
    delete ui;
}
