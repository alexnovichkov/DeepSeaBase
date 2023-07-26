#include "wavfile.h"
#include "logging.h"

WavFile::WavFile(const QString &fileName) : FileDescriptor(fileName)
{DD;

}

WavFile::WavFile(const FileDescriptor &other, const QString &fileName, QVector<int> indexes) : FileDescriptor(fileName)
{DD;
    QVector<Channel *> source;
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) source << other.channel(i);
    else
        for (int i: indexes) source << other.channel(i);

    init(source);
}

WavFile::WavFile(const FileDescriptor &other, const QString &fileName, QVector<int> indexes,
                 WavFormat format) : FileDescriptor(fileName), m_format(format)
{DD;
    QVector<Channel *> source;
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) source << other.channel(i);
    else
        for (int i: indexes) source << other.channel(i);

    init(source);
}

WavFile::WavFile(const QVector<Channel *> &source, const QString &fileName) : FileDescriptor(fileName)
{DD;
    init(source);
}

WavFile::WavFile(const QVector<Channel *> &source, const QString &fileName, WavFormat format)
    : FileDescriptor(fileName), m_format(format)
{DD;
    init(source);
}

WavFile::~WavFile()
{DD;
//    delete m_header;
//    delete m_dataChunk;
//    delete m_fmtChunk;
//    qDeleteAll(m_assocFiles);
//    delete m_cueChunk;
//    delete m_factChunk;
}


