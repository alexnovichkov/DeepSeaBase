#ifndef WAVFILE_H
#define WAVFILE_H

#include "filedescriptor.h"

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
    /*4*/quint32 fmtSize = 40;
    /*2*/quint16 wFormatTag = 0xfffe; //WAVE_FORMAT_EXTENSIBLE, M=2
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
    /*4*//*const*/ quint32 factSize = 4;
    /*4*/quint32 dwSampleLength = 0; // Nc*Ns, number of samples
}__attribute__((packed));

struct WavChunkData
{
    //data block - 8 + M*Nc*Ns bytes
    /*const*/ quint32 dataId = fourCC("data"); //"data"
    quint32 dataSize = 0; //M*Nc*Ns
}__attribute__((packed));

class WavFile : public FileDescriptor
{
public:
    WavFile(const QString &fileName);
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
    virtual QString fileType() const override;
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const override;
    virtual bool canTakeAnyChannels() const override;

    WavHeader *m_header = nullptr;
    WavChunkFmt *m_fmtChunk = nullptr;
    WavChunkData *m_dataChunk = nullptr;
    WavChunkFact *m_factChunk = nullptr;
    WavFormat m_format = WavFormat::WavExtendedPCM;
    qint64 dataBegin = -1;
    quint64 dataSize = 0;
private:
    friend class WavChannel;
    QList<WavChannel*> channels;
};

#endif // WAVFILE_H
