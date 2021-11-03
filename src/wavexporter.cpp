#include "wavexporter.h"
#include "algorithms.h"
#include "fileformats/filedescriptor.h"
#include <QtDebug>
#include <QDataStream>
#include "logging.h"

#define BLOCK_SIZE 20000



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

int WavExporter::chunksCount() const
{DD;
    int blocks = file->samplesCount()/BLOCK_SIZE;
    if (file->samplesCount() % BLOCK_SIZE != 0) blocks++;
    return blocks;
}

//int WavExporter::chunksCount() const
//{DD;
//    return indexes.size();
//}

void WavExporter::stop()
{DD;
    finalize();
}

void WavExporter::writeWithStreams(const QVector<int> &v, const QString &wavFileName)
{DD;
    if (v.isEmpty()) return;

    QFile wavFile(wavFileName);
    if (!wavFile.open(QFile::WriteOnly)) {
        qCritical()<<"Не удалось открыть файл"<<wavFileName;
        finalize();
        return;
    }

    QDataStream s(&wavFile);
    s.setByteOrder(QDataStream::LittleEndian);

    //Определяем общий размер файла Wav
    const quint16 M = format == WavFloat ? 4 : 2; // bytesForFormat = short / float
    const quint16 channelsCount = quint16(v.size()); //Fc
    const quint32 sampleRate = (quint32)qRound(1.0 / file->xStep()); //F
    const quint32 samples = quint32(file->samplesCount());
    const quint32 headerSize = format==WavPCM ? sizeof(SimpleWavHeader) : sizeof(WavHeader);
    //const quint32 totalSize = sizeof(WavHeader) + M * channelsCount * samples;

    //Создаем заголовок файла
    if (format == WavPCM) {
        SimpleWavHeader header = initSimpleHeader(channelsCount, samples, sampleRate, format);

        s << header.ckID;
        s << header.cksize;
        s << header.waveId;
        s << header.fmtId;
        s << header.fmtSize;
        s << header.wFormatTag;
        s << header.nChannels;
        s << header.samplesPerSec;
        s << header.bytesPerSec;
        s << header.blockAlign;
        s << header.bitsPerSample;
        s << header.dataId;
        s << header.dataSize;
    }
    else {
        WavHeader header = initHeader(channelsCount, samples, sampleRate, format);

        s << header.ckID;
        s << header.cksize;
        s << header.waveId;
        s << header.fmtId;
        s << header.fmtSize;
        s << header.wFormatTag;
        s << header.nChannels;
        s << header.samplesPerSec;
        s << header.bytesPerSec;
        s << header.blockAlign;
        s << header.bitsPerSample;
        s << header.cbSize;
        s << header.wValidBitsPerSample;
        s << header.dwChannelMask;
        s.writeRawData(header.subFormat, 16);
        s << header.factID;
        s << header.factSize;
        s << header.dwSampleLength;
        s << header.dataId;
        s << header.dataSize;
    }


    //пишем блоками по BLOCK_SIZE отсчетов
    int count = chunksCount();
    emit chunksCountChanged(count);
    for (int chunk = 0; chunk < count; ++chunk) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            wavFile.close();
            wavFile.remove();
            qDebug()<<"Сохранение файла wav прервано";
            finalize();
            return;
        }

        //мы должны сформировать блоки с каждого канала.
        //Так как читать сразу все каналы в память слишком объемно,
        //1. заполняем канал
        //2. берем из него BLOCK_SIZE отсчетов
        //3. из всех каналов формируем колбасу для записи
        //4. очищаем данные канала

        QVector<QByteArray> chunkData;
        for (const int c: qAsConst(v)) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                wavFile.close();
                qDebug()<<"Сохранение файла wav прервано";
                finalize();
                return;
            }

            Channel *channel = file->channel(c);
            bool populated = channel->populated();
            if (!populated) channel->populate();
            auto data = channel->wavData(chunk * BLOCK_SIZE, BLOCK_SIZE, format==WavFloat?2:1);
            chunkData.append(data);
            if (!populated) channel->clear();
        }

        //теперь перетасовываем chunkData - берем из него по одному отсчету каждого канала
        //и записываем в wav
        for (int i = 0; i < chunkData.constFirst().size()/M; ++i) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                wavFile.close();
                wavFile.remove();
                qDebug()<<"Сохранение файла wav прервано";
                finalize();
                return;
            }

            for (const QByteArray &c: qAsConst(chunkData)) {
                s.device()->write(c.mid(i*M, M));
            }
        }
        emit tick();
    }
    wavFile.close();
}

