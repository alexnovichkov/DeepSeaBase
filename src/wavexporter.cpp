#include "wavexporter.h"
#include "algorithms.h"
#include "fileformats/filedescriptor.h"
#include <QtDebug>
#include <QDataStream>
#include "logging.h"

#define BLOCK_SIZE 10000

WavExporter::WavExporter(FileDescriptor * file, const QVector<int> &indexes, QObject *parent)
    : QObject(parent), file(file), indexes(indexes)
{

}

WavExporter::WavExporter(Channel *channel, QObject *parent):
    QObject(parent), channel(channel)
{
    file = channel->descriptor();
    indexes << channel->index();
}

WavExporter::~WavExporter()
{

}

static constexpr quint32 fourCC(const char (&ch)[5])
{
    return quint32(quint8(ch[0])) | quint32(quint8(ch[1])) << 8 | quint32(quint8(ch[2])) << 16 | quint32(quint8(ch[3])) << 24;
}

struct WavHeader
{
    quint32 id = fourCC("RIFF"); //"RIFF"
    quint32 totalSize; //4 + 24 + (8 + M*Nc*Ns)
    quint32 waveId = fourCC("WAVE"); //"WAVE"
    quint32 fmtId = fourCC("fmt ");	//"fmt "
    quint32 fmtSize = 16;
    quint16 format = 1; //PCM, M=2
    quint16 channelsCount; //Nc
    quint32 samplesPerSec; //F
    quint32 bytesPerSec; //F*M*Nc
    quint16 blockAlign; //M*Nc
    quint16 bitsPerSample; //rounds up to 8*M
    quint32 dataId = fourCC("data"); //"data"
    quint32 dataSize; //M*Nc*Ns
} __attribute__((packed));

//int WavExporter::chunksCount() const
//{
//    int blocks = file->samplesCount()/BLOCK_SIZE;
//    if (file->samplesCount() % BLOCK_SIZE != 0) blocks++;
//    return blocks;
//}

int WavExporter::chunksCount() const
{
    return indexes.size();
}

void WavExporter::stop()
{
    finalize();
}

void WavExporter::writeWithStreams(const QString &wavFileName)
{
    //2. Записываем заголовок файла wav
    QFile wavFile(wavFileName);
    if (!wavFile.open(QFile::WriteOnly)) {
        qCritical()<<"Не удалось открыть файл"<<wavFileName;
        finalize();
        return;
    }

    QDataStream s(&wavFile);
    s.setByteOrder(QDataStream::LittleEndian);
    s.writeRawData("RIFF",4);

    //Определяем общий размер файла Wav
    quint32 totalSize = 36 + 2 * indexes.size() * file->samplesCount();

    s << totalSize;
    s.writeRawData("WAVE",4);
    s.writeRawData("fmt ",4);
    s << (quint32)16; //chunk size
    s << (quint16)1; //PCM
    s << (quint16)indexes.size(); //channels count
    quint32 samplerate = (quint32)(1.0 / file->xStep());
    s << samplerate; //samplerate
    s << quint32(samplerate * indexes.size() * 2); // bytes per second
    s << quint16(indexes.size() * 2); //block align
    s << (quint16)16;


    //3. Записываем данные поканально
    s.writeRawData("data",4);
    s << quint32(2 * indexes.size() * file->samplesCount());

    //пишем блоками по BLOCK_SIZE отсчетов
//    qDebug()<<"chunks" << chunksCount();
    for (int chunk = 0; chunk < chunksCount(); ++chunk) {
//        qDebug()<<"chunk"<<chunk+1;
        if (QThread::currentThread()->isInterruptionRequested()) {
            wavFile.close();
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
//        QElapsedTimer reading; reading.start();
        for (int c = 0; c < indexes.size(); ++c) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                wavFile.close();
                qDebug()<<"Сохранение файла wav прервано";
                finalize();
                return;
            }

            Channel *channel = file->channel(indexes.at(c));
            bool populated = channel->populated();
            if (!populated) channel->populate();
            chunkData.append(channel->wavData(chunk * BLOCK_SIZE, BLOCK_SIZE));
            if (!populated) channel->clear();
        }
//        qDebug()<<"reading"<<reading.elapsed();

        //теперь перетасовываем chunkData - берем из него по одному отсчету каждого канала
        //и записываем в wav
//        QElapsedTimer writing; writing.start();
        for (int i = 0; i < chunkData.constFirst().size()/2; ++i) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                wavFile.close();
                qDebug()<<"Сохранение файла wav прервано";
                finalize();
                return;
            }

            for (int c = 0; c < chunkData.size(); ++c) {
                s.device()->write(chunkData.at(c).mid(i*2, 2));
            }
        }
