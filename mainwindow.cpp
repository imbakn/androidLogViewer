#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplitter>
#include <QTextEdit>
#include <QDockWidget>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QSqlTableModel>
#include <QTableView>
#include "tableeditor.h"
#include <QDebug>
#include <QDateTime>
#include <QMenu>
#include <QLineEdit>
#include <QPair>
#include <QTimer>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QClipboard>
#include <util.h>

#define DEBUG_SWITCH 0

static int MODE_FUZZY = 0;
static int MODE_NORMAL = 1;
static int MODE_SQL = 2;

MainWindow::MainWindow(QWidget *parent,QStringList flist) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isMenuBarShow(true),
    totalCount(0),
    totalPage(0),
    currentLine(0),
    currentPage(0),
    sizeChanged(false),
    sTimer(NULL),
    selectWholeQuery(NULL),
    hasOpenedFile(false)
{
    ui->setupUi(this);

    InitWindow();

    //创建日志等级颜色配色表
    QString colorStr = "#0843CE,#007BB9,#007F00,#FF7F00,#FF0000,#A50000,#FFFFFF";
    QStringList corlorList = colorStr.split(",");
    mColorConfig = new QList<QColor>;
    for (int i = 0; i < 7; ++i) {
        mColorConfig->append(QColor(corlorList.at(i)));
    }

    if(!flist.isEmpty())
    {
        loadFiles(flist);
    }
}

bool MainWindow::initSql(const QString& table,const QString& where)
{
    if(!sqliteDB.isOpen())
        return false;
    totalCount = getCountFromQuery(table,where);
    return execSql(table,where);
}

int MainWindow::binarySearch(int index)
{
    if(ui->etFind->text().isEmpty() || idListToSearch.isEmpty() || idListToFind.isEmpty() || idListToFind.length()<=index || index < 0)
    {
        return -1;
    }
    int key = idListToFind.at(index);
    int len = idListToSearch.length();

    int index1 = 0;
    int index2 = len-1;
    while(index1<=index2)
    {
        if(key==idListToSearch.at((index1+index2)/2))
        {
            qDebug()<<"find"<<endl;
            return (index1+index2)/2;
        }else if(key<idListToSearch.at(index1+index2)/2)
        {
            index2 = (index1+index2)/2 - 1;
        }else
        {
            index1 = (index1+index2)/2 + 1;
        }
        qDebug()<<index1<<" "<<index2<<endl;
    }
    qDebug()<<"not found"<<endl;
    return -1;
}


void MainWindow::initFindList()
{
    QString where = getSearchAndFindSqlString();
    QString sql = where.isEmpty()?
                QString("select id from logcat"):
                QString("select id from logcat where %1").arg(where);
    QSqlQuery sqlQuery(sqliteDB);
    QString noLevelStr = getSearchSqlString();
    noLevelStr = noLevelStr.remove("\'V\'").remove("\'D\'").remove("\'I\'").remove("\'W\'").remove("\'E\'").remove("\'F\'");
    if(noLevelStr.toLower() == noLevelStr)
    {
        sqlQuery.prepare("PRAGMA case_sensitive_like=0");
    }
    else
    {
        sqlQuery.prepare("PRAGMA case_sensitive_like=1");
    }
    if(!sqlQuery.exec(sql))
    {
        qDebug()<<sqlQuery.lastError();
        return;
    }
    QTime time;
    time.start();
    idListToFind.clear();
    while(sqlQuery.next())
    {
        idListToFind.append(sqlQuery.value(0).toInt());
    }
    qDebug()<<"存储 FIND ID COUNT:"<<idListToFind.size()<<" 耗时:"<<time.elapsed()<<"ms";

    int line = binarySearch(0);
    gotoLine(line);

}

bool MainWindow::execSql(const QString& table,const QString& where)
{
    if(!sqliteDB.isOpen())
        return false;
    QString sql = where.isEmpty()?
                QString("select * from %1").arg(table):
                QString("select * from %1 where %2").arg(table).arg(where);
    if(selectWholeQuery != NULL)
        delete selectWholeQuery;
    selectWholeQuery = new QSqlQuery(sqliteDB);
    QString noLevelStr = where;
    noLevelStr = noLevelStr.remove("\'V\'").remove("\'D\'").remove("\'I\'").remove("\'W\'").remove("\'E\'").remove("\'F\'");
    if(noLevelStr.toLower() == noLevelStr)
    {
        //qDebug()<<"PRAGMA case_sensitive_like=0";
        selectWholeQuery->prepare("PRAGMA case_sensitive_like=0");
    }
    else
    {
        //qDebug()<<"PRAGMA case_sensitive_like=1";
        selectWholeQuery->prepare("PRAGMA case_sensitive_like=1");
    }

    if(!selectWholeQuery->exec(sql))
    {
        qDebug()<<selectWholeQuery->lastError();
        return false;
    }
    QTime time;
    time.start();
    idListToSearch.clear();
    while(selectWholeQuery->next())
    {
        idListToSearch.append(selectWholeQuery->value(0).toInt());
    }
    qDebug()<<"存储ID COUNT:"<<idListToSearch.size()<<" 耗时:"<<time.elapsed()<<"ms";
    initFindList();
    return true;
}

bool MainWindow::isTableExist(QString table)
{
    QString select_count_sql = QString("select count(*) from sqlite_master where type=\'table\' and name =\'%1\'").arg(table);
    QSqlDatabase database = QSqlDatabase::database("loading");
    QSqlQuery query(database);
    if(!query.exec(select_count_sql))
    {
        qDebug()<<query.lastError();
        return false;
    }
    else
    {
        if(query.next())
        {
            return (query.value(0).toInt()>0);
        }else
        {
            return false;
        }
    }
}

