#ifndef LOGLIST_H
#define LOGLIST_H

#include <QWidget>

namespace Ui {
class LogList;
}

class LogList : public QWidget
{
    Q_OBJECT

public:
    explicit LogList(QWidget *parent = 0);
    ~LogList();

private:
    Ui::LogList *ui;
};

#endif // LOGLIST_H
