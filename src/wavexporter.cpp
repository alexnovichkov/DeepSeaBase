#include "wavexporter.h"
#include <QtDebug>
#include "logging.h"

WavExporter::WavExporter(FileDescriptor * file, const QVector<int> &indexes, int count, QObject *parent)
    : QObject(parent), file(file), indexes(indexes), count(count)
{DDD;

}

WavExporter::WavExporter(Channel *channel, QObject *parent):
    QObject(parent), channel(channel)
{DDD;
    file = channel->descriptor();
    const int index = channel->index();
    if (index >= 0) indexes << index;
}

WavExporter::~WavExporter()
{DDD;

}

void WavExporter::stop()
{DDD;
    finalize();
}

void WavExporter::start()
{DDD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        qDebug()<<"interrupted";
        finalize();
        return;
    }

    auto pool = indexes;
    while (!pool.isEmpty()) {
        //определяем список индексов каналов для записи
        QVector<int> list;
        for (int i=0; i<count && !pool.isEmpty(); ++i)
            list << pool.takeFirst();

        //определяем суффикс имени файла
        QString nameFragment;
        if (count==1) nameFragment = file->channel(list.first())->name();
        else nameFragment = QString("%1-%2").arg(list.first()+1).arg(list.last()+1);
        nameFragment = replaceWinChars(nameFragment);

        //определяем имя файла wav
        QString name;

        if (!_wavFile.isEmpty()) name = _wavFile;
        else
            name = createUniqueFileName("", file->fileName(),
                                            nameFragment, "wav", false);

        WavFile f(*file, name, list, format);
    }

    finalize();
}

void WavExporter::finalize()
{DDD;
    emit finished();
}


