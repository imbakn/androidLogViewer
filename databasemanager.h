#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QSqlDatabase>

class QListWidget;

class DataBaseManager
{
public:
    static int MODE_FUZZY;
    static int MODE_NORMAL;
    static int MODE_SQL;

    DataBaseManager();
    QSqlDatabase *getDatabase();

    void open(const QString& fileName);
    void close();
    bool isOpen();

    QSqlQuery *getSelectWholeQuery();

    bool isTableExist(QString table);
    int getCountFromQuery(const QString& table,const QString& where);
    QString getSearchSqlString(int mode,const QString& search,const QString& pidText,const QString& tagText,int level);
    QString getFindSqlString(const QString& find);

    void initTagList(int pid, QListWidget *listWidget);
    void initPidList(QStringList *pidList);

    bool loadTextFile(const QString& fileName, const QString& sqlFileName);
    void loadFile(const QString& fileName, const QString& sqlFileName);
    QString loadFiles(QStringList fileList);

private:
    QSqlQuery *selectWholeQuery;
    QSqlDatabase sqliteDB;
};

#endif // DATABASEMANAGER_H
