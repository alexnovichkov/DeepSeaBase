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
    virtual QVariant info(int, bool) const override;
    virtual int columnsCount() const override;
    virtual QVariant channelHeader(int column) const override;
    virtual Descriptor::DataType type() const override;
    virtual int octaveType() const override;
    virtual void populate() override;
    virtual QString name() const override;
    virtual void setName(const QString &name) override;
    virtual QString description() const override;
    virtual void setDescription(const QString &description) override;
    virtual QString xName() const override;
    virtual QString yName() const override;
    virtual QString zName() const override;
    virtual void setYName(const QString &yName) override;
    virtual void setXName(const QString &) override;
    virtual void setZName(const QString &) override;
    virtual QString legendName() const override;
    virtual FileDescriptor *descriptor() override;
    virtual int index() const override;
    virtual QString correction() const override;
    virtual void setCorrection(const QString &) override;
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
    virtual void fillPreliminary(Descriptor::DataType) override;
    virtual void fillRest() override;
    virtual void read() override;
    virtual void write() override;
    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &) override;
    virtual QString dataDescriptorAsString() const override;
    virtual QDateTime dateTime() const override;
    virtual void deleteChannels(const QVector<int> &) override;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) override;
    virtual void calculateMean(const QList<Channel *> &) override;
    virtual QString calculateThirdOctave() override;
    virtual void calculateMovingAvg(const QList<Channel *> &, int) override;
    virtual QString saveTimeSegment(double, double) override;
    virtual int channelsCount() const override;
    virtual void move(bool, const QVector<int> &, const QVector<int> &) override;
    virtual QVariant channelHeader(int) const override;
    virtual int columnsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString legend() const override;
    virtual bool setLegend(const QString &) override;
    virtual double xStep() const override;
    virtual void setXStep(const double) override;
    virtual double xBegin() const override;
    virtual int samplesCount() const override;
    virtual void setSamplesCount(int) override;
    virtual QString xName() const override;
    virtual bool setDateTime(QDateTime) override;
    virtual bool dataTypeEquals(FileDescriptor *) const override;
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
    bool _isValid = true;
    DDCChannelGroupHandle *_groups;
};

#endif // TDMSFILE_H
