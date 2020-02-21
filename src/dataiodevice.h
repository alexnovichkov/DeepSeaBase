#ifndef DATAIODEVICE_H
#define DATAIODEVICE_H

#include <QIODevice>

class Channel;

class DataIODevice : public QIODevice
{
public:
    DataIODevice(Channel *channel, QObject *parent = 0);

    // QIODevice interface
public:
    virtual bool isSequential() const override;
    virtual bool open(OpenMode mode) override;
    virtual void close() override;
    qint64 position() const {return m_pos;}
    double positionSec() const;
    virtual qint64 pos() const override;
    virtual qint64 size() const override;
    virtual bool seek(qint64 pos) override;
    virtual bool atEnd() const override;
//    virtual bool reset() override;
//    virtual qint64 bytesAvailable() const override;
//    virtual qint64 bytesToWrite() const override;
    virtual bool canReadLine() const override;

protected:
    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual qint64 writeData(const char *data, qint64 len) override;
private:
    Channel *m_channel;
    qint64 m_pos = 0;
};

#endif // DATAIODEVICE_H
