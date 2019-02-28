#ifndef UFFFILE_H
#define UFFFILE_H

#include <QtCore>

#include "filedescriptor.h"
#include "fields.h"
#include "algorithms.h"

class DataHolder;

int abscissaType(const QString &xName);

class FunctionHeader
{
public:
    FunctionHeader();
    void read(QTextStream &stream);
    void write(QTextStream &stream);

    QVector<FieldDescription> type1858;
    bool valid;
};

QDataStream & operator>> (QDataStream& stream, FunctionHeader& header);
QDataStream & operator<< (QDataStream& stream, const FunctionHeader& header);

class UffFileDescriptor;

class Function : public Channel
{
public:
    Function(UffFileDescriptor *parent);
    Function(Channel &other);
    Function(Function &other);
    virtual ~Function();

    void read(QTextStream &stream, qint64 pos = -1);
    void read(QDataStream &stream);
    void write(QTextStream &stream);

    static QString functionTypeDescription(int type);

    FunctionHeader header;

    UffFileDescriptor *parent;
    virtual FileDescriptor *descriptor();
    QVector<FieldDescription> type58;
    qint64 dataPosition;

    // Channel interface
public:
    virtual QStringList getInfoHeaders();
    virtual QStringList getInfoData();
    virtual Descriptor::DataType type() const;
    virtual Descriptor::OrdinateFormat yFormat() const;
    virtual bool populated() const {return _populated;}
    virtual void setPopulated(bool populated) {_populated = populated;}
    virtual void populate();
    virtual QString name() const;
    virtual void setName(const QString &name);
    virtual QString description() const;
    virtual void setDescription(const QString &description);
    virtual QString xName() const;
    virtual QString yName() const;
    virtual void setYName(const QString &yName) override;
    virtual QString legendName() const;
    virtual int samplesCount() const;

    virtual QString correction() const;
    virtual void setCorrection(const QString &s);
private:
    void readRest();
    bool _populated;
};

class UffHeader
{
public:
    /* Header */
    UffHeader();

    void read(QTextStream &stream);
    void write(QTextStream &stream);
    QString info() const;

    QVector<FieldDescription> type151;

};

QDataStream &operator>>(QDataStream& stream, UffHeader& header);
QDataStream &operator<<(QDataStream& stream, const UffHeader& header);

class UffUnits
{
public:
    UffUnits();

    void read(QTextStream &stream);
    void write(QTextStream &stream);

    QVector<FieldDescription> type164;
};

QDataStream &operator>>(QDataStream& stream, UffUnits& header);
QDataStream &operator<<(QDataStream& stream, const UffUnits& header);

class UffFileDescriptor : public FileDescriptor
{
public:
    UffFileDescriptor(const QString &fileName);
    UffFileDescriptor(const UffFileDescriptor &other);
    UffFileDescriptor(const FileDescriptor &other);
    ~UffFileDescriptor();

    UffHeader header;
    UffUnits units;
    QList<Function *> channels;

    // FileDescriptor interface
public:
    virtual void fillPreliminary(Descriptor::DataType);
    virtual void fillRest();
    virtual void read();
    virtual void write();
    virtual void writeRawFile();
    virtual void updateDateTimeGUID();
    QString dataDescriptorAsString() const;

    QMap<QString, QString> info() const;
    virtual Descriptor::DataType type() const;
    virtual QString typeDisplay() const;
    virtual QDateTime dateTime() const;
    virtual double xStep() const;
    virtual void setXStep(const double xStep);
    virtual QString xName() const;

    virtual bool setLegend(const QString &legend);
    virtual QString legend() const;

    void setDateTime(QDateTime dt);


    virtual void deleteChannels(const QVector<int> &channelsToDelete);
    virtual void copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy);
    /** Calculates mean of channels and writes to a file*/
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels);
    virtual void calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels,
                                    int windowSize);
    virtual QString calculateThirdOctave();
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes);

    virtual int channelsCount() const;
    virtual bool hasAttachedFile() const;
    virtual QString attachedFileName() const;
    virtual void setAttachedFileName(const QString &name);
    virtual QStringList getHeadersForChannel(int channel);
    virtual Channel *channel(int index) const;
    virtual bool isSourceFile() const;
    virtual bool operator ==(const FileDescriptor &descriptor);
    virtual bool dataTypeEquals(FileDescriptor *other) const;
    virtual QString fileFilters() const;
private:

    int NumInd;

    // FileDescriptor interface
    void removeTempFile();
    
public:
    virtual DescriptionList dataDescriptor() const;
    virtual void setDataDescriptor(const DescriptionList &data);
    virtual QString saveTimeSegment(double from, double to);
    virtual int samplesCount() const;
    virtual void setSamplesCount(int count);
    virtual void setChanged(bool changed);
};

#endif // UFFFILE_H
