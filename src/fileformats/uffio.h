#ifndef UFFIO_H
#define UFFIO_H

#include "fileio.h"
#include "ufffile.h"

class UffIO : public FileIO
{
    Q_OBJECT
public:
    UffIO(const QVector<Channel*> &source, QString fileName, QObject *parent = nullptr,
          const QMap<QString, QVariant> & parameters = {});

    // FileIO interface
public:
    virtual void addChannel(DataDescription *description, DataHolder *data) override;
    virtual void finalize() override;
private:
    int referenceChannelNumber = -1; //номер опорного канала ("сила")
    QString referenceChannelName;
    int id=1;
    QList<AbstractField*> fields {
        new DelimiterField,
        new Float13_5Field,
        new Float15_7Field,
        new Float20_12Field,
        new Float25_17Field,
        new Integer4Field,
        new Integer5Field,
        new Integer6Field,
        new Integer10Field,
        new Integer12Field,
        new String80Field,
        new String10Field,
        new String10aField,
        new String20Field,
        new TimeDateField,
        new TimeDate80Field,
        new EmptyField
    };
};

#endif // UFFIO_H
