#ifndef TAGLISTVIEW_H
#define TAGLISTVIEW_H

#include <QWidget>
#include <QComboBox>
#include <QListWidget>

namespace Ui {
class TagListView;
}

class TagListView : public QWidget
{
    Q_OBJECT

public:
    explicit TagListView(QWidget *parent = 0);
    ~TagListView();
    QListWidget * getWidgetList();
    QComboBox * getQComboBox();

private:
    Ui::TagListView *ui;
};

#endif // TAGLISTVIEW_H
