#include "wavexporter.h"
#include <QtDebug>
#include "logging.h"

WavExporter::WavExporter(FileDescriptor * file, const QVector<int> &indexes, int count, QObject *parent)
    : QObject(parent), file(file), indexes(indexes), count(count)
{DD;

}

WavExporter::WavExporter(Channel *channel, QObject *parent):
    QObject(parent), channel(channel)
{DD;
    file = channel->descriptor();
    const int index = channel->index();
    if (index >= 0) indexes << index;
}

WavExporter::~WavExporter()
{DD;

}

void WavExporter::stop()
{DD;
    finalize();
}

void WavExporter::start()
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        LOG(WARNING)<<"WavExporter interrupted";
        finalize();
        return;
    }
    LOG(INFO) << QString("Начало экспорта в WAV для файла ")<<file->fileName();
    LOG(INFO) << QString("  Экспортируются каналы ")<<indexes;

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
        LOG(INFO) << QString("  Экспортирован в файл ") << name;
    }

    finalize();
}

void WavExporter::finalize()
{DD;
    LOG(INFO) << QString("Завершен экспорт в WAV для файла ")<<file->fileName();
    emit finished();
}


