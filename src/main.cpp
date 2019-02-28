#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    qDebug()<<(pow(1.1, 0.0));
//    qDebug()<<(pow(1.1, 1.0));
//    qDebug()<<(pow(-1.1, 0.0));
//    qDebug()<<(pow(-1.1, -1.0));

//    qDebug()<<(pow(0.0, 0.0));
//    qDebug()<<(pow(0.0, 1.0));
//    qDebug()<<(pow(0.00001, -1.01));
//    qDebug()<<(pow(-0.0, -0.0));

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
