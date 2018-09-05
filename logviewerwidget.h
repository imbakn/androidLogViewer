#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H

#include <QWidget>

namespace Ui {
class LogViewerWidget;
}

class LogViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogViewerWidget(QWidget *parent = 0);
    ~LogViewerWidget();

private:
    Ui::LogViewerWidget *ui;
};

#endif // LOGVIEWERWIDGET_H
