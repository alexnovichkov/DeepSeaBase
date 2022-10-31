#ifndef WAVEXPORTER_H
#define WAVEXPORTER_H

#include <QObject>
#include <QVector>

class FileDescriptor;
class Channel;
class QProcess;

#include "fileformats/wavfile.h"

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
    WavChunkFmt initFmt(int channelsCount, int samplesCount, int sampleRate,
                        WavFormat format);
    WavChunkFact initFact(int channelsCount, int samplesCount, int sampleRate,
                          WavFormat format);
    WavChunkData initDataHeader(int channelsCount, int samplesCount, WavFormat format);
    WavChunkCue initCue(int channelsCount, int samplesCount, WavFormat format);
    WavChunkFile initFile(const QVector<int> &v);
    void finalize();
    void writeWithStreams(const QVector<int> &v, const QString &wavFileName);
    bool writeWithMap(const QVector<int> &v, const QString &wavFileName);

    FileDescriptor * file = 0;
    Channel *channel = 0;
    QVector<int> indexes;
    int count=1;
    QString _wavFile;
    bool useTemp = false;
    WavFormat format = WavFormat::WavPCM;
};



#endif // WAVEXPORTER_H
