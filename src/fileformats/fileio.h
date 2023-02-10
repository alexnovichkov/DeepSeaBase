#ifndef FILEIO_H
#define FILEIO_H

class FileDescriptor;
class Channel;

#include <QObject>

class FileIO : public QObject
{
    Q_OBJECT
public:
    FileIO(FileDescriptor *file, QObject *parent = nullptr);
    virtual ~FileIO() {}

    virtual bool writeHeader(FileDescriptor *file) = 0;
    virtual void writeChannel(Channel *channel) = 0;
signals:
    void tick();
protected:
    FileDescriptor *m_file = nullptr;
};

#endif // FILEIO_H