int MainWindow::getCountFromQuery(const QString& table,const QString& where)
{
    QString select_count_sql = where.isEmpty()?
                QString("select count(*) from %1").arg(table):
                QString("select count(*) from %1 where %2").arg(table).arg(where);
    QSqlQuery query(sqliteDB);
    if(!query.exec(select_count_sql))
    {
        qDebug()<<query.lastError();
        return -1;
    }
    else
    {
        if(query.next())
        {
            return query.value(0).toInt();
        }else
        {
            return -1;
        }
    }
}

void MainWindow::InitWindow()
{
    commonInit();
    createDock();
    createActionAndMenu();
    createToolBar();
    createStatusBar();
    connectUpdateLineSlot(true);
}

void MainWindow::initTagListDock()
{
    initPidList();
    tagListView->getQComboBox()->addItems(pidList);
    initTagList(-1);
    disconnect(tagListView->getQComboBox(),SIGNAL(currentTextChanged(QString)),this,SLOT(tagListViewComboChangedSlot(QString)));
    disconnect(tagListView->getWidgetList(),SIGNAL(currentRowChanged(int)),this,SLOT(delaySearch()));
    connect(tagListView->getQComboBox(),SIGNAL(currentTextChanged(QString)),this,SLOT(tagListViewComboChangedSlot(QString)));
    connect(tagListView->getWidgetList(),SIGNAL(currentRowChanged(int)),this,SLOT(delaySearch()));

    connect(ui->time,SIGNAL(toggled(bool)),this,SLOT(reDispAreaData()));
    connect(ui->level,SIGNAL(toggled(bool)),this,SLOT(reDispAreaData()));
    connect(ui->tag,SIGNAL(toggled(bool)),this,SLOT(reDispAreaData()));
    connect(ui->pid,SIGNAL(toggled(bool)),this,SLOT(reDispAreaData()));
}

void MainWindow::delaySearch()
{
    if(sTimer == NULL)
    {
        sTimer = new QTimer();
        sTimer->setInterval(200);
        sTimer->setSingleShot(true);
        connect(sTimer,SIGNAL(timeout()),this,SLOT(beginSearch()));
    }
    sTimer->stop();
    sTimer->start();
}

void MainWindow::tagListViewComboChangedSlot(QString text)
{
    if(text == "All")
        initTagList(-1);
    else{
        initTagList(text.toInt());
    }
}

int MainWindow::getFilterLevel()
{
    return ui->cbLevel->currentIndex();
}

int MainWindow::getQueryMode()
{
    return ui->cbMode->currentIndex();
}

void MainWindow::commonInit()
{
    setWindowState(Qt::WindowMaximized);
    setWindowTitle("AndroidLogcatViewer");
    setWindowIcon(QIcon(":/logo.ico"));

    lwContent = ui->lwContent;
    lwContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mCurRange = new ItemsRange(this,0,0,0,0,0);
    isConnectScroll(true);

    connect(ui->etSearch,SIGNAL(editingFinished()),this,SLOT(beginSearch()));

    connect(ui->cbLevel,SIGNAL(currentIndexChanged(int)),this,SLOT(cbLevelChangeSlot(int)));
    connect(ui->cbMode,SIGNAL(currentIndexChanged(int)),this,SLOT(cbModeChangeSlot(int)));
    connect(ui->lwContent,SIGNAL(currentTextChanged(QString)),this,SLOT(currentTextContentChangedSlot(QString)));
    connect(ui->etFind,SIGNAL(editingFinished()),this,SLOT(initFindList()));
}

void MainWindow::connectUpdateLineSlot(bool flag)
{
    if(lineCurrentEdit == NULL || pageCurrentEdit == NULL)
        return;
    if(flag)
    {
        connect(lineCurrentEdit,SIGNAL(editingFinished()),this,SLOT(updateByLine()));
        connect(pageCurrentEdit,SIGNAL(editingFinished()),this,SLOT(updateByPage()));
    }else
    {
        disconnect(lineCurrentEdit,SIGNAL(editingFinished()),this,SLOT(updateByLine()));
        disconnect(pageCurrentEdit,SIGNAL(editingFinished()),this,SLOT(updateByPage()));
    }
}

void MainWindow::currentTextContentChangedSlot(QString text)
{
    if(text.isEmpty())
        return;
    currentLineText = text;
    connectUpdateLineSlot(false);
    currentLine = text.split(' ').at(0).toInt();
    lineCurrentEdit->setText(QString::number(currentLine));
    if(totalCount > 0)
    {
        currentPage = currentLine/mCurRange->getPageItemNum() + 1;
        pageCurrentEdit->setText(QString::number(currentPage));
    }
    connectUpdateLineSlot(true);
}

void MainWindow::cbLevelChangeSlot(int index)
{
    //qDebug()<<"cbLevelChangeSlot: "<<index;
    beginSearch();
}

void MainWindow::cbModeChangeSlot(int index)
{
    if(index == MODE_FUZZY)
    {
        tagListView->setDisabled(false);
        tagListDock->show();
        ui->cbLevel->setDisabled(false);
        ui->cbLevel->show();
        toggleProject->setDisabled(false);
    }else
    {
        tagListView->setDisabled(true);
        tagListDock->hide();
        ui->cbLevel->setDisabled(true);
        ui->cbLevel->hide();
        toggleProject->setDisabled(true);
    }
    beginSearch();
}

