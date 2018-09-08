#-------------------------------------------------
#
# Project created by QtCreator 2018-08-31T08:49:33
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = alog

TEMPLATE = app


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    itemsrange.cpp \
    taglistview.cpp \
    util.cpp \
    toolbar.cpp \
    loglist.cpp \
    logview.cpp \
    databasemanager.cpp

HEADERS += \
        mainwindow.h \
    itemsrange.h \
    taglistview.h \
    util.h \
    toolbar.h \
    loglist.h \
    logview.h \
    databasemanager.h


FORMS += \
        mainwindow.ui \
    taglistview.ui \
    toolbar.ui \
    loglist.ui \
    logview.ui

DISTFILES += \
    README \
    dark.css \
    buglist


RESOURCES += \
    resource.qrc
