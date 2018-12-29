#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    QVector<double> vals(32768);
//    vals[0]=0.0;
//    vals[1]=1.0;
//    vals[2]=2.0;

//    QFile f("data.dat");
//    QDataStream stream(&f);
//    stream.setByteOrder(QDataStream::LittleEndian);
//    f.open(QFile::WriteOnly);
//    stream << vals;
//    f.close();

//    QFile f1("data1.dat");
//    QDataStream stream1(&f1);
//    stream1.setByteOrder(QDataStream::LittleEndian);
//    f1.open(QFile::WriteOnly);
//    stream1 << vals.size();
//    stream1 << vals[0];
//    stream1 << vals[1];
//    stream1 << vals[2];
//    f1.close();

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
