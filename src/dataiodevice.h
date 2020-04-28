#ifndef DATAIODEVICE_H
#define DATAIODEVICE_H

#include <QIODevice>

class Channel;

class DataIODevice : public QIODevice
{
public:
    DataIODevice(Channel *channel, QObject *parent = 0);
    void setVolume(double volume) {m_volume = volume;} //0.0 - 1.0
    double volume() const {return m_volume;}
    bool muted() const {return m_muted;}
    void setMuted(bool muted) {m_muted = muted;}

    // QIODevice interface
public:
    virtual bool isSequential() const override;
    double positionSec() const;
    virtual bool seek(qint64 pos) override;
    virtual bool atEnd() const override;
    virtual bool reset() override;
    virtual bool canReadLine() const override;
    virtual qint64 size() const override;

protected:
    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual qint64 writeData(const char *data, qint64 len) override;
private:
    Channel *m_channel;
    qint64 m_pos = 0;
    double m_volume = 1.0;
    bool m_muted = false;
};

#endif // DATAIODEVICE_H
