#ifndef TIMESLICER_H
#define TIMESLICER_H

#include <QObject>

class FileDescriptor;
class QProcess;

class TimeSlicer : public QObject
{
    Q_OBJECT
public:
    explicit TimeSlicer(const QList<FileDescriptor *> &files, double from, double to, QObject *parent = nullptr);
    virtual ~TimeSlicer();

    QStringList getNewFiles() const {return newFiles;}
signals:
    void tick(int);
    void finished();
    void message(const QString &s);
public slots:
    void stop();
    void start();
private:
    void finalize();

    QList<FileDescriptor *> dataBase;
    bool stop_;
    QStringList newFiles;
    double from, to;
};

#endif // TIMESLICER_H
