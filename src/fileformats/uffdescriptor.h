#ifndef UFFDESCRIPTOR_H
#define UFFDESCRIPTOR_H

#include "filedescriptor.h"

class UffChannel;

class Type151;
class Type164;
class Type1858;
class Type58;

class UffDescriptor : public FileDescriptor
{
public:
    UffDescriptor(const QString &fileName);
    virtual void read() override;
    virtual void write() override;


    // FileDescriptor interface
public:
    virtual void fillPreliminary(Descriptor::DataType) override;
    virtual void fillRest() override;


    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    virtual Descriptor::DataType type() const override;
    virtual QString typeDisplay() const override;
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &data) override;
    virtual QString dataDescriptorAsString() const override;
    virtual QDateTime dateTime() const override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) override;
    virtual void calculateMean(const QList<Channel *> &channels) override;
    virtual QString calculateThirdOctave() override;
    virtual void calculateMovingAvg(const QList<Channel *> &channels, int windowSize) override;
    virtual QString saveTimeSegment(double from, double to) override;
    virtual int channelsCount() const override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;
    virtual QVariant channelHeader(int column) const override;
    virtual int columnsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString legend() const override;
    virtual bool setLegend(const QString &legend) override;
    virtual double xStep() const override;
    virtual void setXStep(const double xStep) override;
    virtual double xBegin() const override;
    virtual int samplesCount() const override;
    virtual void setSamplesCount(int count) override;
    virtual QString xName() const override;
    virtual bool setDateTime(QDateTime dt) override;
    virtual bool dataTypeEquals(FileDescriptor *other) const override;
private:
    friend class UffChannel;
    QList<UffChannel*> channels;
};

class UffChannel : public Channel
{
public:
    UffChannel();

    // Channel interface
public:
    virtual QVariant info(int column, bool edit) const override;
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
    virtual QString legendName() const override;
    virtual FileDescriptor *descriptor() override;
    virtual int index() const override;
    virtual QString correction() const override;
    virtual void setCorrection(const QString &s) override;
};

#endif // UFFDESCRIPTOR_H
