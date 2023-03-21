#include "wavio.h"
#include "logging.h"

WavIO::WavIO(const QVector<Channel *> &source, QString fileName, QObject *parent) : FileIO(fileName, parent)
{
    Channel *firstChannel = source.first();

    channelsCount = quint16(source.size()); //не меняется
    sampleRate = (quint32)qRound(1.0 / firstChannel->data()->xStep()); //может измениться в методе addChannel
    samples = quint32(firstChannel->data()->samplesCount()); //может измениться в методе addChannel
    M = m_format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float не меняется
}

WavIO::~WavIO()
{

}

void WavIO::setFormat(WavFormat format)
{
    m_format = format;
}

WavChunkFmt WavIO::initFmt(int channelsCount, int sampleRate)
{
    WavChunkFmt header;

    header.fmtSize = (m_format==WavFormat::WavPCM?16:40);
    header.wFormatTag = m_format==WavFormat::WavPCM?1:0xfffe;
    header.nChannels = quint16(channelsCount); //Nc
    header.samplesPerSec = quint32(sampleRate); //F
    header.bytesPerSec = quint32(header.samplesPerSec * channelsCount * M); //F*M*Nc
    header.blockAlign = quint16(channelsCount * M); //M*Nc, data block size, bytes
    header.bitsPerSample = 8*M; //rounds up to 8*M

    if (header.fmtSize == 40) {
        header.wValidBitsPerSample = 8*M; //используем все биты, для формата 24-bit нужно будет менять
//        header.dwChannelMask = juce::AudioChannelSet::discreteChannels(channelsCount).getWaveChannelMask();

        //subFormat is PCM by default
        if (m_format == WavFormat::WavFloat)
            header.subFormat = IEEEFloatFormat;
    }
    return header;
}


void WavIO::addChannel(DataDescription *description, DataHolder *data)
{
    Q_UNUSED(description);
    if (!wavFile) wavFile = new QFile(fileName);

    M = m_format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float

    if (!m_headerWritten) {
        sampleRate = (quint32)qRound(1.0 / data->xStep()); //F
        samples = quint32(data->samplesCount());

        m_headerWritten = true;

        quint32 totalSize =  sizeof(WavHeader)
                             + (m_format == WavFormat::WavPCM ? 24 : sizeof(WavChunkFmt))
                             + (m_format!=WavFormat::WavPCM ? sizeof (WavChunkFact) : 0)
                             + sizeof(WavChunkData)
                             + M * channelsCount * samples;

        m_header.cksize = totalSize;
        m_fmtChunk = initFmt(channelsCount, sampleRate);
        if (m_format != WavFormat::WavPCM) {
            m_factChunk.factSize = 4;
            m_factChunk.dwSampleLength = samples; // Nc*Ns, number of samples
        }

        m_dataChunk.dataSize = M*channelsCount * samples; //M*Nc*Ns

        if (!wavFile->open(QFile::WriteOnly)) {
            LOG(ERROR)<<QString("Не удалось открыть файл ")<<fileName;
            return;
        }

        //Создаем пустой файл
        if (wavFile->write(QByteArray(totalSize, 0x0)) < totalSize) {
            LOG(ERROR)<<QString("Не удалось создать файл нужного размера ");
            return;
        }
        wavFile->close();

        //Переоткрываем файл для маппинга
        wavFile->open(QFile::ReadWrite);
        mapped = wavFile->map(0, totalSize);
        if (!mapped) {
            LOG(ERROR)<<QString("Не удалось создать файл нужного размера ");
            wavFile->close();
            return;
        }
        uchar *startPos = mapped;

        //Создаем заголовок файла
        memcpy(mapped, &m_header, sizeof(WavHeader));
        mapped += sizeof(WavHeader);

        if (m_format == WavFormat::WavPCM) memcpy(mapped, &m_fmtChunk, 24);
        else memcpy(mapped, &m_fmtChunk, sizeof(m_fmtChunk));
        mapped += m_format == WavFormat::WavPCM ? 24 : sizeof(m_fmtChunk);

        if (m_format!=WavFormat::WavPCM) {
            memcpy(mapped, &m_factChunk, sizeof(m_factChunk));
            mapped += sizeof(m_factChunk);
        }

        memcpy(mapped, &m_dataChunk, sizeof(m_dataChunk));
        mapped += sizeof(m_dataChunk);

        dataPos = mapped - startPos;
    }

    //Записываем данные в свои позиции
    if (dataPos != 0) {
        QByteArray channelData = data->wavData(0, data->samplesCount(),
                                                        m_format==WavFormat::WavFloat?2:1);

        //записываем каждый сэмпл на свое место
        for (int sample = 0; sample < data->samplesCount(); ++sample) {
            // i-й отсчет n-го канала имеет номер
            //       [n + i*ChannelsCount]
            //то есть целевой указатель будет иметь адрес
            //       sizeof(WavHeader) + (n+i*ChannelsCount)*M
            int offset = (currentChannelsCount + sample * channelsCount) * M;
            memcpy(mapped + dataPos + offset, reinterpret_cast<void*>(channelData.data() + sample * M), M);
        }
    }

    currentChannelsCount++;
    emit tick();
}

void WavIO::finalize()
{
    delete wavFile;
}
