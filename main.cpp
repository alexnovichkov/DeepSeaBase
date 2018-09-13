#include "mainwindow.h"
#include <QApplication>

#include <QtDebug>

#include "matlabfiledescriptor.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
