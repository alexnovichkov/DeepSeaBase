#include "timeslicer.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "methods/calculations.h"

#include <QProgressDialog>

TimeSlicer::TimeSlicer(const QVector<FileDescriptor *> &files, double from, double to, QObject *parent)
    : QObject(parent), dataBase(files), from(from), to(to)
{DDD;
    stop_ = false;
}

TimeSlicer::~TimeSlicer()
{DDD;

}

void TimeSlicer::stop()
{DDD;
    stop_ = true;
    finalize();
}

void TimeSlicer::start()
{DDD;
    //dt = QDateTime::currentDateTime();
    //qDebug()<<"Start converting"<<dt.time();

//    QProgressDialog progress("Сохранение вырезки...", "Отменить сохранение", 0, dataBase.size(), this);
//    progress.setWindowModality(Qt::WindowModal);

    int i=0;
    for (FileDescriptor *file: qAsConst(dataBase)) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            finalize();
            return;
        }
//        progress.setValue(i);
//        if (progress.wasCanceled()) {
//            break;
//        }
        newFiles << saveTimeSegment(file, from, to);
        i++;
        emit tick(i);
    }
//    progress.setValue(dataBase.size());
    //addFiles(newFiles);

    finalize();
}

void TimeSlicer::finalize()
{DDD;
    emit finished();
}
