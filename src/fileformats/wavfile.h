#ifndef WAVFILE_H
#define WAVFILE_H

#include "filedescriptor.h"
#include "juce_AudioChannelSet.h"

class WavChannel;

enum class WavFormat {
    WavPCM = 0,
    WavExtendedPCM,
    WavFloat
};

static constexpr quint32 fourCC(const char (&ch)[5])
{
    return quint32(quint8(ch[0])) | quint32(quint8(ch[1])) << 8 | quint32(quint8(ch[2])) << 16 | quint32(quint8(ch[3])) << 24;
}

struct WavHeader
{
    /*const*/ quint32 ckID = fourCC("RIFF"); //"RIFF"
    quint32 cksize = 0; //8 + 48 + 12 + (8 + M*Nc*Ns)
    /*const*/ quint32 waveId = fourCC("WAVE"); //"WAVE"
}__attribute__((packed));

struct ExtensibleWavSubFormat
{
    quint32 data1;
    quint16 data2;
    quint16 data3;
    quint8  data4[8];

    bool operator== (const ExtensibleWavSubFormat& other) const noexcept   { return memcmp (this, &other, sizeof (*this)) == 0; }
    bool operator!= (const ExtensibleWavSubFormat& other) const noexcept   { return ! operator== (other); }

} __attribute__((packed));

static const ExtensibleWavSubFormat pcmFormat       = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const ExtensibleWavSubFormat IEEEFloatFormat = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const ExtensibleWavSubFormat ambisonicFormat = { 0x00000001, 0x0721, 0x11d3, { 0x86, 0x44, 0xC8, 0xC1, 0xCA, 0x00, 0x00, 0x00 } };

struct WavChunkFmt
{
    /*4*/ /*const*/ quint32 fmtId = fourCC("fmt ");	//"fmt "
    /*4*/quint32 fmtSize = 0;
    /*2*/quint16 wFormatTag = 0; //WAVE_FORMAT
    /*2*/quint16 nChannels = 0; //Nc
    /*4*/quint32 samplesPerSec = 0; //F
    /*4*/quint32 bytesPerSec = 0; //F*M*Nc
    /*2*/quint16 blockAlign = 0; //M*Nc, data block size, bytes
    /*2*/quint16 bitsPerSample = 0; //rounds up to 8*M

    //extended part
    /*2*//*const*/ quint16 cbSize = 22;
    /*2*/quint16 wValidBitsPerSample = 0;
    /*4*//*const*/ quint32 dwChannelMask = 0;
    //subFormat is PCM by default
    ExtensibleWavSubFormat subFormat = pcmFormat;
} __attribute__((packed));

struct WavChunkFact
{
    //fact block - 12 bytes
    /*4*//*const*/ quint32 factID = fourCC("fact");
    /*4*//*const*/ quint32 factSize = 0;
    /*4*/quint32 dwSampleLength = 0; // Nc*Ns, number of samples
}__attribute__((packed));

struct WavChunkData
{
    //data block - 8 + M*Nc*Ns bytes
    /*const*/ quint32 dataId = fourCC("data"); //"data"
    quint32 dataSize = 0; //M*Nc*Ns
}__attribute__((packed));

struct WavChunkCue
{
    struct Cue
    {
        quint32 identifier = 0;
        quint32 order = 0;
        quint32 chunkID = fourCC("data");
        quint32 chunkStart = 0;
        quint32 blockStart = 0;
        quint32 offset = 0;
    } __attribute__((packed));

    quint32 cueId = fourCC("cue "); //"data"
    quint32 cueSize = 0; //M*Nc*Ns
    quint32 dwCuePoints = 0;
    QList<Cue> cues;
};

struct WavChunkFile
{
    quint32 listId = fourCC("LIST");
    quint32 listSize = 0;
    quint32 adtlId = fourCC("adtl");
    quint32 adtlSize = 0;

    quint32 fileId = fourCC("file");
    quint32 fileSize = 0;
    quint32 dwName = 0;
    quint32 dwMedType = 0;
    QByteArray data;
};

//struct WavChunkAdtl
//{

//};

class WavFile : public FileDescriptor
{
public:
    WavFile(const QString &fileName);
    WavFile(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>());
    WavFile(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>(),
            WavFormat format = WavFormat::WavFloat);
    WavFile(const QVector<Channel *> &source, const QString &fileName);
    WavFile(const QVector<Channel *> &source, const QString &fileName, WavFormat format = WavFormat::WavFloat);
    ~WavFile();

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual void write() override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(const QVector<Channel *> &) override;
    virtual int channelsCount() const override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;
    virtual Channel *channel(int index) const override;
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const override;
    virtual bool canTakeAnyChannels() const override;
    virtual QString fileType() const override;
    static QStringList fileFilters();
    static QStringList suffixes();
    virtual QString icon() const override {return ":/icons/wav.svg";}

    WavHeader m_header;
    WavChunkFmt m_fmtChunk;
    WavChunkData m_dataChunk;
    WavChunkFact m_factChunk;
    WavChunkCue m_cueChunk;
    QList<WavChunkFile> m_assocFiles;
    WavFormat m_format = WavFormat::WavFloat;
    qint64 m_dataBegin = -1;
//    juce::AudioChannelSet audioChannelSet;
    DataPrecision m_dataPrecision = DataPrecision::Float;
    bool m_valid = true;
private:
    void init(const QVector<Channel*> &source);
    bool writeWithMap(const QVector<Channel *> &source, quint32 totalSize);
    void writeWithStream(const QVector<Channel *> &source);
    WavHeader initHeader(int totalSize);
    WavChunkFmt initFmt(int channelsCount, int sampleRate);
    WavChunkFact initFact(int samplesCount);
    WavChunkData initDataHeader(int channelsCount, int samplesCount);
    WavChunkCue initCue();
    WavChunkFile initFile();

private:
    friend class WavChannel;
    QList<WavChannel*> channels;
};

class WavChannel : public Channel
{
public:
    WavChannel(WavFile *parent, const QString &name);
    WavChannel(WavFile *parent, const DataDescription &description);
    WavChannel(Channel *other, WavFile *parent);

    // Channel interface
public:
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;

private:
    WavFile *parent = nullptr;
};

#endif // WAVFILE_H
