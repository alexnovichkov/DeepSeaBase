#-------------------------------------------------
#
# Project created by QtCreator 2013-08-14T13:18:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = DeepSeaBase
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
        dfdfiledescriptor.cpp \
        qcustomplot.cpp \
    convertdialog.cpp \
    sortabletreewidgetitem.cpp

HEADERS  += mainwindow.h \
        dfdfiledescriptor.h \
        qcustomplot.h \
    convertdialog.h \
    sortabletreewidgetitem.h