void MainWindow::createDock()
{
    //停靠窗口1
    tagListDock=new QDockWidget(tr("TagList"),this);
    tagListDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    tagListDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    tagListDock->setTitleBarWidget(new QWidget());
    tagListView = new TagListView();
    //TODO:
    QFont font = QFont("Ubuntu Mono",14,QFont::Normal);
    tagListView->setFont(font);

    tagListDock->setWidget(tagListView);
    addDockWidget(Qt::LeftDockWidgetArea,tagListDock);
    //停靠窗口2
    filterDock=new QDockWidget(tr("Filter"),this);
    filterDock->setFeatures(QDockWidget::DockWidgetMovable);
    filterDock->setAllowedAreas(Qt::RightDockWidgetArea);
    QTextEdit *te2 =new QTextEdit();
    filterDock->setWidget(te2);
    addDockWidget(Qt::RightDockWidgetArea,filterDock);
    filterDock->hide();
    //停靠窗口3
    consoleDock=new QDockWidget(tr("Console"),this);
    consoleDock->setFeatures(QDockWidget::DockWidgetMovable);
    consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea|Qt::RightDockWidgetArea);
    QTextEdit *te3 =new QTextEdit();
    consoleDock->setWidget(te3);
    addDockWidget(Qt::BottomDockWidgetArea,consoleDock);
    consoleDock->hide();
}

void MainWindow::createActionAndMenu()
{
    QAction *fullscreenAction;
    QAction *hidemenuAction;
    QAction *loadfileAction;
    QAction *closefileAction;


    QAction *firstPageAction;
    QAction *lastPageAction;

    QAction *copyCurrentPageAction;
    QAction *copyCurrentLineAction;

    QAction *gotoPrePageAction;
    QAction *gotoNextPageAction;

    fullscreenAction = new QAction("全屏",this);
    fullscreenAction->setShortcut(Qt::Key_F11 );
    connect(fullscreenAction,SIGNAL(triggered(bool)),this,SLOT(changeFullScreenSlot()));

    hidemenuAction = new QAction("隐藏菜单",this);
    hidemenuAction->setShortcut(Qt::Key_F10 );
    connect(hidemenuAction,SIGNAL(triggered(bool)),this,SLOT(showMenuSlot()));

    loadfileAction = new QAction("打开",this);
    loadfileAction->setShortcut(Qt::Key_F3 );
    connect(loadfileAction,SIGNAL(triggered(bool)),this,SLOT(loadFileSlot()));

    closefileAction = new QAction("关闭",this);
    closefileAction->setShortcut(Qt::Key_F4 );
    connect(closefileAction,SIGNAL(triggered(bool)),this,SLOT(closeFileSlot()));

    copyCurrentPageAction = new QAction("复制当前页",this);
    copyCurrentPageAction->setShortcut(Qt::Key_F5);
    connect(copyCurrentPageAction,SIGNAL(triggered(bool)),this,SLOT(clipCurrentPage()));

//    copyCurrentLineAction = new QAction("复制当前行",this);
//    copyCurrentLineAction->setShortcut(Qt::Key_F6);
//    connect(copyCurrentLineAction,SIGNAL(triggered(bool)),this,SLOT(clipCurrentLine()));


    firstPageAction = new QAction("第一页",this);
    firstPageAction->setShortcut(Qt::Key_F7);
    connect(firstPageAction,SIGNAL(triggered(bool)),this,SLOT(gotoFirstPageSlot()));

    lastPageAction = new QAction("最后一页",this);
    lastPageAction->setShortcut(Qt::Key_F8);
    connect(lastPageAction,SIGNAL(triggered(bool)),this,SLOT(gotoLastPageSlot()));


    gotoPrePageAction = new QAction("上一页",this);
//    gotoPrePageAction->setShortcut(Qt::Key_F9);
    connect(gotoPrePageAction,SIGNAL(triggered(bool)),this,SLOT(gotoPrePageSlot()));

    gotoNextPageAction = new QAction("下一页",this);
//    gotoNextPageAction->setShortcut(Qt::Key_F10);
    connect(gotoNextPageAction,SIGNAL(triggered(bool)),this,SLOT(gotoNextPageSlot()));

    QFont font = QFont("Ubuntu Mono",12,QFont::Normal);
    menuBar()->setFont(font);

    QMenu *fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(loadfileAction);
    fileMenu->addAction(closefileAction);

    QMenu *queryMenu = menuBar()->addMenu(tr("查看"));
    queryMenu->addAction(fullscreenAction);
    queryMenu->addAction(hidemenuAction);
//    queryMenu->addAction(copyCurrentLineAction);
    queryMenu->addAction(copyCurrentPageAction);

    queryMenu->addAction(firstPageAction);
    queryMenu->addAction(lastPageAction);

    queryMenu->addAction(gotoPrePageAction);
    queryMenu->addAction(gotoNextPageAction);
}

void MainWindow::createToolBar()
{
    ui->mainToolBar->hide();
}

