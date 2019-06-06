#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>

#include "matlabfiledescriptor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    MatFile f("K:\\Shared\\Rec_001_single.prec_not.group_MKS_bigger.mat");

    //MatFile f("K:\\Shared\\Rec_001_single.prec_group_MKS_not.bigger.mat");
    //MatFile f("K:\\Shared\\rec001_with_rus.mat");
//    MatFile f("K:\\Shared\\rec_001_M_group.mat");

    //return 0;


//    qDebug()<<( 0x020004 >> 16);
//    qDebug()<<( 0x020004 % 65536);
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
