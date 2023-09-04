#ifndef WAVIO_H
#define WAVIO_H

#include "fileio.h"

#include "wavfile.h"

class WavIO : public FileIO
{
    Q_OBJECT
public:
    WavIO(const QVector<Channel*> &source, QString fileName, QObject *parent = nullptr,
          const QMap<QString, QVariant> & parameters = {});
    ~WavIO();
    void setFormat(WavFormat format);
private:
    WavChunkFmt initFmt(int channelsCount, int sampleRate);

    WavFormat m_format = WavFormat::WavPCM;
    quint32 samples;
    quint16 channelsCount;
    quint32 sampleRate;
    quint16 M;

    WavHeader m_header;
    WavChunkFmt m_fmtChunk;
    WavChunkData m_dataChunk;
    WavChunkFact m_factChunk;

    bool m_headerWritten = false;

private:
    QFile *wavFile = nullptr;
    uchar *mapped = nullptr;
    int currentChannelsCount=0;
    quint64 dataPos = 0;

    // FileIO interface
public:
    virtual void addChannel(DataDescription *description, DataHolder *data) override;
    virtual void finalize() override;
};

#endif // WAVIO_H