void MainWindow::createStatusBar()
{

    toggleProject = new QPushButton();
    toggleProject->setFlat(true);
    toggleProject->setFixedSize(25,25);
    toggleProject->setCheckable(true);
    toggleProject->setChecked(true);
    toggleProject->setText("P");
    statusBar()->addWidget(toggleProject);

    QPushButton *menuBtn = new QPushButton();
    menuBtn->setFixedSize(25,25);
    menuBtn->setText("M");
    statusBar()->addPermanentWidget(menuBtn);

    QPushButton *toggleConsole = new QPushButton();
    toggleConsole->setCheckable(true);
    toggleConsole->setFixedSize(25,25);
    toggleConsole->setText("C");
    toggleConsole->setFlat(true);
    statusBar()->addPermanentWidget(toggleConsole);

    QPushButton *toggleFilter = new QPushButton();
    toggleFilter->setCheckable(true);
    toggleFilter->setFixedSize(25,25);
    toggleFilter->setText("F");
    toggleFilter->setFlat(true);
    statusBar()->addPermanentWidget(toggleFilter);

    connect(toggleProject,SIGNAL(clicked(bool)),this,SLOT(showOrHideProjectWindow()));
    connect(toggleFilter,SIGNAL(clicked(bool)),this,SLOT(showOrHideFilterWindow()));
    connect(toggleConsole,SIGNAL(clicked(bool)),this,SLOT(showOrHideConsoleWindow()));
    connect(menuBtn,SIGNAL(clicked(bool)),this,SLOT(showMenuSlot()));

    QLabel *totalLineLabelInfo = new QLabel("总行数:");
    QLabel *totalPageLabelInfo = new QLabel("总页数:");
    QLabel *currentLineLabelInfo = new QLabel("当前行数:");
    QLabel *currentPageLabelInfo = new QLabel("当前页数:");

    lineTotalLabel = new QLabel();
    pageTotalLabel = new QLabel();
    lineTotalLabel->setFixedWidth(80);
    pageTotalLabel->setFixedWidth(80);
    lineCurrentEdit = new QLineEdit();
    lineCurrentEdit->setMaximumWidth(80);
    pageCurrentEdit = new QLineEdit();
    pageCurrentEdit->setMaximumWidth(80);

    statusBar()->addWidget(totalLineLabelInfo);
    statusBar()->addWidget(lineTotalLabel);
    statusBar()->addWidget(currentLineLabelInfo);
    statusBar()->addWidget(lineCurrentEdit);


    statusBar()->addWidget(totalPageLabelInfo);
    statusBar()->addWidget(pageTotalLabel);
    statusBar()->addWidget(currentPageLabelInfo);
    statusBar()->addWidget(pageCurrentEdit);

    QPushButton *prevPageBtn = new QPushButton;
    QPushButton *nextPageBtn = new QPushButton;
    prevPageBtn->setText(tr("上一页"));
    nextPageBtn->setText(tr("下一页"));

    statusBar()->addWidget(prevPageBtn);
    statusBar()->addWidget(nextPageBtn);

    connect(prevPageBtn,SIGNAL(clicked(bool)),this,SLOT(gotoPrePageSlot()));
    connect(nextPageBtn,SIGNAL(clicked(bool)),this,SLOT(gotoNextPageSlot()));
}

void MainWindow::isConnectScroll(bool isConnect)
{
    if (isConnect) {
        QObject::connect(ui->lwContent->verticalScrollBar(),SIGNAL(valueChanged(int)),
                         this,SLOT(selfVerticalScrollSlot(int)));

        QObject::connect(ui->verticalScrollBar,SIGNAL(valueChanged(int)),
                         this,SLOT(verticalScrollSlot(int)));

    } else {
        QObject::disconnect(ui->lwContent->verticalScrollBar(),SIGNAL(valueChanged(int)),
                         this,SLOT(selfVerticalScrollSlot(int)));

        QObject::disconnect(ui->verticalScrollBar,SIGNAL(valueChanged(int)),
                         this,SLOT(verticalScrollSlot(int)));

    }
}

void MainWindow::beginSearch()
{
    QString sql = getSearchSqlString();
    if(sql == lastSql && !sizeChanged)
        return;
    bool flag = initSql("logcat",sql);

    lastSql = sql;
    if(flag)
        dispContainSearchString("");
}

QString MainWindow::getFindSqlString()
{
    QString sql = "";
    QString find = ui->etFind->text().trimmed();

    if(!find.isEmpty())
    {
        QStringList argList = find.split(' ');
        qDebug()<<argList;
        sql = Util::getNormalQueryString(argList);
    }
    qDebug()<<"The find sql: "<<sql;

    return sql;
}

QString MainWindow::getSearchAndFindSqlString()
{
    QStringList sql;
    QString search = getSearchSqlString();
    QString find = getFindSqlString();
    if(!search.isEmpty())
        sql.append("("+search+")");
    if(!find.isEmpty())
        sql.append("("+find+")");

    return sql.join(" and ");
}

QString MainWindow::getSearchSqlString()
{
    QString sql = "";
    QString search = ui->etSearch->text().trimmed();
    if(getQueryMode()==MODE_NORMAL)
    {
        if(!search.isEmpty())
        {
            QStringList argList = search.split(' ');
            qDebug()<<argList;
            sql = Util::getNormalQueryString(argList);
        }
    }else if(getQueryMode()==MODE_SQL)
    {
        sql = search;
    }else if(getQueryMode()==MODE_FUZZY)
    {
        QStringList queryList;
        if(!search.isEmpty())
        {
            QStringList argList = search.split(' ');
            qDebug()<<argList;
            foreach (QString arg, argList) {
                arg = arg.trimmed();
                if(arg.isEmpty())
                    continue;

                QString tmp = Util::getSqliteQueryStringLike(arg,"","message");
                if(!tmp.isEmpty())
                    queryList.append(tmp);
            }
        }
        queryList.append(Util::getLevelSql(ui->cbLevel->currentIndex()));
        QString pidText = tagListView->getQComboBox()->currentText();
        if(pidText != "All")
        {
            queryList.append(QString("pid=%1").arg(pidText));
        }
        if(tagListView->getWidgetList()->count() > 0 && tagListView->getWidgetList()->currentItem() != NULL)
        {
            QString tagText = tagListView->getWidgetList()->currentItem()->text();
            {
                int find = tagText.lastIndexOf("(");
                if(find >= 0)
                {
                    QString tt = tagText.left(find).trimmed();
                    if(!tt.startsWith("All"))
                        queryList.append(QString("tag=\'%1\'").arg(tt));
                }
            }
        }
        sql = queryList.join(" and ");
    }
    qDebug()<<"The search sql: "<<sql;
    return sql;
}

