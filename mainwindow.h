#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "itemsrange.h"
#include <QListWidget>
#include <QSqlQuery>
#include <QLabel>
#include "taglistview.h"
#include <QPushButton>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent,QStringList flist);
    ~MainWindow();

private slots:
    void showOrHideProjectWindow();
    void showOrHideFilterWindow();
    void showOrHideConsoleWindow();
    void beginSearch();
    void delaySearch();
    void verticalScrollSlot(int value);
    void selfVerticalScrollSlot(int value);
    void changeFullScreenSlot();
    void showMenuSlot();
    void cbLevelChangeSlot(int index);
    void cbModeChangeSlot(int index);
    void updateLineAndPage();
    void updateByLine();
    void updateByPage();
    void currentTextContentChangedSlot(QString text);
    void tagListViewComboChangedSlot(QString text);
    void loadFileSlot();
    void loadFile(const QString& file,const QString& sqlFileName="");
    bool closeFileSlot();
    void clipCurrentPage();
    void clipCurrentLine();
    QString getSearchSqlString();
    void gotoFirstPageSlot();

    void gotoLastPageSlot();

    void gotoPrePageSlot();

    void gotoNextPageSlot();

    void reDispAreaData();

    void initFindList();
    void gotoPrevFind();
    void gotoNextFind();
protected:
    void resizeEvent(QResizeEvent * event);
private:
    //init function
    void InitWindow();
    void commonInit();
    void createDock();
    void createActionAndMenu();
    void createToolBar();
    void createStatusBar();
    void initTagListDock();
    bool isTableExist(QString table);
    //help function
    int getFilterLevel();
    int getQueryMode();

    void showOrHideWindow(QWidget* widget);

    void logCurTime(QString text);
    void isConnectScroll(bool isConnect);
    void connectUpdateLineSlot(bool flag);

    //main ui
    Ui::MainWindow *ui;
    QDockWidget *tagListDock;
    QDockWidget *filterDock;
    QDockWidget *consoleDock;
    QListWidget *lwContent;
    TagListView * tagListView;
    QPushButton *toggleProject;

    //main function
    void dispAreaData(ItemsRange *range, int direction);
    void updateCurRange();
    void dispContainSearchString();
    void dispCurDataList();
    void gotoLine(int line);
    int binarySearch(int key);
    //sql function
    int getCountFromQuery(const QString& table,const QString& where);
    bool execSql(const QString& table,const QString& where);
    bool initSql(const QString& table,const QString& where);

    //others
    ItemsRange *mCurRange;      //当前显示缓冲区的范围
    int mLastScrollValue;
    QList<QColor> *mColorConfig;//全局日志等级颜色表
    bool isMenuBarShow;

    //actions and menus
    bool hasOpenedFile;

    //statusbar
    QLabel *lineTotalLabel;
    QLabel *pageTotalLabel;
    QLineEdit *lineCurrentEdit;
    QLineEdit *pageCurrentEdit;

    // values
    int totalCount;
    int totalPage;
    int currentLine;
    QString currentLineText;
    int currentPage;
    QStringList pidList;
    void initPidList();
    QList< QPair<QString,int> > * getTagList(const QString& where);
    void initTagList(int pid);
    QString lastSql;
    bool sizeChanged;
    QTimer *sTimer;

    QSqlDatabase sqliteDB;
    QSqlQuery *selectWholeQuery;

    bool loadTextFile(const QString& filename, const QString& sqlFileName="");

    bool loadLogSqliteFile(const QString& filename,const QString& oname);

    void loadFiles(QStringList fileList);

    QString getFindSqlString();
    QString getSearchAndFindSqlString();

    QList<int> idListToSearch;
    QList<int> idListToFind;
    int currentFindIdIndex;
    QString lastFindWhere;
    void gotoFindLine(int index);



};

#endif // MAINWINDOW_H
