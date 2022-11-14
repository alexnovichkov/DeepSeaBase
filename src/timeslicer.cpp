#include "timeslicer.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "methods/calculations.h"

#include <QProgressDialog>

TimeSlicer::TimeSlicer(const QVector<FileDescriptor *> &files, double from, double to, QObject *parent)
    : QObject(parent), dataBase(files), from(from), to(to)
{DD;
    stop_ = false;
}

TimeSlicer::~TimeSlicer()
{DD;

}

void TimeSlicer::stop()
{DD;
    stop_ = true;
    finalize();
}

void TimeSlicer::start()
{DD;
    LOG(INFO) << QString("Начало сохранения временной вырезки");
    int i=0;
    for (FileDescriptor *file: qAsConst(dataBase)) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            finalize();
            LOG(INFO)<<QString("Сохранение прервано");
            return;
        }
        LOG(INFO)<<QString("  Сохранение файла ")<<file->fileName();

        newFiles << saveTimeSegment(file, from, to);
        i++;
        emit tick(i);
        LOG(INFO)<<QString("  Новый файл: ")<<newFiles.last();
    }
    LOG(INFO) << QString("Сохранение временной вырезки завершено");
    finalize();
}

void TimeSlicer::finalize()
{DD;
    emit finished();
}