void MainWindow::selfVerticalScrollSlot(int value)
{
    logCurTime("内部滑动：value=" + QString::number(value) + ", mLastScrollValue=" + QString::number(mLastScrollValue));
    int direction = 0;
    int step = value - mLastScrollValue;
    //与上一次的值作比较，判断滑动的方向，1表示向下滑动，-1表示向上滑动，0表示未滑动
    if (value > mLastScrollValue) {
        direction = 1;
    } else if (value < mLastScrollValue) {
        direction = -1;
    }

    //logCurTime("由" + QString::number(mLastScrollValue) + " >> " + QString::number(value));
    //如果是向下滑动
    if (direction == 1) {
        int first = mCurRange->getFirst();
        int visibleFirst = mCurRange->getVisibleFirst();
        int count = mCurRange->getCount();
        int pageItemNum = mCurRange->getPageItemNum();
        if ((visibleFirst != 0)
                && (mCurRange->getLast() != (totalCount - 1))) {
            first += step;
        } else {
            mLastScrollValue = value;
        }
        visibleFirst += step;
        delete mCurRange;
        mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
        dispAreaData(mCurRange,direction);
    }
    //如果是向上滑动
    else if (direction == -1) {
        int first = mCurRange->getFirst();
        int visibleFirst = mCurRange->getVisibleFirst();
        int count = mCurRange->getCount();
        int pageItemNum = mCurRange->getPageItemNum();
        if ((first != 0)
                && (mCurRange->getLast() != (totalCount - 1))) {
            first += step;
        } else if (mCurRange->getLast() == (totalCount - 1)) {
            if ((visibleFirst + step) <= first) {
                first += step;
                if ((value == 0) && (first != 0)) {
                    mLastScrollValue = visibleFirst + step - first;
                }
            } else {
                mLastScrollValue = value;
            }
        } else {
            mLastScrollValue = value;
        }
        visibleFirst += step;
        delete mCurRange;
        mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
        dispAreaData(mCurRange,direction);
    }
    //如果未滑动
    else if (value == 0) {
        mLastScrollValue = 3;
        lwContent->verticalScrollBar()->setValue(1);
    }

}

void MainWindow::reDispAreaData()
{
    dispAreaData(mCurRange,0);
}

void MainWindow::dispAreaData(ItemsRange *range, int direction)
{
    static bool flag = false;
    isConnectScroll(false);         //必须先断开滚动条的信号与槽的连接，否则将造成很多不必要的计算，浪费时间
    lwContent->clear();

    int bitNum = QString::number(totalCount).length();
    if (bitNum < 2) {
        bitNum = 2;
    }

    int first = range->getFirst();
    int last = range->getLast();

    logCurTime("显示范围：" + QString::number(first) + " ,"
               + QString::number(range->getVisibleFirst()) + " ,"
               + QString::number(range->getPageItemNum()) + " ,"
               + QString::number(range->getCount()) + " ,"
               + QString::number(last));

    QString tempText;
    QColor color;
    //根据缓冲区的范围显示对应的数据
    bool seekResult = selectWholeQuery->seek(first);
    if(seekResult)
    {
        for (int i = first; i <= last; ++i) {

            tempText = QString("%1 %2%3%4%5 %6")
                    .arg(Util::formatNumber(i + 1, bitNum))
                     .arg(ui->time->isChecked()?" "+selectWholeQuery->value(1).toString()+" ":"")
                     .arg(ui->level->isChecked()?" "+selectWholeQuery->value(2).toString()+" ":"")
                     .arg(ui->tag->isChecked()?" "+Util::formatTag(selectWholeQuery->value(3).toString())+" ":"")
                     .arg(ui->pid->isChecked()?" "+Util::formatPid(selectWholeQuery->value(4).toString())+" ":"")
                     .arg(selectWholeQuery->value(5).toString());


            lwContent->addItem(tempText);
            color = mColorConfig->at(Util::getLevelIndex(selectWholeQuery->value(2).toString()));
            //将对应日志等级的文本设置成对应的颜色
            QListWidgetItem *item = lwContent->item(i - first );
            if (item == NULL) {
                continue;
            }
            item->setTextColor(color);
            if(!selectWholeQuery->next())
            {
                qDebug()<<"error break;";
                break;
            }
        }
    }else
    {
        qDebug()<<"seek error :"<<selectWholeQuery->lastError();
    }


    if (lwContent->count() > 0) {
        //如果是向下滑动，则将焦点设置在最后一行；否则，将焦点设置在第一行
        if (direction == 1) {
            lwContent->scrollToTop();
            lwContent->setCurrentRow(range->getVisibleFirst() + range->getPageItemNum() - 2 - first);
        } else {
            lwContent->scrollToBottom();
            lwContent->setCurrentRow(range->getVisibleFirst() - first);
        }

        //重新设定滚动条的位置
        ui->verticalScrollBar->setValue(range->getVisibleFirst());
    }

    //获取最新的内部滚动条的值，否则外部滚动条将会发生紊乱
    mLastScrollValue = lwContent->verticalScrollBar()->value();

    isConnectScroll(true);

    //如果显示范围是：0,0,-1，且当前应该是有数据的，则立即恢复显示
    if ((first == 0) && (range->getVisibleFirst() == 0) && (last == -1)) {
        if ((totalCount > 0) && (flag == false)) {
            flag = true;
            logCurTime("当前显示范围：0,0,-1，进一步检查是否真的无数据显示...");
            dispContainSearchString(ui->etSearch->text());
        } else {
            flag = false;
        }
    } else {
        flag = false;
    }
    updateLineAndPage();
}

