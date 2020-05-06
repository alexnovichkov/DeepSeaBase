#ifndef WAVEXPORTER_H
#define WAVEXPORTER_H

#include <QObject>
#include <QVector>

class FileDescriptor;
class Channel;
class QProcess;

class WavExporter : public QObject
{
    Q_OBJECT
public:
    explicit WavExporter(FileDescriptor * file, const QVector<int> &indexes, QObject *parent = nullptr);
    explicit WavExporter(Channel *channel, QObject *parent = nullptr);
    virtual ~WavExporter();
    void setTempFile(const QString &tempFile) {_wavFile = tempFile;}
    int chunksCount() const;
    QString wavFileName() const {return _wavFile;}
signals:
    void tick(int);
    void finished();
    void message(const QString &s);
public slots:
    void stop();
    void start();
private:
    void finalize();
    void writeWithStreams(const QString &wavFileName);
    bool writeWithMap(const QString &wavFileName);

    FileDescriptor * file = 0;
    Channel *channel = 0;
    QVector<int> indexes;
    QString _wavFile;
    bool useTemp = false;
};

#endif // WAVEXPORTER_H