//        qDebug()<<"writing"<<writing.elapsed();
        emit tick(chunk+1);
    }
    wavFile.close();
}

bool WavExporter::writeWithMap(const QString &wavFileName)
{
    //2. Записываем заголовок файла wav
    QFile wavFile(wavFileName);
    if (!wavFile.open(QFile::WriteOnly)) {
        qCritical()<<"Не удалось открыть файл"<<wavFileName;
        //finalize();
        return false;
    }

    //полный размер файла равен 24 +
    //Определяем общий размер файла Wav
    const int channelsCount = indexes.size();
    quint32 totalSize = 44 + 2 * channelsCount * file->samplesCount();
    wavFile.write(QByteArray(totalSize, 0x0));
    wavFile.close();

    wavFile.open(QFile::ReadWrite);
    uchar *mapped = wavFile.map(0, totalSize);
    if (!mapped) {
        qCritical()<<"Не удалось создать файл нужного размера";
        wavFile.close();
        return false;
    }

    WavHeader header;
    header.totalSize = totalSize - 8; //4 + 24 + (8 + M*Nc*Ns)
    header.channelsCount = quint16(channelsCount); //Nc
    header.samplesPerSec = (quint32)qRound(1.0 / file->xStep()); //F
    header.bytesPerSec = quint32(header.samplesPerSec * channelsCount * 2); //F*M*Nc
    header.blockAlign = quint16(channelsCount * 2); //M*Nc
    header.bitsPerSample = 16; //rounds up to 8*M
    header.dataSize = quint32(2 * channelsCount * file->samplesCount()); //M*Nc*Ns

    memcpy(mapped, &header, sizeof(WavHeader));

    for (int ch = 0; ch < channelsCount; ++ch) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            wavFile.close();
            qDebug()<<"Сохранение файла wav прервано";
            finalize();
            return true;
        }

        Channel *channel = file->channel(indexes.at(ch));
        bool populated = channel->populated();
        if (!populated) channel->populate();
        QByteArray channelData = channel->wavData(0, channel->samplesCount());

        //записываем каждый сэмпл на свое место
        for (int sample = 0; sample < channel->samplesCount(); ++sample) {
            // i-й отсчет ch-го канала имеет номер
            //       [n + i*ChannelsCount]
            //то есть целевой указатель будет иметь адрес
            //       sizeof(WavHeader) + (n+i*ChannelsCount)*2
            int offset = sizeof(WavHeader) + (ch + sample*channelsCount)*2;
            memcpy(mapped + offset, reinterpret_cast<void*>(channelData.data() + sample*2), 2);

        }

        if (!populated) channel->clear();
        emit tick(ch+1);
    }

    wavFile.close();
    return true;
}

void WavExporter::start()
{
    QElapsedTimer time;
    time.start();

    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return;
    }

    //1. Определяем имя файла wav
    if (_wavFile.isEmpty())
    _wavFile = createUniqueFileName("", file->fileName(), "", "wav", true);

    if (!writeWithMap(_wavFile))
        writeWithStreams(_wavFile);

    qDebug()<<"total"<<time.elapsed();
    finalize();
}

void WavExporter::finalize()
{
    emit finished();
}
