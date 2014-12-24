#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    short i=1;
//    qDebug()<<i;
//    i=-1;
//    qDebug()<<i;
//    qDebug()<<sizeof(short) << sizeof(quint16);

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
