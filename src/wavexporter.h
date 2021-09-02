#ifndef WAVEXPORTER_H
#define WAVEXPORTER_H

#include <QObject>
#include <QVector>

class FileDescriptor;
class Channel;
class QProcess;

enum WavFormat {
    WavPCM = 0,
    WavExtendedPCM,
    WavFloat
};

static constexpr quint32 fourCC(const char (&ch)[5])
{
    return quint32(quint8(ch[0])) | quint32(quint8(ch[1])) << 8 | quint32(quint8(ch[2])) << 16 | quint32(quint8(ch[3])) << 24;
}

struct SimpleWavHeader
{
    const quint32 ckID = fourCC("RIFF"); //"RIFF"
    quint32 cksize = 0; //8 + 48 + 12 + (8 + M*Nc*Ns)
    const quint32 waveId = fourCC("WAVE"); //"WAVE"

    //fmt block - 20 bytes
    /*4*/const quint32 fmtId = fourCC("fmt ");	//"fmt "
    /*4*/const quint32 fmtSize = 16;
    /*2*/const quint16 wFormatTag = 1; //WAVE_FORMAT_PCM, M=2
    /*2*/quint16 nChannels = 0; //Nc
    /*4*/quint32 samplesPerSec = 0; //F
    /*4*/quint32 bytesPerSec = 0; //F*M*Nc
    /*2*/quint16 blockAlign = 0; //M*Nc, data block size, bytes
    /*2*/quint16 bitsPerSample = 0; //rounds up to 8*M

    //data block - 8 + M*Nc*Ns bytes
    const quint32 dataId = fourCC("data"); //"data"
    quint32 dataSize = 0; //M*Nc*Ns
} __attribute__((packed));

struct WavHeader
{
    const quint32 ckID = fourCC("RIFF"); //"RIFF"
    quint32 cksize = 0; //8 + 48 + 12 + (8 + M*Nc*Ns)
    const quint32 waveId = fourCC("WAVE"); //"WAVE"

    //fmt block - 48 bytes
    /*4*/const quint32 fmtId = fourCC("fmt ");	//"fmt "
    /*4*/const quint32 fmtSize = 40;
    /*2*/const quint16 wFormatTag = 0xfffe; //WAVE_FORMAT_EXTENSIBLE, M=2
    /*2*/quint16 nChannels = 0; //Nc
    /*4*/quint32 samplesPerSec = 0; //F
    /*4*/quint32 bytesPerSec = 0; //F*M*Nc
    /*2*/quint16 blockAlign = 0; //M*Nc, data block size, bytes
    /*2*/quint16 bitsPerSample = 0; //rounds up to 8*M
    /*2*/const quint16 cbSize = 22;
    /*2*/quint16 wValidBitsPerSample = 0;
    /*4*/const quint32 dwChannelMask = 0;
    //subFormat is PCM by default
    /*16*/char subFormat[16] = {'\x01','\x00','\x00','\x00','\x00','\x00',
                          '\x10','\x00','\x80','\x00','\x00','\xAA',
                          '\x00','\x38','\x9B','\x71'};

    //fact block - 12 bytes
    /*4*/const quint32 factID = fourCC("fact");
    /*4*/const quint32 factSize = 4;
    /*4*/quint32 dwSampleLength = 0; // Nc*Ns, number of samples

    //data block - 8 + M*Nc*Ns bytes
    const quint32 dataId = fourCC("data"); //"data"
    quint32 dataSize = 0; //M*Nc*Ns
} __attribute__((packed));

class WavExporter : public QObject
{
    Q_OBJECT
public:
    explicit WavExporter(FileDescriptor * file, const QVector<int> &indexes,
                         int count,
                         QObject *parent = nullptr);
    explicit WavExporter(Channel *channel, QObject *parent = nullptr);
    virtual ~WavExporter();
    void setTempFile(const QString &tempFile) {_wavFile = tempFile;}
    int chunksCount() const;
    QString wavFileName() const {return _wavFile;}
    void setFormat(WavFormat format) {this->format = format;}
signals:
    void tick();
    //void tick(int);
    void finished();
    void message(const QString &s);
    void chunksCountChanged(int count);
public slots:
    void stop();
    void start();
private:
    WavHeader initHeader(int channelsCount, int samplesCount, int sampleRate,
                    WavFormat format);
    SimpleWavHeader initSimpleHeader(int channelsCount, int samplesCount, int sampleRate,
                    WavFormat format);
    void finalize();
    void writeWithStreams(const QVector<int> &v, const QString &wavFileName);
    bool writeWithMap(const QVector<int> &v, const QString &wavFileName);

    FileDescriptor * file = 0;
    Channel *channel = 0;
    QVector<int> indexes;
    int count=1;
    QString _wavFile;
    bool useTemp = false;
    WavFormat format = WavPCM;
};

#endif // WAVEXPORTER_H
