#ifndef WAVEXPORTER_H
#define WAVEXPORTER_H

#include <QObject>
#include <QVector>

class FileDescriptor;
class QProcess;

class WavExporter : public QObject
{
    Q_OBJECT
public:
    explicit WavExporter(FileDescriptor * file, const QVector<int> &indexes, QObject *parent = nullptr);
    virtual ~WavExporter();
    int chunksCount() const;
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
    void writeWithMap(const QString &wavFileName);

    FileDescriptor * file;
    QVector<int> indexes;
};

#endif // WAVEXPORTER_H
