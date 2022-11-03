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
    QString wavFileName() const {return _wavFile;}
    void setFormat(WavFormat format) {this->format = format;}
    void setSimplified(bool simplified) {this->simplified = simplified;}
signals:
    void tick();
    //void tick(int);
    void finished();
    void message(const QString &s);
public slots:
    void stop();
    void start();
private:
    void finalize();

    FileDescriptor * file = 0;
    Channel *channel = 0;
    QVector<int> indexes;
    int count=1;
    QString _wavFile;
    bool useTemp = false;
    WavFormat format = WavFormat::WavPCM;
    bool simplified = false;
};



#endif // WAVEXPORTER_H
