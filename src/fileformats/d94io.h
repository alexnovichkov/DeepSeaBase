#ifndef D94IO_H
#define D94IO_H

#include "fileio.h"

class D94IO : public FileIO
{
    Q_OBJECT
public:
    D94IO(const QVector<Channel*> &source,
          const QString &fileName, QObject *parent = nullptr);
    ~D94IO() {}

    // FileIO interface
public:
    virtual void addChannel(Channel *channel) override;
    virtual void addChannel(DataDescription *description, DataHolder *data) override;
    virtual void finalize() override;
};

#endif // D94IO_H
