#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QListWidget>
#include <QFileInfo>
#include <QFile>
#include "util.h"
#include <QTime>

int DataBaseManager::MODE_FUZZY = 0;
int DataBaseManager::MODE_NORMAL = 1;
int DataBaseManager::MODE_SQL = 2;


DataBaseManager::DataBaseManager()
{

}

QSqlDatabase *DataBaseManager::getDatabase()
{
    return &sqliteDB;
}

void DataBaseManager::open(const QString& fileName)
{
    if (QSqlDatabase::contains("show_logcat"))
    {
        sqliteDB = QSqlDatabase::database("show_logcat");
    }else
    {
        sqliteDB = QSqlDatabase::addDatabase("QSQLITE","show_logcat");
        sqliteDB.setDatabaseName(fileName);
    }

    if(!sqliteDB.isOpen())
        sqliteDB.open();

}
void DataBaseManager::close()
{
    if(sqliteDB.isOpen())
        sqliteDB.close();
    if (QSqlDatabase::contains("show_logcat"))
    {
        QSqlDatabase::removeDatabase("show_logcat");
    }
}
bool DataBaseManager::isOpen()
{
    return sqliteDB.isOpen();
}

bool DataBaseManager::isTableExist(QString table)
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

QSqlQuery *DataBaseManager::getSelectWholeQuery()
{
    return selectWholeQuery;
}

int DataBaseManager::getCountFromQuery(const QString& table,const QString& where)
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

QString DataBaseManager::getSearchSqlString(int mode,const QString& search,const QString& pidText,const QString& tagText,int level)
{
    QString sql = "";
    if(mode==MODE_NORMAL)
    {
        if(!search.isEmpty())
        {
            QStringList argList = search.split(' ');
            qDebug()<<argList;
            sql = Util::getNormalQueryString(argList);
        }
    }else if(mode==MODE_SQL)
    {
        sql = search;
    }else if(mode==MODE_FUZZY)
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
        queryList.append(Util::getLevelSql(level));
        if(pidText != "All")
        {
            queryList.append(QString("pid=%1").arg(pidText));
        }
        if(tagText != NULL && !tagText.isEmpty())
        {
            int find = tagText.lastIndexOf("(");
            if(find >= 0)
            {
                QString tt = tagText.left(find).trimmed();
                if(!tt.startsWith("All"))
                    queryList.append(QString("tag=\'%1\'").arg(tt));
            }
        }
        sql = queryList.join(" and ");
    }
    qDebug()<<"The search sql: "<<sql;
    return sql;
}

//ui->etFind->text().trimmed()
QString DataBaseManager::getFindSqlString(const QString& find)
{
    QString sql = "";
    if(!find.isEmpty())
    {
        QStringList argList = find.split(' ');
        qDebug()<<argList;
        sql = Util::getNormalQueryString(argList);
    }
    qDebug()<<"The find sql: "<<sql;

    return sql;
}

void DataBaseManager::initTagList(int pid , QListWidget *listWidget)
{
    QString select_tag_sql = pid<0?
                QString("select tag,sum(`count`) from analize group by tag"):
                QString("select tag,sum(`count`) from analize where pid=%1 group by tag").arg(pid);
    QSqlQuery sqlQuery(sqliteDB);
    listWidget->clear();
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
            listWidget->addItem(tempText);
        }
        listWidget->insertItem(0,QString(" All Message(%1)").arg(ccc));
        if(listWidget->count()>0)
        {
            listWidget->setCurrentRow(0);
        }
    }
}

void DataBaseManager::initPidList(QStringList *pidList)
{
    QString select_pid_sql = "select pid from analize group by pid order by pid";
    QSqlQuery sqlQuery(sqliteDB);
    pidList->clear();
    pidList->append("All");
    if(!sqlQuery.exec(select_pid_sql))
    {
        qDebug()<<sqlQuery.lastError();
    }
    else
    {
        while(sqlQuery.next())
        {
            pidList->append(sqlQuery.value(0).toString());
        }
    }
}


QString DataBaseManager::loadFiles(QStringList fileList)
{
    if(fileList.isEmpty())
        return "";
    QFileInfo firstfileInfo(fileList[0]);
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

    return targetFileName;
}

void DataBaseManager::loadFile(const QString& fileName, const QString& sqlFileName)
{
    QFileInfo fileInfo(fileName);
    if(fileInfo.exists())
    {
//        if(hasOpenedFile)
//        {
//            closeFileSlot();
//        }
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

bool DataBaseManager::loadTextFile(const QString& fileName, const QString& sqlFileName)
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
    }

    QTime    tmpTime;
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
//    qDebug()<<"解析数据耗时："<<tmpTime.elapsed()<<"ms"<<endl;
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
    }

    QString insertQuerysql = "insert into analize select pid,tag,count(*) from logcat group by pid,tag order by pid,tag";
    if(!sql_query.exec(insertQuerysql))
    {
        qDebug()<<sql_query.lastError();
        return false;
    }
//    qDebug()<<"解析数据耗时2："<<tmpTime.elapsed()<<"ms"<<endl;
}
