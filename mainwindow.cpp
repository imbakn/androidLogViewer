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
#include "databasemanager.h"

#define DEBUG_SWITCH 1

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
    hasOpenedFile(false),
    currentFindIdIndex(0),
    pidList(new QStringList())
{
    ui->setupUi(this);
    dbManager = new DataBaseManager();
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

//    LogView *logview1 = new LogView(this);
//    LogView *logview2 = new LogView(this);
//    QLabel *analize = new QLabel(this);
//    analize->setText(tr("分析界面"));
//    QTabWidget * tabWidget = new QTabWidget(this);
//    tabWidget->addTab(logview1,tr("View1"));
//    tabWidget->addTab(logview2,tr("View2"));
//    tabWidget->addTab(analize,tr("分析"));
//    this->setCentralWidget(tabWidget);


}

MainWindow::~MainWindow()
{
    delete ui;
    dbManager->close();
    if(QSqlDatabase::contains("show_logcat"))
        QSqlDatabase::removeDatabase("show_logcat");
}

bool MainWindow::initSql(const QString& table,const QString& where)
{
    if(!dbManager->isOpen())
        return false;
    totalCount = dbManager->getCountFromQuery(table,where);
    updateLineAndPage();
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
            return (index1+index2)/2;
        }else if(key<idListToSearch.at((index1+index2)/2))
        {
            index2 = (index1+index2)/2 - 1;
        }else
        {
            index1 = (index1+index2)/2 + 1;
        }
    }
    return -1;
}

void MainWindow::initFindList()
{
    if(ui->etFind->text().isEmpty())
        return;

    QString where = getSearchAndFindSqlString();
    if(lastFindWhere == where)
        currentFindIdIndex = 0;
    lastFindWhere = where;
    qDebug()<<"search find sql: "<<where;
    QString sql = where.isEmpty()?
                QString("select id from logcat"):
                QString("select id from logcat where %1").arg(where);
    QSqlQuery sqlQuery(*dbManager->getDatabase());
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
    //qDebug()<<idListToFind;
    qDebug()<<"存储 FIND ID COUNT:"<<idListToFind.size()<<" 耗时:"<<time.elapsed()<<"ms";

    if(idListToFind.count() == 0)
        return;


    gotoFindLine((currentFindIdIndex++)%idListToFind.count());

}

void MainWindow::gotoPrevFind()
{
    if(ui->etFind->text().isEmpty())
        return;
    if(idListToFind.isEmpty()||idListToSearch.isEmpty())
        return;
    int cindex = (currentFindIdIndex--)%idListToFind.count();
    if(cindex < 0)
        cindex = cindex + idListToFind.count();
    gotoFindLine(cindex);
}

void MainWindow::gotoNextFind(){
    if(ui->etFind->text().isEmpty())
        return;
    if(idListToFind.isEmpty()||idListToSearch.isEmpty())
        return;
    gotoFindLine((currentFindIdIndex++)%idListToFind.count());
}

void MainWindow::gotoFindLine(int index)
{
    int line = binarySearch(index);
    if(line >= 0)
        gotoLine(line+1);
    else
    {
        qDebug()<<"not find!!";
    }
}