void MainWindow::updateCurRange()
{
    //if (lwFilter->count() > 0) {
        int visibleHeight = lwContent->height();        //获取内容显示区的总高度
        int itemHeight = tagListView->getWidgetList()->sizeHintForRow(0);   //获取一行所占的高度getCountFromQuery
        int visibleItemNum = visibleHeight / itemHeight;//计算可完整显示的行数

        if (visibleItemNum < 0) {
            visibleItemNum = 0;
        }
        if(visibleItemNum > 0)
            totalPage = totalCount/visibleItemNum + 1;

        //更新当前显示缓冲区范围
        int first = mCurRange->getFirst();
        int visibleFirst = mCurRange->getVisibleFirst();
        int count = visibleItemNum + visibleItemNum / 2;
//        qDebug()<<"visibleHeight:"<<visibleHeight;
//        qDebug()<<"itemHeight:"<<itemHeight;
//        qDebug()<<"visibleItemNum:"<<visibleItemNum;
//        qDebug()<<"count:"<<count;
        delete mCurRange;
        mCurRange = new ItemsRange(this,first,visibleFirst,count,visibleItemNum,totalCount/*mCurLevels.size()*/);
    //}
}

void MainWindow::verticalScrollSlot(int value)
{
    logCurTime("外部滑动：value=" + QString::number(value));
    int visibleFirst = value;
    int first = (visibleFirst > 0) ? (visibleFirst - 1) : visibleFirst;
    int count = mCurRange->getCount();
    int pageItemNum = mCurRange->getPageItemNum();
    delete mCurRange;
    mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
    dispAreaData(mCurRange,0);
}

void MainWindow::dispContainSearchString(QString str)
{
    //if (mAllLines > 0) {
        //加载大于等于某一日志等级且指定Tag的所有日志信息
        //loadLevelMessage(ui->cbLevel->currentText(), mCurFilters.at(ui->lwFilter->currentRow()));

        updateCurRange();   //更新当前显示缓冲区范围


        //updateTimeStringToWidget(mStartTime,mStopTime);

        //显示当前数据表
        dispCurDataList();
   // }
}

void MainWindow::dispCurDataList()
{
    //计算滚动条的最大值
    int max = mCurRange->getTotal() - mCurRange->getPageItemNum() + 1;
    //如果该值大于0则显示滚动条，否则隐藏
    if (max > 0) {
        ui->verticalScrollBar->setHidden(false);
        ui->verticalScrollBar->setRange(0,max);
    } else {
        ui->verticalScrollBar->setHidden(true);
        ui->verticalScrollBar->setRange(0,0);
    }

    //autoAdjustTitleLabel();     //自动调节标题标签的位置

    dispAreaData(mCurRange,0);  //将显示缓冲区的数据显示到屏幕
}

void MainWindow::showOrHideWindow(QWidget *widget)
{
    if(widget->isHidden())
    {
        widget->show();
    }else
    {
        widget->hide();
    }
}

void MainWindow::showOrHideProjectWindow()
{
    showOrHideWindow(tagListDock);
}

void MainWindow::showOrHideFilterWindow()
{
    showOrHideWindow(filterDock);
}

void MainWindow::showOrHideConsoleWindow()
{
    showOrHideWindow(consoleDock);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(sqliteDB.isOpen())
        sqliteDB.close();
    if(QSqlDatabase::contains("show_logcat"))
        QSqlDatabase::removeDatabase("show_logcat");
}

void MainWindow::logCurTime(QString text)
{
#if DEBUG_SWITCH
    qDebug()<<"【" + QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "】" + text;
#endif
}

void MainWindow::changeFullScreenSlot()
{
    if(this->isFullScreen())
    {
        this->showNormal();
        this->showMaximized();
    }else
    {
        this->showFullScreen();
    }
}

void MainWindow::showMenuSlot()
{
    if(isMenuBarShow)
    {
        ui->menuBar->hide();
        isMenuBarShow = false;
    }else
    {
        ui->menuBar->show();
        isMenuBarShow = true;
    }
}

void MainWindow::resizeEvent(QResizeEvent * event)
{
    sizeChanged = true;
    beginSearch();
    sizeChanged = false;
}

void MainWindow::updateLineAndPage()
{
    lineTotalLabel->setText(QString::number(totalCount));
    pageTotalLabel->setText(QString::number(totalPage));
}

void MainWindow::updateByLine()
{
    if(lineCurrentEdit->text().isEmpty())
        return;
    if(lineCurrentEdit->text() != "0" && lineCurrentEdit->text().toInt() == 0)
    {
        lineCurrentEdit->clear();
        return;
    }
    int goNum = lineCurrentEdit->text().toInt();
    gotoLine(goNum);
}

