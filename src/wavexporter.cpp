#include "wavexporter.h"
#include "algorithms.h"
#include "fileformats/filedescriptor.h"
#include <QtDebug>
#include <QDataStream>
#include "logging.h"

#define BLOCK_SIZE 20000



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

int WavExporter::chunksCount() const
{DDD;
    int blocks = file->samplesCount()/BLOCK_SIZE;
    if (file->samplesCount() % BLOCK_SIZE != 0) blocks++;
    return blocks;
}

//int WavExporter::chunksCount() const
//{DDD;
//    return indexes.size();
//}

void WavExporter::stop()
{DDD;
    finalize();
}

void WavExporter::writeWithStreams(const QVector<int> &v, const QString &wavFileName)
{DDD;
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
    const quint16 M = format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float
    const quint16 channelsCount = quint16(v.size()); //Fc
    const quint32 sampleRate = (quint32)qRound(1.0 / file->xStep()); //F
    const quint32 samples = quint32(file->samplesCount());

    //Создаем заголовок файла
    {
        WavHeader header = initHeader(channelsCount, samples, sampleRate, format);
        s << header.ckID;
        s << header.cksize;
        s << header.waveId;
    }
    {
        auto header = initFmt(channelsCount, samples, sampleRate, format);
        s << header.fmtId;
        s << header.fmtSize;
        s << header.wFormatTag;
        s << header.nChannels;
        s << header.samplesPerSec;
        s << header.bytesPerSec;
        s << header.blockAlign;
        s << header.bitsPerSample;
        if (format != WavFormat::WavPCM) {
            s << header.cbSize;
            s << header.wValidBitsPerSample;
            s << header.dwChannelMask;
            s << header.subFormat.data1;
            s << header.subFormat.data2;
            s << header.subFormat.data3;
            for (int i=0; i<8; ++i) s << header.subFormat.data4[i];

            auto fact = initFact(channelsCount, samples, sampleRate, format);
            s << fact.factID;
            s << fact.factSize;
            s << fact.dwSampleLength;
        }
    }
    {
        WavChunkCue header = initCue(channelsCount, samples, format);
        s << header.cueId;
        s << header.cueSize;
        s << header.dwCuePoints;
        for (auto c: header.cues) {
            s << c.identifier << c.order << c.chunkID << c.chunkStart << c.blockStart << c.offset;
        }
    }
    {
        auto assocFile = initFile(v);
        s << assocFile.fileId << assocFile.fileSize;
        s << assocFile.dwName << assocFile.dwMedType;
        s.writeRawData(assocFile.data.data(), assocFile.data.size());
    }
    {
        WavChunkData header = initDataHeader(channelsCount, samples, format);
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
            auto data = channel->wavData(chunk * BLOCK_SIZE, BLOCK_SIZE, format==WavFormat::WavFloat?2:1);
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
{DDD;
    if (v.isEmpty()) return false;

    //2. Записываем заголовок файла wav
    QFile wavFile(wavFileName);
    if (!wavFile.open(QFile::WriteOnly)) {
        qCritical()<<"Не удалось открыть файл"<<wavFileName;
        //finalize();
        return false;
    }

    //Определяем общий размер файла Wav
    const quint16 M = format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float
    const quint16 channelsCount = quint16(v.size()); //Fc
    const quint32 sampleRate = (quint32)qRound(1.0 / file->xStep()); //F
    const quint32 samples = quint32(file->samplesCount());
    quint32 totalSize =  sizeof(WavHeader) + 24 + M * channelsCount * samples;
    if (format != WavFormat::WavPCM) totalSize += 8 + sizeof(ExtensibleWavSubFormat);

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
    WavHeader header = initHeader(channelsCount, samples, sampleRate, format);
    memcpy(mapped, &header, sizeof(WavHeader));
    mapped += sizeof(WavHeader);

    auto fmt = initFmt(channelsCount, samples, sampleRate, format);
    if (format == WavFormat::WavPCM) memcpy(mapped, &fmt, 24);
    else memcpy(mapped, &fmt, sizeof(fmt));
    mapped += format == WavFormat::WavPCM ? 24 : sizeof(fmt);

    if (format!=WavFormat::WavPCM) {
        auto fact = initFact(channelsCount, samples, sampleRate, format);
        memcpy(mapped, &fact, sizeof(fact));
        mapped += sizeof(fact);
    }

    WavChunkCue cue = initCue(channelsCount, samples, format);
    memcpy(mapped, &cue.cueId, 4); mapped +=4;
    memcpy(mapped, &cue.cueSize, 4); mapped +=4;
    memcpy(mapped, &cue.dwCuePoints, 4); mapped +=4;
    for (auto c: cue.cues) {
        memcpy(mapped, &c, sizeof(c)); mapped += sizeof(c);
    }

    auto assocFile = initFile(v);
    memcpy(mapped, &assocFile.fileId, 4); mapped += 4;
    memcpy(mapped, &assocFile.fileSize, 4); mapped += 4;
    memcpy(mapped, &assocFile.dwName, 4); mapped += 4;
    memcpy(mapped, &assocFile.dwMedType, 4); mapped += 4;
    memcpy(mapped, assocFile.data.data(), assocFile.data.size()); mapped += assocFile.data.size();

    auto data = initDataHeader(channelsCount, samples, format);
    memcpy(mapped, &data, sizeof(data));
    mapped += sizeof(data);

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
        QByteArray channelData = channel->wavData(0, channel->data()->samplesCount(), format==WavFormat::WavFloat?2:1);

        //записываем каждый сэмпл на свое место
        for (int sample = 0; sample < channel->data()->samplesCount(); ++sample) {
            // i-й отсчет n-го канала имеет номер
            //       [n + i*ChannelsCount]
            //то есть целевой указатель будет иметь адрес
            //       sizeof(WavHeader) + (n+i*ChannelsCount)*M
            int offset = (ch + sample*channelsCount)*M;
            memcpy(mapped + offset, reinterpret_cast<void*>(channelData.data() + sample*M), M);
        }

        if (!populated) channel->clear();
        emit tick();
    }

    wavFile.close();
    return true;
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
        if (!writeWithMap(list, name))
            writeWithStreams(list, name);
    }

    finalize();
}

