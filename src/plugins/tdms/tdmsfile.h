#ifndef TDMSFILE_H
#define TDMSFILE_H

#include <QList>
#include <QFlag>
#include <QVariant>
#include <QObject>


#include "nilibddc.h"

#include "fileformats/filedescriptor.h"

class TDMSGroup;
class TDMSChannel : public Channel
{
public:
    TDMSChannel(DDCChannelHandle channel, TDMSGroup *parent);
    ~TDMSChannel();
    QVector<double> getDouble();
    QVariantMap properties;
    quint64 numberOfValues = 0;
    DDCDataType dataType;
private:
    DDCChannelHandle channel;
    TDMSGroup *parent;

    // Channel interface
public:
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;
};

class TDMSFile;
class TDMSGroup : public FileDescriptor
{
public:
    TDMSGroup(DDCChannelGroupHandle group, const QString &name);
    ~TDMSGroup();
    QList<TDMSChannel*> channels;
    QVariantMap properties;
    TDMSFile *parent;
private:
    DDCChannelHandle *_channels;
    DDCChannelGroupHandle group;

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual void write() override;
    virtual void copyChannelsFrom(const QVector<Channel*> &) override {}
    virtual int channelsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString fileType() const override {return "tdms";}
};

//может содержать несколько групп, поэтому рассматриваем как директорию
class TDMSFile
{
public:
    TDMSFile(const QString &fileName);
    ~TDMSFile();
    bool isValid() const {return _isValid;}
    QList<TDMSGroup*> groups;
    QVariantMap properties;
private:
    QString fileName;
    DDCFileHandle file;
    bool _isValid = false;
    DDCChannelGroupHandle *_groups;
};

#endif // TDMSFILE_H
