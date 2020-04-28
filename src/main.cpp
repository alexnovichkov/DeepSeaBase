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



//    QFile wav("R4G3.wav");
//    wav.open(QFile::ReadOnly);
//    QDataStream sWav(&wav);
//    sWav.setByteOrder(QDataStream::LittleEndian);
////    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

//    QFile raw("R4G3.raw");
//    raw.open(QFile::WriteOnly);
//    QDataStream sRaw(&raw);
//    sRaw.setByteOrder(QDataStream::LittleEndian);

//    sWav.device()->seek(16*16);
//    QByteArray b= sWav.device()->read(1000*2);

//    sRaw.device()->write(b);

//    DfdFileDescriptor dfd("R4G3.dfd");
//    dfd.fillPreliminary(Descriptor::TimeResponse);
//    dfd.BlockSize = 0;
//    dfd.NumInd = 1000;
//    dfd.XBegin = 0.0;
//    dfd.XName = "с";
//    dfd.XStep = 1.0 / 8192.0;
////    for (int ch=0; ch<4; ++ch) {
//        DfdChannel *c = new DfdChannel(&dfd, 0);
//        c->ChanBlockSize = 1000;
//        c->ChanName = QString("Channel 1");
//        c->IndType = 0x80000002;
//        c->YName = "m/s2";
//        dfd.channels << c;
////    }
//    dfd.setChanged(true);
//    dfd.write();

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
