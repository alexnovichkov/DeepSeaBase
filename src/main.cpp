#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>

#include "fileformats/matlabfiledescriptor.h"

#include "fileformats/tdmsfile.h"
#include "fileformats/dfdfiledescriptor.h"

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



//    QFile f("IndType=c0000008.raw");
//    f.open(QFile::WriteOnly);
//    QDataStream s(&f);
//    s.setByteOrder(QDataStream::LittleEndian);
////    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

//        for (int ch=0; ch<4;++ch) {
//            for(int i=0; i<10000000; ++i) {
//            double val = double(i);
//            s << val;
//        }
//    }
//    DfdFileDescriptor dfd("IndType=c0000008.dfd");
//    dfd.fillPreliminary(Descriptor::TimeResponse);
//    dfd.BlockSize = 0;
//    dfd.NumInd = 10000000;
//    dfd.XBegin = 0.0;
//    dfd.XName = "с";
//    dfd.XStep = 1.0;
//    for (int ch=0; ch<4; ++ch) {
//        DfdChannel *c = new DfdChannel(&dfd, ch);
//        c->ChanBlockSize = 10000000;
//        c->ChanName = QString("Channel %1").arg(ch+1);
//        c->IndType = 0xc0000008;
//        c->YName = "m/s2";
//        dfd.channels << c;
//    }
//    dfd.setChanged(true);
//    dfd.write();

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
