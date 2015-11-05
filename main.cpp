#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>

#include "matlabfiledescriptor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    short i=1;
//    qDebug()<<i;
//    i=-1;
//    qDebug()<<1 << (1<<0) << (1<<1) << (1 << 2);
//    qDebug()<<sizeof(short) << sizeof(quint16);

//    MatlabFile f("K://2015_09_07_R1aG1 (2).mat");
//    f.read();
//return 0;
    MainWindow w;
    w.showMaximized();
    return a.exec();
}