WavHeader WavExporter::initHeader(int channelsCount, int samplesCount, int sampleRate, WavFormat format)
{DDD;
    const int M = format==WavFormat::WavFloat?4:2;
    WavHeader header;
    header.cksize = (format==WavFormat::WavPCM?36:72) + channelsCount*samplesCount*M;

    return header;
}

WavChunkFmt WavExporter::initFmt(int channelsCount, int samplesCount, int sampleRate, WavFormat format)
{
    const int M = format==WavFormat::WavFloat?4:2;
    WavChunkFmt header;

    header.fmtSize = (format==WavFormat::WavPCM?16:40);
    header.wFormatTag = format==WavFormat::WavPCM?1:0xfffe;
    header.nChannels = quint16(channelsCount); //Nc
    header.samplesPerSec = quint32(sampleRate); //F
    header.bytesPerSec = quint32(header.samplesPerSec * channelsCount * M); //F*M*Nc
    header.blockAlign = quint16(channelsCount * M); //M*Nc, data block size, bytes
    header.bitsPerSample = 8*M; //rounds up to 8*M

    if (header.fmtSize == 40) {
        header.wValidBitsPerSample = 8*M; //используем все биты, для формата 24-bit нужно будет менять
//        header.dwChannelMask = juce::AudioChannelSet::discreteChannels(channelsCount).getWaveChannelMask();
        qDebug() << header.dwChannelMask;

        //subFormat is PCM by default
        if (format == WavFormat::WavFloat)
            header.subFormat = IEEEFloatFormat;
    }
    return header;
}

WavChunkFact WavExporter::initFact(int channelsCount, int samplesCount, int sampleRate, WavFormat format)
{
    WavChunkFact header;
    header.factSize = 4;
    header.dwSampleLength = samplesCount; // Nc*Ns, number of samples
    return header;
}

WavChunkData WavExporter::initDataHeader(int channelsCount, int samplesCount, WavFormat format)
{
    const int M = format==WavFormat::WavFloat?4:2;
    WavChunkData header;
    //data block - 8 + M*Nc*Ns bytes
    header.dataSize = M*channelsCount*samplesCount; //M*Nc*Ns
    return header;
}

WavChunkCue WavExporter::initCue(int channelsCount, int samplesCount, WavFormat format)
{
    WavChunkCue cue;
    cue.dwCuePoints = 1;
    cue.cueSize = sizeof (WavChunkCue::Cue) + 4;
    WavChunkCue::Cue c;
    cue.cues.append(c);

    return cue;
}

WavChunkFile WavExporter::initFile(const QVector<int> &v)
{
    WavChunkFile chunk;
    QJsonArray array;
    for (auto index: v)
        array.append(file->channel(index)->dataDescription().toJson());
    QJsonObject o;
    o.insert("channels", array);

    auto data = QJsonDocument(o).toJson();
    chunk.fileSize = 8 + data.size();
    chunk.data = data;

    return chunk;
}

void WavExporter::finalize()
{DDD;
    emit finished();
}


