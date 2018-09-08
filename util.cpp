#include "util.h"
#include <QStringList>
#include <QString>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#define DEBUG_SWITCH 1

static int LEVEL_VERBOSE = 0;
static int LEVEL_DEBUG = 1;
static int LEVEL_INFO = 2;
static int LEVEL_WARN = 3;
static int LEVEL_ERROR = 4;
static int LEVEL_FATAL = 5;
Util::Util()
{

}

QString Util::getSqliteQueryStringIs(QString data,const QString& prefix,const QString& colume)
{
    if(!prefix.isEmpty()&&!(data.startsWith(prefix) && !data.endsWith(prefix)))
        return "";

    QString content = data.remove(prefix);
    if(content.contains('|'))
    {
        QStringList listString = content.split('|');
        QStringList queryString;
        foreach (QString var, listString) {
            if(var.startsWith("^"))
            {
                queryString.append(QString("%1!=\'%2\'").arg(colume).arg(var.remove("^")));
            }else
            {
                queryString.append(QString("%1=\'%2\'").arg(colume).arg(var));
            }
        }
        return QString(" ( %1 ) ").arg(queryString.join(" or "));
    }else if(content.startsWith("^"))
    {
        return QString("%1!=\'%2\'").arg(colume).arg(content.remove("^"));
    }
    else
    {
        return QString("%1=\'%2\'").arg(colume).arg(content);
    }

    return "";
}

QString Util::getSqliteQueryStringLike(QString data,const QString& prefix,const QString& colume)
{
    if(!prefix.isEmpty()&&!(data.startsWith(prefix) && !data.endsWith(prefix)))
        return "";

    QString content = data.remove(prefix);
    if(content.contains('|'))
    {
        QStringList listString = content.split('|');
        QStringList queryString;
        foreach (QString var, listString) {
            if(var.startsWith("^"))
            {
                queryString.append(QString("%1 not like \'\%%2\%\'").arg(colume).arg(var.remove("^").replace("\`"," ")));
            }else
            {
                queryString.append(QString("%1 like \'\%%2\%\'").arg(colume).arg(var.replace("\`"," ")));
            }
        }
        return QString(" ( %1 ) ").arg(queryString.join(" or "));
    }else if(content.startsWith("^"))
    {
        return QString("%1 not like \'\%%2\%\'").arg(colume).arg(content.remove("^").replace("\`"," "));
    }
    else
    {
        return QString("%1 like \'\%%2\%\'").arg(colume).arg(content.replace("\`"," "));
    }

    return "";
}

QString Util::getNormalQueryString(QStringList argList)
{
    QStringList queryList;
    foreach (QString arg, argList) {
        if(arg.trimmed().isEmpty())
            continue;
        if(arg.startsWith("tag:") )
        {
            QString tmp = Util::getSqliteQueryStringLike(arg,"tag:","tag");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("t:") )
        {
            QString tmp = Util::getSqliteQueryStringLike(arg,"t:","tag");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("l:") )
        {
            QString tmp = Util::getSqliteQueryStringIs(arg,"l:","level");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("level:") )
        {
            QString tmp = Util::getSqliteQueryStringIs(arg,"level:","level");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("pid:"))
        {
            QString tmp = Util::getSqliteQueryStringIs(arg,"pid:","pid");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("p:"))
        {
            QString tmp = Util::getSqliteQueryStringIs(arg,"p:","pid");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("message:"))
        {
            QString tmp = Util::getSqliteQueryStringLike(arg,"message:","message");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else if(arg.startsWith("m:"))
        {
            QString tmp = Util::getSqliteQueryStringLike(arg,"m:","message");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }else
        {
            QString tmp = Util::getSqliteQueryStringLike(arg,"","message");
            if(!tmp.isEmpty())
                queryList.append(tmp);
        }
    }
    return queryList.join(" and ");
}

int Util::getLevelIndex(QString logLevel)
{
    int levelIndex = LEVEL_VERBOSE;
    if ((logLevel == "V") || (logLevel == "verbose")) {
        levelIndex = LEVEL_VERBOSE;
    } else if (logLevel == "D" || (logLevel == "debug")) {
        levelIndex = LEVEL_DEBUG;
    } else if (logLevel == "I" || (logLevel == "info")) {
        levelIndex = LEVEL_INFO;
    } else if (logLevel == "W" || (logLevel == "warn")) {
        levelIndex = LEVEL_WARN;
    } else if (logLevel == "E" || (logLevel == "error")) {
        levelIndex = LEVEL_ERROR;
    } else if (logLevel == "A" || (logLevel == "assert") || (logLevel == "F")) {
        levelIndex = LEVEL_FATAL;
    }

    return levelIndex;
}

QString Util::getLevelSql(int level)
{
    if (level == LEVEL_VERBOSE) {
        return QString("level in \(\'V\',\'D\',\'I\',\'W\',\'E\',\'F\'\)");
    } else if (level == LEVEL_DEBUG) {
        return QString("level in \(\'D\',\'I\',\'W\',\'E\',\'F\'\)");
    } else if (level == LEVEL_INFO) {
        return QString("level in \(\'I\',\'W\',\'E\',\'F\'\)");
    } else if (level == LEVEL_WARN) {
        return QString("level in \(\'W\',\'E\',\'F\'\)");
    } else if (level == LEVEL_ERROR) {
        return QString("level in \(\'E\',\'F\'\)");
    } else if (level == LEVEL_FATAL){
        return QString("level in \(\'F\'\)");
    }
}

QString Util::formatNumber(int num, int bitNum)
{
    return QString::number(num).rightJustified(bitNum,'0');
}

QString Util::formatPid(QString pid)
{
    return pid.rightJustified(5,' ');
}

QString Util::formatTag(QString tag)
{
    if(tag == NULL || tag.isEmpty())
        tag = "";
    if(tag.length() <= 25)
    {
        return tag.leftJustified(25,' ');
    }else
    {
        return tag.left(12)+"."+tag.right(12);
    }
}

QByteArray Util::fetchQrc(const QString &fileName)
{
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    return file.readAll();
}

void Util::logCurTime(QString text)
{
#if DEBUG_SWITCH
    qDebug()<<"【" + QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "】" + text;
#endif
}