void WavFile::read()
{DD;
    QFile wavFile(fileName());
    if (!wavFile.open(QFile::ReadOnly)) {
        LOG(ERROR)<<QString("Не удалось открыть файл")<<fileName();
        m_valid = false;
        return;
    }

    if (wavFile.size() < 44) {
        LOG(ERROR)<<QString("Файл слишком маленький:")<<fileName();
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
        LOG(ERROR)<<QString("Неизвестный тип файла:")<<fileName();
        m_valid = false;
        return;
    }

    r >> u32; m_header.cksize = u32;
    r >> u32; m_header.waveId = u32;
    if (m_header.waveId != fourCC("WAVE")) {
        LOG(ERROR)<<QString("Неизвестный тип файла:")<<fileName();
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
                LOG(ERROR)<<QString("Два блока fmt в одном файле:")<<fileName();
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

//            audioChannelSet = juce::AudioChannelSet::fromWaveChannelMask(m_fmtChunk.dwChannelMask);
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
                LOG(ERROR)<<QString("Два блока fact в одном файле:")<<fileName();
                m_valid = false;
                return;
            }

            m_factChunk.factSize = length;
            r >> u32; m_factChunk.dwSampleLength = u32;
        }

        else if (chunkType == fourCC("data")) {
            if (m_dataChunk.dataSize != 0) {
                LOG(ERROR)<<QString("Два блока data в одном файле:")<<fileName();
                m_valid = false;
                return;
            }
            m_dataChunk.dataSize = length;
            m_dataBegin = r.device()->pos();
            r.skipRawData(m_dataChunk.dataSize);
        }

        else if (chunkType == fourCC("cue ")) {
            if (m_cueChunk.cueSize != 0) {
                LOG(ERROR)<<QString("Два блока cue в одном файле:")<<fileName();
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



        else if (chunkType == fourCC("LIST")) {
            quint32 subChunk = 0;
            quint32 subChunkSize = 0;
            r >> subChunk >> subChunkSize;
            if (subChunk == fourCC("adtl")) {
                quint32 adtlLength = subChunkSize;
                WavChunkFile file;
                file.listSize = length;
                file.adtlSize = adtlLength;
                while (subChunkSize > 0) {
                    quint32 adtlSubChunk = 0;
                    quint32 adtlSubChunkSize = 0;
                    r >> adtlSubChunk >> adtlSubChunkSize;
                    subChunkSize -= adtlSubChunkSize+8;
                    if (adtlSubChunk == fourCC("file")) {
                        file.fileSize = adtlSubChunkSize;
                        r >> file.fileName >> file.fileMedType;
                        file.fileData.resize(file.fileSize - 8);
                        r.readRawData(file.fileData.data(), file.fileData.size());
                    } else
                    if (adtlSubChunk == fourCC("labl")) {
                        file.lablSize = adtlSubChunkSize;
                        r >> file.lablName;
                        file.lablData.resize(file.lablSize - 4);
                        r.readRawData(file.lablData.data(), file.lablData.size());
                    } else
                    if (adtlSubChunk == fourCC("note")) {
                        file.noteSize = adtlSubChunkSize;
                        r >> file.noteName;
                        file.noteData.resize(file.noteSize - 4);
                        r.readRawData(file.noteData.data(), file.noteData.size());
                    } else
                    if (adtlSubChunk == fourCC("ltxt")) {
                        file.ltxtSize = adtlSubChunkSize;
                        r >> file.ltxtName >> file.ltxtSampleLength >> file.ltxtPurpose;
                        r >> file.ltxtCountry >> file.ltxtLanguage >> file.ltxtDialect >> file.ltxtCodePage;
                        file.ltxtData.resize(file.ltxtSize - 20);
                        r.readRawData(file.ltxtData.data(), file.ltxtData.size());
                    }
                    else r.skipRawData(adtlSubChunkSize);
                }
                m_assocFiles << file;
            }
            else r.skipRawData(subChunkSize); //неизвестный LIST
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
            LOG(ERROR) << "This type of data is not supported:"<<m_fmtChunk.wFormatTag;
            m_valid = false;
        }
    }
    channelsCount = m_fmtChunk.nChannels;

    QJsonArray channelsData;
    if (!m_assocFiles.isEmpty()) {
        QJsonParseError error;
        QByteArray data = QByteArray::fromBase64(m_assocFiles.first().ltxtData);

        auto json = QJsonDocument::fromJson(data, &error);
        if (error.error == QJsonParseError::NoError)  {
            channelsData = json.object().take("channels").toArray();
            dataDescription() = DataDescription::fromJson(json.object());
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
{DD;
}

int WavFile::channelsCount() const
{DD;
    return channels.size();
}

Channel *WavFile::channel(int index) const
{DD;
    return channels.value(index, nullptr);
}

bool WavFile::canTakeChannelsFrom(FileDescriptor *other) const
{DD;
    Q_UNUSED(other);
    return false;
}

bool WavFile::canTakeAnyChannels() const
{DD;
    return false;
}

QString WavFile::fileType() const
{DD;
    return "wav";
}

QStringList WavFile::fileFilters()
{DD;
    return {"Файлы wav (*.wav)"};
}

QStringList WavFile::suffixes()
{DD;
    return {"*.wav"};
}

void WavFile::init(const QVector<Channel *> &source)
{DD;
    auto d = source.first()->descriptor();
    setDataDescription(d->dataDescription());
    updateDateTimeGUID();

    //1. если среди indexes нет временных каналов, то ничего не делаем
    //2. параметры файла берем по первому каналу. Каналы, не подходящие по типу, пропускаем

    QVector<Channel*> timeChannels;
    timeChannels.reserve(source.size());
    Channel *firstChannel = nullptr;
    for (auto c: source) if (c->type()==Descriptor::TimeResponse) {
        firstChannel = c;
        break;
    }

    if (!firstChannel) {
        m_valid = false;
        return;
    }

    for (auto c: source) {
        if (c->type()==Descriptor::TimeResponse && c->data()->xStep() == firstChannel->data()->xStep()
            && c->data()->samplesCount() == firstChannel->data()->samplesCount())
            timeChannels << c;
    }
    if (timeChannels.isEmpty()) {
        m_valid = false;
        return;
    }

    if (channelsFromSameFile(timeChannels)) {
        dataDescription().put("source.file", d->fileName());
        dataDescription().put("source.guid", d->dataDescription().get("guid"));
        dataDescription().put("source.dateTime", d->dataDescription().get("dateTime"));
        dataDescription().put("dateTime", d->dataDescription().get("dateTime"));

        if (d->channelsCount() > timeChannels.size()) {
            //только если копируем не все каналы
            dataDescription().put("source.channels", stringify(channelIndexes(timeChannels)));
        }
    }


    for (int ch = 0; ch < timeChannels.size(); ++ch)
        new WavChannel(timeChannels.at(ch), this);

    const quint16 channelsCount = quint16(timeChannels.size()); //Fc
    const quint32 sampleRate = (quint32)qRound(1.0 / firstChannel->data()->xStep()); //F
    const quint32 samples = quint32(firstChannel->data()->samplesCount());
    const quint16 M = m_format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float

    QString channelName;
    QString channelDescription;
    if (channelsCount == 1) {
        channelName = firstChannel->name();
        channelDescription = firstChannel->description();
    }

    auto assocFile = initFile(samples, channelName, channelDescription);

    quint32 totalSize =  sizeof(WavHeader)
                         + (m_format == WavFormat::WavPCM ? 24 : sizeof(WavChunkFmt))
                         + (m_format!=WavFormat::WavPCM ? sizeof (WavChunkFact) : 0)
//                         + 12 + sizeof(WavChunkCue::Cue)
//                         + 8 + assocFile.listSize
                         + sizeof(WavChunkData)
                         + M * channelsCount * samples;

    m_header = initHeader(totalSize);
    m_fmtChunk = initFmt(channelsCount, sampleRate);
    if (m_format != WavFormat::WavPCM) {
        m_factChunk = initFact(samples);
    }
//    m_cueChunk = initCue();
//    m_assocFiles << assocFile;
    m_dataChunk = initDataHeader(channelsCount, samples);
    m_dataBegin = totalSize - M * channelsCount * samples;

    if (!writeWithMap(timeChannels, totalSize))
        writeWithStream(timeChannels);
}

bool WavFile::writeWithMap(const QVector<Channel*> &source, quint32 totalSize)
{DD;
    const quint16 M = m_format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float

    // Сохранение
    QFile wavFile(fileName());
    if (!wavFile.open(QFile::WriteOnly)) {
        LOG(ERROR)<<QString("Не удалось открыть файл ")<<fileName();
        m_valid = false;
        return false;
    }

    //Создаем пустой файл
    if (wavFile.write(QByteArray(totalSize, 0x0)) < totalSize) {
        LOG(ERROR)<<QString("Не удалось создать файл нужного размера ");
        m_valid = false;
        return false;
    }
    wavFile.close();

    //Переоткрываем файл для маппинга
    wavFile.open(QFile::ReadWrite);
    uchar *mapped = wavFile.map(0, totalSize);
    if (!mapped) {
        LOG(ERROR)<<QString("Не удалось создать файл нужного размера ");
        wavFile.close();
        m_valid = false;
        return false;
    }

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

//    memcpy(mapped, &m_cueChunk.cueId, 4); mapped +=4;
//    memcpy(mapped, &m_cueChunk.cueSize, 4); mapped +=4;
//    memcpy(mapped, &m_cueChunk.dwCuePoints, 4); mapped +=4;
//    for (auto c: m_cueChunk.cues) {
//        memcpy(mapped, &c, sizeof(c)); mapped += sizeof(c);
//    }

//    if (!m_assocFiles.isEmpty()) {
//        auto assocFile = m_assocFiles.first();
//        memcpy(mapped, &assocFile.listId, 4); mapped += 4;
//        memcpy(mapped, &assocFile.listSize, 4); mapped += 4;
//        memcpy(mapped, &assocFile.adtlId, 4); mapped += 4;
//        memcpy(mapped, &assocFile.adtlSize, 4); mapped += 4;

//        memcpy(mapped, &assocFile.lablId, 4); mapped += 4;
//        memcpy(mapped, &assocFile.lablSize, 4); mapped += 4;
//        memcpy(mapped, &assocFile.lablName, 4); mapped += 4;
//        memcpy(mapped, assocFile.lablData.data(), assocFile.lablData.size()); mapped += assocFile.lablData.size();

//        memcpy(mapped, &assocFile.noteId, 4); mapped += 4;
//        memcpy(mapped, &assocFile.noteSize, 4); mapped += 4;
//        memcpy(mapped, &assocFile.noteName, 4); mapped += 4;
//        memcpy(mapped, assocFile.noteData.data(), assocFile.noteData.size()); mapped += assocFile.noteData.size();

//        memcpy(mapped, &assocFile.ltxtId, 4); mapped += 4;
//        memcpy(mapped, &assocFile.ltxtSize, 4); mapped += 4;
//        memcpy(mapped, &assocFile.ltxtName, 4); mapped += 4;
//        memcpy(mapped, &assocFile.ltxtSampleLength, 4); mapped += 4;
//        memcpy(mapped, &assocFile.ltxtPurpose, 4); mapped += 4;
//        memcpy(mapped, &assocFile.ltxtCountry, 2); mapped += 2;
//        memcpy(mapped, &assocFile.ltxtLanguage, 2); mapped += 2;
//        memcpy(mapped, &assocFile.ltxtDialect, 2); mapped += 2;
//        memcpy(mapped, &assocFile.ltxtCodePage, 2); mapped += 2;
//        memcpy(mapped, assocFile.ltxtData.data(), assocFile.ltxtData.size()); mapped += assocFile.ltxtData.size();
//    }

    memcpy(mapped, &m_dataChunk, sizeof(m_dataChunk));
    mapped += sizeof(m_dataChunk);

    for (int ch = 0; ch < source.size(); ++ch) {
        auto sourceChannel = source.at(ch);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        QByteArray channelData = sourceChannel->data()->wavData(0, sourceChannel->data()->samplesCount(),
                                                        m_format==WavFormat::WavFloat?2:1);

        //записываем каждый сэмпл на свое место
        for (int sample = 0; sample < sourceChannel->data()->samplesCount(); ++sample) {
            // i-й отсчет n-го канала имеет номер
            //       [n + i*ChannelsCount]
            //то есть целевой указатель будет иметь адрес
            //       sizeof(WavHeader) + (n+i*ChannelsCount)*M
            int offset = (ch + sample*source.size())*M;
            memcpy(mapped + offset, reinterpret_cast<void*>(channelData.data() + sample*M), M);
        }

        if (!populated) sourceChannel->clear();
    }
    return true;
}

void WavFile::writeWithStream(const QVector<Channel *> &source)
{DD;
    QFile wavFile(fileName());
    if (!wavFile.open(QFile::WriteOnly)) {
        LOG(ERROR)<<QString("Не удалось открыть файл ")<<fileName();
        return;
    }

    QDataStream s(&wavFile);
    s.setByteOrder(QDataStream::LittleEndian);

    //Определяем общий размер файла Wav
    const quint16 M = m_format == WavFormat::WavFloat ? 4 : 2; // bytesForFormat = short / float

    //Создаем заголовок файла
    {
        s << m_header.ckID;
        s << m_header.cksize;
        s << m_header.waveId;
    }
    {
        s << m_fmtChunk.fmtId;
        s << m_fmtChunk.fmtSize;
        s << m_fmtChunk.wFormatTag;
        s << m_fmtChunk.nChannels;
        s << m_fmtChunk.samplesPerSec;
        s << m_fmtChunk.bytesPerSec;
        s << m_fmtChunk.blockAlign;
        s << m_fmtChunk.bitsPerSample;
        if (m_format != WavFormat::WavPCM) {
            s << m_fmtChunk.cbSize;
            s << m_fmtChunk.wValidBitsPerSample;
            s << m_fmtChunk.dwChannelMask;
            s << m_fmtChunk.subFormat.data1;
            s << m_fmtChunk.subFormat.data2;
            s << m_fmtChunk.subFormat.data3;
            for (unsigned char i: m_fmtChunk.subFormat.data4) s << i;

            s << m_factChunk.factID;
            s << m_factChunk.factSize;
            s << m_factChunk.dwSampleLength;
        }
    }
//    {
//        s << m_cueChunk.cueId;
//        s << m_cueChunk.cueSize;
//        s << m_cueChunk.dwCuePoints;
//        for (auto c: m_cueChunk.cues) {
//            s << c.identifier << c.order << c.chunkID << c.chunkStart << c.blockStart << c.offset;
//        }
//    }
//    if (!m_assocFiles.isEmpty()) {
//        auto assocFile = m_assocFiles.first();
//        s << assocFile.listId << assocFile.listSize;
//        s << assocFile.adtlId << assocFile.adtlSize;

//        s << assocFile.lablId << assocFile.lablSize;
//        s << assocFile.lablName;
//        s.writeRawData(assocFile.lablData.data(), assocFile.lablData.size());

//        s << assocFile.noteId << assocFile.noteSize << assocFile.noteName;
//        s.writeRawData(assocFile.noteData.data(), assocFile.noteData.size());

//        s << assocFile.ltxtId << assocFile.ltxtSize << assocFile.ltxtName;
//        s << assocFile.ltxtSampleLength << assocFile.ltxtPurpose;
//        s << assocFile.ltxtCountry << assocFile.ltxtLanguage;
//        s << assocFile.ltxtDialect << assocFile.ltxtCodePage;
//        s.writeRawData(assocFile.ltxtData.data(), assocFile.ltxtData.size());
//    }
    {
        s << m_dataChunk.dataId;
        s << m_dataChunk.dataSize;
    }

    constexpr const int BLOCK_SIZE = 20000;
    int blocks = samplesCount()/BLOCK_SIZE;
    if (samplesCount() % BLOCK_SIZE != 0) blocks++;

    //пишем блоками по BLOCK_SIZE отсчетов
    for (int chunk = 0; chunk < blocks; ++chunk) {
        //мы должны сформировать блоки с каждого канала.
        //Так как читать сразу все каналы в память слишком объемно,
        //1. заполняем канал
        //2. берем из него BLOCK_SIZE отсчетов
        //3. из всех каналов формируем колбасу для записи
        //4. очищаем данные канала

        QVector<QByteArray> chunkData;
        for (auto c: source) {
            bool populated = c->populated();
            if (!populated) c->populate();
            auto data = c->data()->wavData(chunk * BLOCK_SIZE, BLOCK_SIZE, m_format==WavFormat::WavFloat?2:1);
            chunkData.append(data);
            if (!populated) c->clear();
        }

        //теперь перетасовываем chunkData - берем из него по одному отсчету каждого канала
        //и записываем в wav
        for (int i = 0; i < chunkData.constFirst().size()/M; ++i) {
            for (const QByteArray &c: qAsConst(chunkData)) {
                s.device()->write(c.mid(i*M, M));
            }
        }
    }
    wavFile.close();
}

WavHeader WavFile::initHeader(int totalSize)
{DD;
    WavHeader header;
    header.cksize = totalSize;

    return header;
}

WavChunkFmt WavFile::initFmt(int channelsCount, int sampleRate)
{DD;
    const int M = m_format==WavFormat::WavFloat?4:2;
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

WavChunkFact WavFile::initFact(int samplesCount)
{DD;
    WavChunkFact header;
    header.factSize = 4;
    header.dwSampleLength = samplesCount; // Nc*Ns, number of samples
    return header;
}

WavChunkData WavFile::initDataHeader(int channelsCount, int samplesCount)
{DD;
    const int M = m_format==WavFormat::WavFloat?4:2;
    WavChunkData header;
    //data block - 8 + M*Nc*Ns bytes
    header.dataSize = M*channelsCount*samplesCount; //M*Nc*Ns
    return header;
}

WavChunkCue WavFile::initCue()
{DD;
    WavChunkCue cue;
    cue.dwCuePoints = 1;
    cue.cueSize = sizeof (WavChunkCue::Cue) + 4;
    WavChunkCue::Cue c;
    cue.cues.append(c);

    return cue;
}

WavChunkFile WavFile::initFile(quint32 samplesCount, const QString &name, const QString &description)
{DD;
    WavChunkFile chunk;
    QJsonArray array;
    for (auto c: channels) {
        auto chDescr = c->dataDescription();
        chDescr.put("function.precision", m_format==WavFormat::WavFloat?"float":"int16");
        chDescr.put("function.name", "Time");
        chDescr.put("function.type", Descriptor::TimeResponse);
        array.append(chDescr.toJson());
    }
    QJsonObject o = dataDescription().toJson();
    o.insert("channels", array);

    auto data = QJsonDocument(o).toJson();

//    chunk.fileSize = 8 + data.size();
//    chunk.fileData = data;

    //labl chunk is empty if name is empty
    chunk.lablData = name.toUtf8();
    chunk.lablSize = 4+chunk.lablData.size();

    //note chunk is empty if description is empty
    chunk.noteData = description.toUtf8();
    chunk.noteSize = 4+chunk.noteData.size();

    //ltxt chunk is empty
    chunk.ltxtName = 0;
    chunk.ltxtSampleLength = samplesCount;
    chunk.ltxtCountry = 0;
    chunk.ltxtLanguage = 25; //russian
    chunk.ltxtDialect = 1; //russian
    chunk.ltxtCodePage = 0;
    chunk.ltxtData = data.toBase64();
    chunk.ltxtSize = 20+chunk.ltxtData.size();

    chunk.adtlSize = /*chunk.fileSize+8 +*/ chunk.ltxtSize+8 + chunk.lablSize+8 + chunk.noteSize+8;
    chunk.listSize = chunk.adtlSize + 8;

    return chunk;
}

WavChannel::WavChannel(WavFile *parent, const QString &name) : parent(parent)
{DD;
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
    switch (parent->m_dataPrecision) {
        case DataPrecision::Int8: dataDescription().put("function.precision", "int8"); break;
        case DataPrecision::UInt8: dataDescription().put("function.precision", "uint8"); break;
        case DataPrecision::Int16: dataDescription().put("function.precision", "int16"); break;
        case DataPrecision::UInt16: dataDescription().put("function.precision", "uint16"); break;
        case DataPrecision::Float: dataDescription().put("function.precision", "float"); break;
        case DataPrecision::Int32: dataDescription().put("function.precision", "int32"); break;
        case DataPrecision::UInt32: dataDescription().put("function.precision", "uint32"); break;
        case DataPrecision::Int64: dataDescription().put("function.precision", "int64"); break;
        case DataPrecision::UInt64: dataDescription().put("function.precision", "uint64"); break;
        case DataPrecision::Double: dataDescription().put("function.precision", "double"); break;
        default: break;
    }

    _data->setThreshold(2e-5);
    _data->setYValuesUnits(DataHolder::unitsFromString(dataDescription().get("function.logscale").toString()));
    _data->setYValuesFormat(DataHolder::formatFromString(dataDescription().get("function.format").toString()));
    _data->setXValues(0, 1.0 / parent->m_fmtChunk.samplesPerSec, samples);
    _data->setZValues(0, 1, 1);
}

WavChannel::WavChannel(WavFile *parent, const DataDescription &description) : parent(parent)
{DD;
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

WavChannel::WavChannel(Channel *other, WavFile *parent) : Channel(other), parent(parent)
{DD;
    parent->channels << this;
    uint samples = 0;
    if (parent->m_fmtChunk.blockAlign != 0)
        samples = parent->m_dataChunk.dataSize / parent->m_fmtChunk.blockAlign;
    dataDescription().put("samples", samples);
}

Descriptor::DataType WavChannel::type() const
{DD;
    return Descriptor::TimeResponse;
}

void WavChannel::populate()
{DD;
    if (parent->m_dataBegin < 0 || !parent->m_valid) return;

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
        unsigned char *ptr = rawFile.map(parent->m_dataBegin, parent->m_dataChunk.dataSize);
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
            readStream.skipRawData(parent->m_dataBegin);


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
        LOG(ERROR)<<QString("Не смог прочитать ")<<parent->fileName();
    }
}

FileDescriptor *WavChannel::descriptor() const
{DD;
    return parent;
}

int WavChannel::index() const
{DD;
    if (parent) return parent->channels.indexOf(const_cast<WavChannel*>(this));
    return -1;
}
