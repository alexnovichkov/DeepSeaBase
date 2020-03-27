#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>

#include "fileformats/matlabfiledescriptor.h"

#include "fileformats/tdmsfile.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    MatFile f("K:\\Shared\\Rec_001_single.prec_not.group_MKS_bigger.mat");

    //MatFile f("K:\\Shared\\Rec_001_single.prec_group_MKS_not.bigger.mat");
//    MatFile f("K:\\Shared\\Record_8_580_1200s.mat");
//    MatFile f("K:\\Shared\\rec_001_M_group.mat");

//    TDMSFile f("K:\\NI\\R2_36_ob_16_11_2019_180937\\Acceleration.tdms");

//    QFile f("K:\\NI\\R2_36_ob_16_11_2019_180937\\1.txt");
//    f.open(QFile::ReadOnly);
//    //QString s("Р2 36 об");
//    QByteArray b = f.read(30);
//    QString s = QString::fromUtf8(b.data(), b.size());
//    qDebug()<<s;
////    f.write(s.toUtf8());
//    f.close();
//    return 0;


//    qDebug()<<( 1L << 1);
//    qDebug()<<( 1L << 2);
//    qDebug()<<(1L<<3);
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
