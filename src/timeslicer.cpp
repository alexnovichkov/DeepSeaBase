#include "timeslicer.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"

#include <QProgressDialog>

TimeSlicer::TimeSlicer(const QList<FileDescriptor *> &files, double from, double to, QObject *parent)
    : QObject(parent), dataBase(files), from(from), to(to)
{
    stop_ = false;
}

TimeSlicer::~TimeSlicer()
{

}

void TimeSlicer::stop()
{
    stop_ = true;
    finalize();
}

void TimeSlicer::start()
{
    //dt = QDateTime::currentDateTime();
    //qDebug()<<"Start converting"<<dt.time();

//    QProgressDialog progress("Сохранение вырезки...", "Отменить сохранение", 0, dataBase.size(), this);
//    progress.setWindowModality(Qt::WindowModal);

    int i=0;
    for (FileDescriptor *file: dataBase) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            finalize();
            return;
        }
//        progress.setValue(i);
//        if (progress.wasCanceled()) {
//            break;
//        }
        newFiles << file->saveTimeSegment(from, to);
        i++;
        emit tick(i);
    }
//    progress.setValue(dataBase.size());
    //addFiles(newFiles);

    finalize();
}

void TimeSlicer::finalize()
{
    emit finished();
}