void MainWindow::updateByPage()
{
    if(pageCurrentEdit->text().isEmpty())
        return;
    if(pageCurrentEdit->text() != "0" && pageCurrentEdit->text().toInt() == 0)
    {
        pageCurrentEdit->clear();
        return;
    }
    int goPageNum = pageCurrentEdit->text().toInt();
    if(mCurRange != NULL)
        gotoLine(goPageNum*mCurRange->getPageItemNum());
}

void MainWindow::gotoLine(int line)
{
    qDebug()<<"gotoLine: "<<line;
    int first = mCurRange->getFirst();
    int visibleFirst = mCurRange->getVisibleFirst();
    int count = mCurRange->getCount();
    int pageItemNum = mCurRange->getPageItemNum();
    if(line < 0 || line > mCurRange->getTotal())
        return;
    isConnectScroll(false);
    visibleFirst = line-pageItemNum/2;
    if(visibleFirst < 0)
        visibleFirst = 0;
    first = (visibleFirst > 0) ? (visibleFirst - 1) : visibleFirst;
    delete mCurRange;
    mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
    dispAreaData(mCurRange,0);
    isConnectScroll(true);
}

void MainWindow::initPidList()
{
    QString select_pid_sql = "select pid from analize group by pid order by pid";
    QSqlQuery sqlQuery(sqliteDB);
    pidList.clear();
    pidList.append("All");
    if(!sqlQuery.exec(select_pid_sql))
    {
        qDebug()<<sqlQuery.lastError();
    }
    else
    {
        while(sqlQuery.next())
        {
            pidList.append(sqlQuery.value(0).toString());
        }
    }
}

void MainWindow::initTagList(int pid)
{
    QString select_tag_sql = pid<0?
                QString("select tag,sum(`count`) from analize group by tag"):
                QString("select tag,sum(`count`) from analize where pid=%1 group by tag").arg(pid);
    QSqlQuery sqlQuery(sqliteDB);
    tagListView->getWidgetList()->clear();
    if(!sqlQuery.exec(select_tag_sql))
    {
        qDebug()<<sqlQuery.lastError();
    }
    else
    {
        QString tempText;
        int ccc = 0;
        while(sqlQuery.next())
        {
            int cc = sqlQuery.value(1).toInt();
            ccc += cc;
            tempText = QString().sprintf(" %s(%d)",sqlQuery.value(0).toByteArray().data(),cc);
            tagListView->getWidgetList()->addItem(tempText);
        }
        tagListView->getWidgetList()->insertItem(0,QString(" All Message(%1)").arg(ccc));
        if(tagListView->getWidgetList()->count()>0)
        {
            tagListView->getWidgetList()->setCurrentRow(0);
        }
    }
}

void MainWindow::loadFileSlot()
{
    QString fileName = QFileDialog::getOpenFileName(this,
            QString::fromLocal8Bit("打开日志文件"),
            "",
            tr("Log File (*)"));
    QFileInfo fileInfo(fileName);
    if(fileInfo.exists())
    {
        QStringList list;
        list.append(fileName);
        loadFiles(list);
    }
}

void MainWindow::loadFiles(QStringList fileList)
{
    if(fileList.isEmpty())
        return;
    QFileInfo firstfileInfo(fileList[0]);
    //QString targetFileName = "/home/ubuntu/.alog/"+firstfileInfo.baseName()+".log.sqlite";
    QString targetFileName = firstfileInfo.baseName()+".log.sqlite";
    QFile targetFileInfo(targetFileName);
    if(targetFileInfo.exists())
        targetFileInfo.remove();
    //load
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE","loading");
    database.setDatabaseName(targetFileName);
    database.open();
    //qDebug()<<"open error: "<<database.lastError();
    for(int i = 0;i < fileList.size();i++)
    {
        loadFile(fileList.at(i),targetFileName);
    }

    if(database.isOpen())
        database.close();
    QSqlDatabase::removeDatabase("loading");

    loadLogSqliteFile(targetFileName,firstfileInfo.absoluteFilePath());
}

void MainWindow::loadFile(const QString& fileName, const QString& sqlFileName)
{
    QFileInfo fileInfo(fileName);
    if(fileInfo.exists())
    {
        if(hasOpenedFile)
        {
            closeFileSlot();
        }
        if(fileInfo.isDir())
        {
           qDebug()<<"isDir";
        }else if(fileName.endsWith(".rar"))
        {
            qDebug()<<"isRar";
        }else if(fileName.endsWith(".zip"))
        {
            qDebug()<<"isZip";
        }else if(fileName.endsWith(".tar.gz"))
        {
            qDebug()<<"isTarGz";
        }else if(fileName.endsWith(".log") ||
                 fileName.endsWith(".txt") ||
                 fileName.endsWith(".logcat") ||
                 fileInfo.baseName().contains("log"))
        {
            qDebug()<<"isTxt";
            loadTextFile(fileName,sqlFileName);
        }else
        {
            qDebug()<<"isTxt";
        }
    }
}

