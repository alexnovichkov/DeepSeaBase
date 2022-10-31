#include "wavfile.h"

WavFile::WavFile(const QString &fileName) : FileDescriptor(fileName)
{

}

WavFile::WavFile(const FileDescriptor &other, const QString &fileName, QVector<int> indexes) : FileDescriptor(fileName)
{

}

WavFile::WavFile(const QVector<Channel *> &source, const QString &fileName) : FileDescriptor(fileName)
{

}

WavFile::~WavFile()
{
//    delete m_header;
//    delete m_dataChunk;
//    delete m_fmtChunk;
//    qDeleteAll(m_assocFiles);
//    delete m_cueChunk;
//    delete m_factChunk;
}


void WavFile::read()
{
    QFile wavFile(fileName());
    if (!wavFile.open(QFile::ReadOnly)) {
        qCritical()<<"Не удалось открыть файл"<<fileName();
        m_valid = false;
        return;
    }

    dataDescription().put("dateTime", wavFile.fileTime(QFileDevice::FileBirthTime));
    dataDescription().put("fileCreationTime", wavFile.fileTime(QFileDevice::FileBirthTime));

    if (wavFile.size() < 44) {
        qCritical()<<"Файл слишком маленький:"<<fileName();
        wavFile.close();
        m_valid = false;
        return;
    }

    QDataStream r(&wavFile);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    quint16 u16;
    quint32 u32;

    r >> u32;
    m_header.ckID = u32;
    if (m_header.ckID != fourCC("RIFF")) {
        qCritical()<<"Неизвестный тип файла:"<<fileName();
        m_valid = false;
        return;
    }

    r >> u32; m_header.cksize = u32;
    r >> u32; m_header.waveId = u32;
    if (m_header.waveId != fourCC("WAVE")) {
        qCritical()<<"Неизвестный тип файла:"<<fileName();
        m_valid = false;
        return;
    }

    while (!r.atEnd()) {
        quint32 chunkType;
        r >> chunkType;
        quint32 length;
        r >> length;

        if (chunkType == fourCC("fmt ")) {
            if (m_fmtChunk.fmtSize != 0) {
                qCritical()<<"Два блока fmt в одном файле:"<<fileName();
                m_valid = false;
                return;
            }
            m_fmtChunk.fmtSize = length;

            r >> u16; m_fmtChunk.wFormatTag = u16;
            r >> u16; m_fmtChunk.nChannels = u16; //Nc
            r >> u32; m_fmtChunk.samplesPerSec = u32; //F, sampleRate
            r >> u32; m_fmtChunk.bytesPerSec = u32; //F*M*Nc
            r >> u16; m_fmtChunk.blockAlign = u16; //M*Nc, data block size, bytes
            r >> u16; m_fmtChunk.bitsPerSample = u16; //rounds up to 8*M

            if (m_fmtChunk.wFormatTag == 0xfffe) {// WAVE_FORMAT_EXTENSIBLE
                r >> u16; m_fmtChunk.cbSize = u16;
                r >> u16; m_fmtChunk.wValidBitsPerSample = u16;
                r >> u32; m_fmtChunk.dwChannelMask = u32;


                r >> u32; m_fmtChunk.subFormat.data1 = u32;
                r >> u16; m_fmtChunk.subFormat.data2 = u16;
                r >> u16; m_fmtChunk.subFormat.data3 = u16;
                for (int i=0; i<8; ++i) r >> m_fmtChunk.subFormat.data4[i];
            }

            audioChannelSet = juce::AudioChannelSet::fromWaveChannelMask(m_fmtChunk.dwChannelMask);
//            if (audioChannelSet.size() != static_cast<int> (m_fmtChunk.nChannels)) {
//                // for backward compatibility with old wav files, assume 1 or 2
//                // channel wav files are mono/stereo respectively
//                if (m_fmtChunk.nChannels <= 2 && m_fmtChunk.dwChannelMask == 0)
//                    audioChannelSet = juce::AudioChannelSet::canonicalChannelSet (static_cast<int> (m_fmtChunk.nChannels));
//                else {
//                    auto discreteSpeaker = static_cast<int> (juce::AudioChannelSet::discreteChannel0);
//                    while (audioChannelSet.size() < static_cast<int> (m_fmtChunk.nChannels))
//                        audioChannelSet.addChannel (static_cast<juce::AudioChannelSet::ChannelType> (discreteSpeaker++));
//                }
//            }
        }

        else if (chunkType == fourCC("fact")) {
            if (m_factChunk.factSize) {
                qCritical()<<"Два блока fact в одном файле:"<<fileName();
                m_valid = false;
                return;
            }

            m_factChunk.factSize = length;
            r >> u32; m_factChunk.dwSampleLength = u32;
        }

        else if (chunkType == fourCC("data")) {
            if (m_dataChunk.dataSize != 0) {
                qCritical()<<"Два блока data в одном файле:"<<fileName();
                m_valid = false;
                return;
            }
            m_dataChunk.dataSize = length;
            dataBegin = r.device()->pos();
            r.skipRawData(m_dataChunk.dataSize);
        }

        else if (chunkType == fourCC("cue ")) {
            if (m_cueChunk.cueSize != 0) {
                qCritical()<<"Два блока cue в одном файле:"<<fileName();
                m_valid = false;
                return;
            }
            m_cueChunk.cueSize = length;
            r >> u32; m_cueChunk.dwCuePoints = u32;
            for (uint i=0; i<u32; ++i) {
                WavChunkCue::Cue c;
                auto count = r.readRawData((char*)&c, sizeof(c));
                if (count != -1) m_cueChunk.cues.append(c);
            }
        }

        else if (chunkType == fourCC("file")) {
            WavChunkFile file;
            file.fileSize = length;
            r >> file.dwName >> file.dwMedType;
            file.data.resize(length - 8);
            r.readRawData(file.data.data(), file.data.size());

            m_assocFiles << file;
        }


        else r.skipRawData(length);
    }

    int channelsCount = 0;

    switch (m_fmtChunk.wFormatTag) {
        case 3: {//floating point, simple fmt
            m_dataPrecision = DataPrecision::Float;
            break;
        }
        case 1: {//PCM
            if (m_fmtChunk.bitsPerSample <=8) m_dataPrecision = DataPrecision::UInt8;
            else if (m_fmtChunk.bitsPerSample <= 16) m_dataPrecision = DataPrecision::Int16;
            else if (m_fmtChunk.bitsPerSample <= 32) m_dataPrecision = DataPrecision::Int32;
            else m_dataPrecision = DataPrecision::Int64;
            break;
        }
        case 0xfffe: {//extended fmt
            if (m_fmtChunk.subFormat == pcmFormat) {//PCM
                if (m_fmtChunk.wValidBitsPerSample <=8) m_dataPrecision = DataPrecision::UInt8;
                else if (m_fmtChunk.wValidBitsPerSample <= 16) m_dataPrecision = DataPrecision::Int16;
                else if (m_fmtChunk.wValidBitsPerSample <= 32) m_dataPrecision = DataPrecision::Int32;
                else m_dataPrecision = DataPrecision::Int64;
            }
            else if (m_fmtChunk.subFormat == IEEEFloatFormat) {//floating point
                m_dataPrecision = DataPrecision::Float;
            }
            else m_valid = false;
            break;
        }
        default: {
            qCritical() << "This type of data is not supported:"<<m_fmtChunk.wFormatTag;
            m_valid = false;
        }
    }
    channelsCount = m_fmtChunk.nChannels;

    QJsonArray channelsData;
    if (!m_assocFiles.isEmpty()) {
        QJsonParseError error;
        auto json = QJsonDocument::fromJson(m_assocFiles.first().data, &error);
        if (error.error == QJsonParseError::NoError)  {
            channelsData = json.object().value("channels").toArray();
        }
    }

    for (int i=0; i<channelsCount; ++i) {
        if (channelsData.isEmpty())
            channels << new WavChannel(this, QString("Канал %1").arg(i+1));
        else {
            channels << new WavChannel(this, DataDescription::fromJson(channelsData.at(i).toObject()));
        }
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
    return channels.value(index, nullptr);
}

bool WavFile::canTakeChannelsFrom(FileDescriptor *other) const
{
    return false;
}

bool WavFile::canTakeAnyChannels() const
{
    return false;
}


WavChannel::WavChannel(WavFile *parent, const QString &name) : parent(parent)
{
    dataDescription().put("name", name);
    dataDescription().put("xname", "s");
    dataDescription().put("yname", "Pa");

    uint samples = 0;
    if (parent->m_fmtChunk.blockAlign != 0)
        samples = parent->m_dataChunk.dataSize / parent->m_fmtChunk.blockAlign;
    dataDescription().put("samples", samples);

    dataDescription().put("blocks", 1);
    dataDescription().put("samplerate", parent->m_fmtChunk.samplesPerSec);
    dataDescription().put("function.name", "Time");

    dataDescription().put("function.type", 1); //time data
    dataDescription().put("function.logscale", "linear");
    dataDescription().put("function.logref", 1);
    dataDescription().put("function.format", "real");
    if (parent->m_dataPrecision == DataPrecision::Float)
        dataDescription().put("function.precision", "float");
    else if (parent->m_dataPrecision == DataPrecision::Int16)
        dataDescription().put("function.precision", "int16");

    _data->setThreshold(2e-5);
    _data->setYValuesUnits(DataHolder::unitsFromString(dataDescription().get("function.logscale").toString()));
    _data->setYValuesFormat(DataHolder::formatFromString(dataDescription().get("function.format").toString()));
    _data->setXValues(0, 1.0 / parent->m_fmtChunk.samplesPerSec, samples);
    _data->setZValues(0, 1, 1);
}

WavChannel::WavChannel(WavFile *parent, const DataDescription &description) : parent(parent)
{
    dataDescription() = description;

    uint samples = 0;
    if (parent->m_fmtChunk.blockAlign != 0)
        samples = parent->m_dataChunk.dataSize / parent->m_fmtChunk.blockAlign;
    dataDescription().put("samples", samples);


    _data->setYValuesUnits(DataHolder::unitsFromString(dataDescription().get("function.logscale").toString()));
    _data->setYValuesFormat(DataHolder::formatFromString(dataDescription().get("function.format").toString()));
    _data->setXValues(0, 1.0 / parent->m_fmtChunk.samplesPerSec, samples);
    _data->setZValues(0, 1, 1);
}

Descriptor::DataType WavChannel::type() const
{
    return Descriptor::TimeResponse;
}

void WavChannel::populate()
{
    if (parent->dataBegin < 0 || !parent->m_valid) return;

    _data->clear();

    int channelIndex = index();

    QFile rawFile(parent->fileName());

    if (rawFile.open(QFile::ReadOnly)) {
        int M = 1;
        switch (parent->m_dataPrecision) {
            case DataPrecision::Int16:
            case DataPrecision::UInt16: M=2; break;
            case DataPrecision::Float:
            case DataPrecision::Int32:
            case DataPrecision::UInt32: M=4; break;
            case DataPrecision::Int64:
            case DataPrecision::UInt64: M=8; break;
            default: break;
        }


        QVector<double> YValues;
        const int channelsCount = parent->channelsCount();

        // map file into memory
        unsigned char *ptr = rawFile.map(parent->dataBegin, parent->m_dataChunk.dataSize);
        if (ptr) {//достаточно памяти отобразить весь файл
            unsigned char *ptrCurrent = ptr;

            /*
                * i-й отсчет n-го канала имеет номер
                * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                *
                * если BlockSize=1 или ChanBlockSize=1, то
                * n + i*ChannelsCount
                */
            YValues.resize(data()->samplesCount());
            for (int i=0; i < YValues.size(); ++i) {
                ptrCurrent = ptr + (channelIndex + i*channelsCount) * M;
                YValues[i] = convertFrom<double>(ptrCurrent, parent->m_dataPrecision);
            }

        } /// mapped
        else {
            //читаем классическим способом через getChunk

            QDataStream readStream(&rawFile);
            readStream.setByteOrder(QDataStream::LittleEndian);
            if (parent->m_dataPrecision == DataPrecision::Float)
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            qint64 actuallyRead = 0;
            readStream.skipRawData(parent->dataBegin);


            const quint64 chunkSize = channelsCount;
            while (1) {
                QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, parent->m_dataPrecision, &actuallyRead);

                //распихиваем данные по каналам
                actuallyRead /= channelsCount;
                YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                if (actuallyRead < 1)
                    break;
            }
        }

        YValues.resize(data()->samplesCount());
        _data->setYValues(YValues, _data->yValuesFormat());
        setPopulated(true);
        rawFile.close();

    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->fileName();
    }
}

FileDescriptor *WavChannel::descriptor() const
{
    return parent;
}

int WavChannel::index() const
{
    if (parent) return parent->channels.indexOf(const_cast<WavChannel*>(this));
    return -1;
}


QString WavFile::fileType() const
{
    return "wav";
}

QStringList WavFile::fileFilters()
{
    return {"Файлы wav (*.wav)"};
}

QStringList WavFile::suffixes()
{
    return {"wav"};
}
