#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //QTextCodec *utfcodec = QTextCodec::codecForName("UTF-8");
//    QTextCodec::setCodecForTr(utfcodec);
//    QTextCodec::setCodecForCStrings(utfcodec);
//    QTextCodec::setCodecForLocale(utfcodec);

    MainWindow w;

    return a.exec();
}
