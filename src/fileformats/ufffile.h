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
    void read(char *data, qint64 &offset);
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
    Function(Channel &other, UffFileDescriptor *parent);
    Function(Function &other, UffFileDescriptor *parent);
    virtual ~Function();

    void read(QTextStream &stream, qint64 pos = -1);
    void read(char *data, qint64 &offset, int size);
    void read(QDataStream &stream);
    void write(QTextStream &stream, int &id);

    FunctionHeader header;

    UffFileDescriptor *parent;
    virtual FileDescriptor *descriptor() override;
    QVector<FieldDescription> type58;
    QVector<qint64> dataPositions;
    QVector<qint64> dataEnds;
    QVector<double> zValues;

    // Channel interface
public:
    virtual int index() const override;
    virtual QVariant info(int column, bool edit) const override;
    virtual int columnsCount() const override;
    virtual QVariant channelHeader(int column) const override;

    virtual Descriptor::DataType type() const override;
    int octaveType() const override;
    virtual void populate() override;
    virtual QString name() const override;
    virtual void setName(const QString &name) override;
    virtual QString description() const override;
    virtual void setDescription(const QString &description) override;
    virtual QString xName() const override;
    virtual QString yName() const override;
    virtual QString zName() const override;
    virtual void setYName(const QString &yName) override;
    virtual void setXName(const QString &xName) override;
    virtual void setZName(const QString &zName) override;
    virtual QString legendName() const override;
    virtual int samplesCount() const override;

    virtual QString correction() const override;
    virtual void setCorrection(const QString &s) override;

//    virtual QByteArray wavData(qint64 pos, qint64 samples) override;
private:
    friend class UffFileDescriptor;
    void readRest();
    bool populateWithMmap();
    bool populateWithStream();
};

class UffHeader
{
public:
    /* Header */
    UffHeader();

    void read(QTextStream &stream);
    void read(char *pos, qint64 &offset);
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
    void read(char *pos, qint64 &offset);
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
    virtual void fillPreliminary(Descriptor::DataType) override;
    virtual void fillRest() override;
    virtual void read() override;
    virtual void write() override;
    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    QString dataDescriptorAsString() const override;

    virtual QDateTime dateTime() const override;
    virtual double xStep() const override;
    virtual void setXStep(const double xStep) override;
    virtual double xBegin() const override;

    virtual QString xName() const override;

    virtual bool setLegend(const QString &legend) override;
    virtual QString legend() const override;

    virtual bool setDateTime(QDateTime dt) override;

    bool canTakeChannelsFrom(FileDescriptor *other) const override;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes) override;
    /** Calculates mean of channels and writes to a file*/
    virtual void calculateMean(const QList<Channel *> &toMean) override;
    virtual void calculateMovingAvg(const QList<Channel *> &toAvg, int windowSize) override;
    virtual QString calculateThirdOctave() override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;

    virtual int channelsCount() const override;

    virtual QVariant channelHeader(int column) const override;
    virtual int columnsCount() const override;

    virtual Channel *channel(int index) const override;
    virtual bool operator ==(const FileDescriptor &descriptor) override;
    virtual bool dataTypeEquals(FileDescriptor *other) const override;
//    virtual bool hasSameParameters(FileDescriptor *other) const override;
    static QStringList fileFilters();
    static QStringList suffixes();
private:
    void removeTempFile();
    void readWithStreams();
    bool readWithMmap();

public:
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &data) override;
    virtual QString saveTimeSegment(double from, double to) override;
    virtual int samplesCount() const override;
    virtual void setSamplesCount(int count) override;
    virtual void setChanged(bool changed) override;
};

#endif // UFFFILE_H
