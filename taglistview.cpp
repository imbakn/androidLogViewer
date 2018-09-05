#include "taglistview.h"
#include "ui_taglistview.h"

TagListView::TagListView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagListView)
{
    ui->setupUi(this);
}

TagListView::~TagListView()
{
    delete ui;
}

QListWidget *TagListView::getWidgetList()
{
    return ui->listWidget;
}

QComboBox * TagListView::getQComboBox()
{
    return ui->comboBox;
}
