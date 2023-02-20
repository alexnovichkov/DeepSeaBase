#ifndef DFDIO_H
#define DFDIO_H

#include "fileio.h"

class DfdIO : public FileIO
{
    Q_OBJECT
public:
    DfdIO(const QVector<Channel*> &source,
          const QString &fileName, QObject *parent = nullptr);

    // FileIO interface
public:
    virtual void addChannel(DataDescription *description, DataHolder *data) override;
    virtual void finalize() override;
private:
    int channelsCount=0;
};

#endif // DFDIO_H