bool MainWindow::loadTextFile(const QString& fileName, const QString& sqlFileName)
{
    QFileInfo fileInfo(fileName);
    QString targetFileName = sqlFileName.isEmpty()?fileInfo.baseName()+".log.sqlite":sqlFileName;
    QFile logFile(fileName);
    QFile targetFile(targetFileName);
    QString fName = fileInfo.baseName()+fileInfo.suffix();


        QSqlDatabase database = QSqlDatabase::database("loading");
        //qDebug()<<"database error: "<<database.lastError();
        QSqlQuery sql_query(database);

        if(!isTableExist("logcat"))
        {
            QString create_sql = "create table logcat("
                                 "id integer primary key autoincrement, "
                                 "time datetime, "
                                 "level char(1),"
                                 "tag varchar(30), "
                                 "pid integer, "
                                 "message varchar(4096)"
                                 ")";
            sql_query.prepare(create_sql);

            if(!sql_query.exec())
            {
                qDebug() << "Error: Fail to create table logcat." << sql_query.lastError();
                return false;
            }

            qDebug() << "Table created!";
        }

        QTime    tmpTime;
        qDebug()<<"开始解析数据";
        tmpTime.start();
        QRegExp exp("^(.{18}) (\\w)/(.*)\\(([\\s\\d]{5})\\):(.*)$");
        if (!exp.isValid())
        {
            qDebug() << qPrintable(exp.errorString());
            return false;
        }
        if (!logFile.open(QIODevice::ReadOnly|QIODevice::Text))
            return false;

        database.transaction();
        QString insert_sql = "insert into logcat (time,level,tag,pid,message) values (?,?,?,?,?)";
        sql_query.prepare(insert_sql);
        QTextStream in(&logFile);
        while (!in.atEnd())
        {
            QString str = in.readLine();
            int pp = exp.indexIn(str);
            if(pp > -1)
            {
                QString tag = exp.cap(3).trimmed();
                sql_query.bindValue(0,exp.cap(1));
                sql_query.bindValue(1,exp.cap(2));
                sql_query.bindValue(2,tag);
                sql_query.bindValue(3,exp.cap(4).toInt());
                sql_query.bindValue(4,exp.cap(5));
                if(!sql_query.exec())
                {
                    qDebug() << sql_query.lastError();
                    break;
                }
            }
            else
            {
                qDebug()<<"not match: "<<str;
            }
        }
        database.commit();
        logFile.close();
        qDebug()<<"解析数据耗时："<<tmpTime.elapsed()<<"ms"<<endl;
        if(!isTableExist("analize"))
        {
            QString createAnalizeSql = "create table analize("
                                 "pid integer, "
                                 "tag varchar(30), "
                                 "count integer"
                                 ")";
            sql_query.prepare(createAnalizeSql);
            if(!sql_query.exec())
            {
                qDebug() << "Error: Fail to create table." << sql_query.lastError();
                return false;
            }
            qDebug() << "Table created!";
        }

        QString insertQuerysql = "insert into analize select pid,tag,count(*) from logcat group by pid,tag order by pid,tag";
        if(!sql_query.exec(insertQuerysql))
        {
            qDebug()<<sql_query.lastError();
            return false;
        }
        qDebug()<<"解析数据耗时2："<<tmpTime.elapsed()<<"ms"<<endl;

}

bool MainWindow::loadLogSqliteFile(const QString& filename,const QString& oname)
{
    if (QSqlDatabase::contains("show_logcat"))
    {
        sqliteDB = QSqlDatabase::database("show_logcat");
    }else
    {
        sqliteDB = QSqlDatabase::addDatabase("QSQLITE","show_logcat");
        sqliteDB.setDatabaseName(filename);
    }

    if(!sqliteDB.isOpen())
        sqliteDB.open();
    initTagListDock();
    beginSearch();
    hasOpenedFile = true;

    setWindowTitle(QString("AndroidLogcatViewer[%1]").arg(oname));
}

bool MainWindow::closeFileSlot()
{
    if(selectWholeQuery != NULL)
    {
        selectWholeQuery->clear();
        delete selectWholeQuery;
        selectWholeQuery = NULL;
    }
    if (QSqlDatabase::contains("show_logcat"))
    {
        QSqlDatabase::removeDatabase("show_logcat");
    }
    lwContent->clear();
    tagListView->getQComboBox()->clear();
    tagListView->getWidgetList()->clear();
    hasOpenedFile = false;
    setWindowTitle("AndroidLogcatViewer");
}

void MainWindow::clipCurrentPage()
{
    if(selectWholeQuery == NULL)
        return;
    int first = mCurRange->getVisibleFirst();
    int last = first + mCurRange->getPageItemNum();

    bool seekResult = selectWholeQuery->seek(first);
    QString totalText;
    if(seekResult)
    {
        for (int i = first; i <= last; ++i) {

            QString tempText = QString("%1 %2/%3 (%4): %5")
                    .arg(selectWholeQuery->value(1).toString())
                    .arg(selectWholeQuery->value(2).toString())
                    .arg(Util::formatTag(selectWholeQuery->value(3).toString()))
                    .arg(Util::formatPid(selectWholeQuery->value(4).toString()))
                    .arg(selectWholeQuery->value(5).toString());
            totalText = totalText + tempText + "\n";
            if(!selectWholeQuery->next())
            {
                qDebug()<<"error break";
                break;
            }
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(totalText);
    }
}

void MainWindow::clipCurrentLine()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(currentLineText);
}

void MainWindow::gotoFirstPageSlot()
{
    gotoLine(0);
}

void MainWindow::gotoLastPageSlot()
{
    gotoLine(totalCount);
}

void MainWindow::gotoPrePageSlot()
{
    gotoLine((currentPage-2)*mCurRange->getPageItemNum());
}

void MainWindow::gotoNextPageSlot()
{
    gotoLine((currentPage)*mCurRange->getPageItemNum());
}
