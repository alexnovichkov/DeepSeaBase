#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>
#include <QtCore>
#include <QSplashScreen>

#include "converters/matlabconvertor.h"

#include "fileformats/tdmsfile.h"
#include "fileformats/ufffile.h"
#include "fileformats/matfile.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPixmap pixmap(":/icons/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    a.processEvents();

//    MatFile f("K:\\Shared\\R3G1-oct.mat");
//    f.read();

//    QFile ff("K:\\Shared\\R3G1-oct.json");
//    ff.open(QFile::WriteOnly);
//    ff.write(f.toJson());
//    return 0;


//    UffFileDescriptor uff("K:\\MyDeepSeaData\\Винт 160\\02_Вода_Лопасть_В2_бш_2кГц_RSMPL_000.uff");
//    uff.read();
//    return 0;



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
    splash.finish(&w);
    return a.exec();
}
