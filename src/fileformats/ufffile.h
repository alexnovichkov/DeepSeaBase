#ifndef UFFFILE_H
#define UFFFILE_H

#include <QtCore>

#include "fileformats/filedescriptor.h"
#include "fields.h"
#include "algorithms.h"

class DataHolder;

int abscissaType(const QString &xName);
QString abscissaTypeDescription(int type);

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
    void write(QTextStream &stream, int &id);

    FunctionHeader header;

    UffFileDescriptor *parent;
    virtual FileDescriptor *descriptor();
    QVector<FieldDescription> type58;
    QVector<qint64> dataPositions;
    QVector<double> zValues;

    // Channel interface
public:
    virtual int index() const override;
    virtual QVariant info(int column, bool edit) const;
    virtual int columnsCount() const;
    virtual QVariant channelHeader(int column) const;

    virtual Descriptor::DataType type() const;
    int octaveType() const override;
    virtual void populate();
    virtual QString name() const;
    virtual void setName(const QString &name);
    virtual QString description() const;
    virtual void setDescription(const QString &description);
    virtual QString xName() const;
    virtual QString yName() const;
    virtual QString zName() const;
    virtual void setYName(const QString &yName) override;
    virtual QString legendName() const;
    virtual int samplesCount() const;

    virtual QString correction() const;
    virtual void setCorrection(const QString &s);

//    virtual QByteArray wavData(qint64 pos, qint64 samples) override;
private:
    friend class UffFileDescriptor;
    void readRest();
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
    UffFileDescriptor(const UffFileDescriptor &other, const QString &fileName,
                      QVector<int> indexes = QVector<int>());
    UffFileDescriptor(const FileDescriptor &other, const QString &fileName,
                      QVector<int> indexes = QVector<int>());
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

    virtual Descriptor::DataType type() const;
    virtual QString typeDisplay() const;
    virtual QDateTime dateTime() const;
    virtual double xStep() const;
    virtual void setXStep(const double xStep);
    virtual double xBegin() const override;

    virtual QString xName() const;

    virtual bool setLegend(const QString &legend);
    virtual QString legend() const;

    virtual bool setDateTime(QDateTime dt) override;

    bool canTakeChannelsFrom(FileDescriptor *other) const override;

    virtual void deleteChannels(const QVector<int> &channelsToDelete);
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes);
    /** Calculates mean of channels and writes to a file*/
    virtual void calculateMean(const QList<Channel *> &toMean);
    virtual void calculateMovingAvg(const QList<Channel *> &toAvg, int windowSize);
    virtual QString calculateThirdOctave();
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes);

    virtual int channelsCount() const;

    virtual QVariant channelHeader(int column) const;
    virtual int columnsCount() const;

    virtual Channel *channel(int index) const;
    virtual bool isSourceFile() const;
    virtual bool operator ==(const FileDescriptor &descriptor);
    virtual bool dataTypeEquals(FileDescriptor *other) const;
//    virtual bool hasSameParameters(FileDescriptor *other) const override;
    static QStringList fileFilters();
    static QStringList suffixes();
private:
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