bool MainWindow::execSql(const QString& table,const QString& where)
{
    if(!dbManager->isOpen())
        return false;
    QString sql = where.isEmpty()?
                QString("select * from %1").arg(table):
                QString("select * from %1 where %2").arg(table).arg(where);
    if(selectWholeQuery != NULL)
        delete selectWholeQuery;
    selectWholeQuery = new QSqlQuery(*dbManager->getDatabase());
    QString noLevelStr = where;
    noLevelStr = noLevelStr.remove("\'V\'").remove("\'D\'").remove("\'I\'").remove("\'W\'").remove("\'E\'").remove("\'F\'");
    if(noLevelStr.toLower() == noLevelStr)
    {
        selectWholeQuery->prepare("PRAGMA case_sensitive_like=0");
    }
    else
    {
        selectWholeQuery->prepare("PRAGMA case_sensitive_like=1");
    }

    if(!selectWholeQuery->exec(sql))
    {
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
    tagListView->getQComboBox()->addItems(*pidList);
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
    //lwContent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mCurRange = new ItemsRange(this,0,0,0,0,0);
    isConnectScroll(true);

    connect(ui->etSearch,SIGNAL(editingFinished()),this,SLOT(beginSearch()));

    connect(ui->cbLevel,SIGNAL(currentIndexChanged(int)),this,SLOT(cbLevelChangeSlot(int)));
    connect(ui->cbMode,SIGNAL(currentIndexChanged(int)),this,SLOT(cbModeChangeSlot(int)));
    connect(ui->lwContent,SIGNAL(currentTextChanged(QString)),this,SLOT(currentTextContentChangedSlot(QString)));
    connect(ui->etFind,SIGNAL(editingFinished()),this,SLOT(initFindList()));

    connect(ui->prev,SIGNAL(clicked(bool)),this,SLOT(gotoPrevFind()));
    connect(ui->next,SIGNAL(clicked(bool)),this,SLOT(gotoNextFind()));
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
    beginSearch();
}

void MainWindow::cbModeChangeSlot(int index)
{
    if(index == DataBaseManager::MODE_FUZZY)
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
        dispContainSearchString();
}

QString MainWindow::getFindSqlString()
{
    return dbManager->getFindSqlString(ui->etFind->text().trimmed());
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
    QString tagString = (tagListView->getWidgetList() != NULL && tagListView->getWidgetList()->currentItem() != NULL)?
                tagListView->getWidgetList()->currentItem()->text():"";
    return dbManager->getSearchSqlString(getQueryMode(), ui->etSearch->text().trimmed(),
                                          tagListView->getQComboBox()->currentText(),
                                          tagString,
                                          ui->cbLevel->currentIndex());
}

void MainWindow::selfVerticalScrollSlot(int value)
{
    Util::logCurTime("内部滑动：value=" + QString::number(value) + ", mLastScrollValue=" + QString::number(mLastScrollValue));
    int direction = 0;
    int step = value - mLastScrollValue;
    //与上一次的值作比较，判断滑动的方向，1表示向下滑动，-1表示向上滑动，0表示未滑动
    if (value > mLastScrollValue) {
        direction = 1;
    } else if (value < mLastScrollValue) {
        direction = -1;
    }

    Util::logCurTime("由" + QString::number(mLastScrollValue) + " >> " + QString::number(value));
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

//绘制当前区域
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

    Util::logCurTime("显示范围：" + QString::number(first) + " ,"
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
            qDebug()<<"向上："<<range->getVisibleFirst() + range->getPageItemNum() - 2 - first;
        } else {
            lwContent->scrollToBottom();
            lwContent->setCurrentRow(range->getVisibleFirst() - first);
            qDebug()<<"向下："<<range->getVisibleFirst() - first;
        }
        //重新设定滚动条的位置
        ui->verticalScrollBar->setValue(range->getVisibleFirst());
    }

    //获取最新的内部滚动条的值，否则外部滚动条将会发生紊乱
    mLastScrollValue = lwContent->verticalScrollBar()->value();

    isConnectScroll(true);

}

//本函数更新当前缓冲区的范围，仅仅在 dispContainSearchString 中调用，在窗口高度变化的时候，应该调用此函数
void MainWindow::updateCurRange()
{
    int visibleHeight = lwContent->height();        //获取内容显示区的总高度
    int itemHeight = tagListView->getWidgetList()->sizeHintForRow(0);
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
    delete mCurRange;
    mCurRange = new ItemsRange(this,first,visibleFirst,count,visibleItemNum,totalCount);
    updateLineAndPage();
}

void MainWindow::verticalScrollSlot(int value)
{
    Util::logCurTime("外部滑动：value=" + QString::number(value));
    int visibleFirst = value;
    int first = (visibleFirst > 0) ? (visibleFirst - 1) : visibleFirst;
    int count = mCurRange->getCount();
    int pageItemNum = mCurRange->getPageItemNum();
    delete mCurRange;
    mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
    dispAreaData(mCurRange,-1);
}

void MainWindow::gotoLine(int line)
{
    qDebug()<<"gotoLine: "<<line;
    int first = mCurRange->getFirst();
    int visibleFirst = mCurRange->getVisibleFirst();
    int count = mCurRange->getCount();
    int pageItemNum = mCurRange->getPageItemNum();
    if(line < 0 || line >= mCurRange->getTotal())
        return;
    isConnectScroll(false);
    visibleFirst = line - 1;
    first = (visibleFirst > 0) ? (visibleFirst - 1) : visibleFirst;
    delete mCurRange;
    mCurRange = new ItemsRange(this,first,visibleFirst,count,pageItemNum,totalCount);
    dispAreaData(mCurRange,0);
    isConnectScroll(true);
}

void MainWindow::dispContainSearchString()
{
    if(totalCount > 0)
    {
        updateCurRange();
        dispCurDataList();
    }
}

//设置滚动条最大值，以及显示和隐藏，应该在每次重新搜索后调用此函数
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
    dispContainSearchString();
    sizeChanged = false;
}

//更新总行数和页数
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

void MainWindow::initPidList()
{
    dbManager->initPidList(pidList);
}

void MainWindow::initTagList(int pid)
{
    dbManager->initTagList(pid,tagListView->getWidgetList());
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
    QString targetFileName = dbManager->loadFiles(fileList);
    loadLogSqliteFile(targetFileName,targetFileName);
}

bool MainWindow::loadLogSqliteFile(const QString& filename,const QString& oname)
{
    dbManager->open(filename);
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
    dbManager->close();
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