bool WavExporter::writeWithMap(const QVector<int> &v, const QString &wavFileName)
{DD;
    if (v.isEmpty()) return false;

    //2. Записываем заголовок файла wav
    QFile wavFile(wavFileName);
    if (!wavFile.open(QFile::WriteOnly)) {
        qCritical()<<"Не удалось открыть файл"<<wavFileName;
        //finalize();
        return false;
    }

    //Определяем общий размер файла Wav
    const quint16 M = format == WavFloat ? 4 : 2; // bytesForFormat = short / float
    const quint16 channelsCount = quint16(v.size()); //Fc
    const quint32 sampleRate = (quint32)qRound(1.0 / file->xStep()); //F
    const quint32 samples = quint32(file->samplesCount());
    const quint32 headerSize = format==WavPCM ? sizeof(SimpleWavHeader) : sizeof(WavHeader);
    const quint32 totalSize =  headerSize + M * channelsCount * samples;

    //Создаем пустой файл
    wavFile.write(QByteArray(totalSize, 0x0));
    wavFile.close();

    //Переоткрываем файл для маппинга
    wavFile.open(QFile::ReadWrite);
    uchar *mapped = wavFile.map(0, totalSize);
    if (!mapped) {
        qCritical()<<"Не удалось создать файл нужного размера";
        wavFile.close();
        return false;
    }

    //Создаем заголовок файла
    if (format==WavPCM) {
        SimpleWavHeader header = initSimpleHeader(channelsCount, samples, sampleRate, format);
        memcpy(mapped, &header, sizeof(SimpleWavHeader));
    }
    else {
        WavHeader header = initHeader(channelsCount, samples, sampleRate, format);
        memcpy(mapped, &header, sizeof(WavHeader));
    }

    for (int ch = 0; ch < channelsCount; ++ch) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            wavFile.close();
            wavFile.remove();
            qDebug()<<"Сохранение файла wav прервано";
            finalize();
            return true;
        }

        Channel *channel = file->channel(v.at(ch));
        bool populated = channel->populated();
        if (!populated) channel->populate();
        QByteArray channelData = channel->wavData(0, channel->data()->samplesCount(), format==WavFloat?2:1);

        //записываем каждый сэмпл на свое место
        for (int sample = 0; sample < channel->data()->samplesCount(); ++sample) {
            // i-й отсчет n-го канала имеет номер
            //       [n + i*ChannelsCount]
            //то есть целевой указатель будет иметь адрес
            //       sizeof(WavHeader) + (n+i*ChannelsCount)*M
            int offset = headerSize + (ch + sample*channelsCount)*M;
            memcpy(mapped + offset, reinterpret_cast<void*>(channelData.data() + sample*M), M);
        }

        if (!populated) channel->clear();
        emit tick();
    }

    wavFile.close();
    return true;
}

void WavExporter::start()
{DD;
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
        if (!writeWithMap(list, name))
            writeWithStreams(list, name);
    }

    finalize();
}

WavHeader WavExporter::initHeader(int channelsCount, int samplesCount, int sampleRate, WavFormat format)
{DD;
    const int M = format==WavFloat?4:2;
    WavHeader header;
    header.cksize = sizeof(WavHeader) + channelsCount*samplesCount*M - 8;

    //fmt block - 48 bytes
    header.nChannels = quint16(channelsCount); //Nc
    header.samplesPerSec = quint32(sampleRate); //F
    header.bytesPerSec = quint32(header.samplesPerSec * channelsCount * M); //F*M*Nc
    header.blockAlign = quint16(channelsCount * M); //M*Nc, data block size, bytes
    header.bitsPerSample = 8*M; //rounds up to 8*M
    header.wValidBitsPerSample = header.bitsPerSample; //используем все биты, для формата 24-bit нужно будет менять

    //subFormat is PCM by default
    if (format == WavFloat)
        header.subFormat[0] = 3; // float

    //fact block - 12 bytes
    header.dwSampleLength = samplesCount; // Nc*Ns, number of samples

    //data block - 8 + M*Nc*Ns bytes
    header.dataSize = M*channelsCount*samplesCount; //M*Nc*Ns

    return header;
}

SimpleWavHeader WavExporter::initSimpleHeader(int channelsCount, int samplesCount, int sampleRate, WavFormat format)
{DD;
    Q_UNUSED(format);
    const int M = 2;
    SimpleWavHeader header;
    header.cksize = sizeof(SimpleWavHeader) + channelsCount*samplesCount*M - 8;

    //fmt block - 20 bytes
    header.nChannels = quint16(channelsCount); //Nc
    header.samplesPerSec = quint32(sampleRate); //F
    header.bytesPerSec = quint32(header.samplesPerSec * channelsCount * M); //F*M*Nc
    header.blockAlign = quint16(channelsCount * M); //M*Nc, data block size, bytes
    header.bitsPerSample = 8*M; //rounds up to 8*M

    //data block - 8 + M*Nc*Ns bytes
    header.dataSize = M*channelsCount*samplesCount; //M*Nc*Ns

    return header;
}

void WavExporter::finalize()
{DD;
    emit finished();
}
