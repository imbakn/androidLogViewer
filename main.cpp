#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QTime>
#include <QSqlRecord>
#include <QRegExp>
#include <QFile>
#include <QMessageBox>
#include "util.h"
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(QLatin1String("fusion"));
    a.setStyleSheet(QString::fromUtf8(Util::fetchQrc(QLatin1String(":/dark.css"))));
    QFont font = QFont("Ubuntu Mono",14,QFont::Normal);
    a.setFont(font);
    QStringList flist;
    if(argc > 1)
    {
        for(int index = 1;index < argc;index++)
        {
            QString path = QString().sprintf("%s",argv[index]);
            QFileInfo finfo(path);
            if(finfo.exists())
            {
                flist.append(path);
            }
        }

    }


    MainWindow w(NULL,flist);
    w.show();

    return a.exec();
}
