#ifndef UTIL_H
#define UTIL_H
#include <QString>

class Util
{
public:
    Util();
    static QString getSqliteQueryStringIs(QString content,const QString& prefix,const QString& colume);
    static QString getSqliteQueryStringLike(QString content,const QString& prefix,const QString& colume);


    static QString getNormalQueryString(QStringList argList);

    static QString formatNumber(int num, int bitNum);
    static QString formatPid(QString pid);
    static QString formatTag(QString tag);
    static QByteArray fetchQrc(const QString &fileName);
    static QString getLevelSql(int level);
    static int getLevelIndex(QString logLevel);
};

#endif // UTIL_H
