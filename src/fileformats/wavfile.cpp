#include "wavfile.h"

WavFile::WavFile(const QString &fileName) : FileDescriptor(fileName)
{

}

WavFile::~WavFile()
{
    delete m_header;
    delete m_dataChunk;
    delete m_fmtChunk;
}


void WavFile::read()
{
    QFile wavFile(fileName());
    if (!wavFile.open(QFile::ReadOnly)) {
        qCritical()<<"Не удалось открыть файл"<<fileName();
        return;
    }

    if (wavFile.size() < 44) {
        qCritical()<<"Файл слишком маленький:"<<fileName();
        wavFile.close();
        return;
    }

    QDataStream r(&wavFile);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    quint16 u16;
    quint32 u32;
    quint8 u8;

    m_header = new WavHeader;
    r >> u32;
    m_header->ckID = u32;
    if (m_header->ckID != fourCC("RIFF")) {
        qCritical()<<"Неизвестный тип файла:"<<fileName();
        return;
    }

    r >> u32; m_header->cksize = u32;
    r >> u32; m_header->waveId = u32;
    if (m_header->waveId != fourCC("WAVE")) {
        qCritical()<<"Неизвестный тип файла:"<<fileName();
        return;
    }

    while (!r.atEnd()) {
        quint32 chunkType;
        r >> chunkType;
        quint32 length;
        r >> length;

        if (chunkType == fourCC("fmt ")) {
            if (m_fmtChunk) {
                qCritical()<<"Два блока fmt в одном файле:"<<fileName();
                return;
            }
            m_fmtChunk = new WavChunkFmt;
            m_fmtChunk->fmtSize = length;

            r >> u16; m_fmtChunk->wFormatTag = u16;
            r >> u16; m_fmtChunk->nChannels = u16; //Nc
            r >> u32; m_fmtChunk->samplesPerSec = u32; //F, sampleRate
            r >> u32; m_fmtChunk->bytesPerSec = u32; //F*M*Nc
            r >> u16; m_fmtChunk->blockAlign = u16; //M*Nc, data block size, bytes
            r >> u16; m_fmtChunk->bitsPerSample = u16; //rounds up to 8*M

            if (m_fmtChunk->wFormatTag == 0xfffe) {// WAVE_FORMAT_EXTENSIBLE
                r >> u16; m_fmtChunk->cbSize = u16;
                r >> u16; m_fmtChunk->wValidBitsPerSample = u16;
                r >> u32; m_fmtChunk->dwChannelMask = u32;

                r >> u32; m_fmtChunk->subFormat.data1 = u32;
                r >> u16; m_fmtChunk->subFormat.data2 = u16;
                r >> u16; m_fmtChunk->subFormat.data3 = u16;
                for (int i=0; i<8; ++i) r >> m_fmtChunk->subFormat.data4[i];
            }
        }

        else if (chunkType == fourCC("fact")) {
            if (m_factChunk) {
                qCritical()<<"Два блока fact в одном файле:"<<fileName();
                return;
            }

            m_factChunk = new WavChunkFact;
            m_factChunk->factSize = length;
            r >> u32; m_factChunk->dwSampleLength = u32;
        }

        else if (chunkType == fourCC("data")) {
            m_dataChunk = new WavChunkData;
            r >> u32; m_dataChunk->dataSize = u32;
            dataSize = m_dataChunk->dataSize;
            dataBegin = r.device()->pos();
        }


        else r.skipRawData(length);
    }

    int channelsCount = 0;
    QString dataFormat;
    if (m_fmtChunk)  {
        switch (m_fmtChunk->wFormatTag) {
            case 3: {//floating point, simple fmt
                dataFormat = "float";
                break;
            }
            case 1: {//PCM
                dataFormat = "int16";
                break;
            }
            case 0xfffe: {//extended fmt
                if (m_fmtChunk->subFormat == pcmFormat) {//PCM
                    dataFormat = "int16";
                }
                if (m_fmtChunk->subFormat == IEEEFloatFormat) {//floating point
                    dataFormat = "float";
                }
                break;
            }
            default: {
                qCritical() << "This type of data is not supported:"<<m_fmtChunk->wFormatTag;
            }
        }
        channelsCount = m_fmtChunk->nChannels;
    }
    if (dataFormat.isEmpty()) {
        qCritical() << "Unable to find valid data format";
        return;
    }

    for (int i=0; i<channelsCount; ++i) {
//        channels << new WavChannel(this);
    }

}

void WavFile::write()
{
}

void WavFile::deleteChannels(const QVector<int> &channelsToDelete)
{
}

void WavFile::copyChannelsFrom(const QVector<Channel *> &)
{
}

int WavFile::channelsCount() const
{
    return channels.size();
}

void WavFile::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
}

Channel *WavFile::channel(int index) const
{
}

QString WavFile::fileType() const
{
}

bool WavFile::canTakeChannelsFrom(FileDescriptor *other) const
{

}

bool WavFile::canTakeAnyChannels() const
{
    return false;
}
