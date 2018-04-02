#ifndef UFFFILE_H
#define UFFFILE_H

#include <QtCore>

#include "filedescriptor.h"
#include "fields.h"
#include <complex>

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

class UffFileDescriptor;

class Function : public Channel
{
public:
    Function(UffFileDescriptor *parent);
    Function(Channel &other);
    Function(const Function &other);
    virtual ~Function();

    void read(QTextStream &stream, qint64 pos = -1);
    void write(QTextStream &stream);

    QString functionTypeDescription() const;

    FunctionHeader header;

    double xMax;
    double yMin;
    double yMax;

    quint32 samples;

    QVector<double> values;
    QVector<double> xvalues;
    QVector<QPair<double,double> > valuesComplex;

    UffFileDescriptor *parent;
    virtual FileDescriptor *descriptor();
    QVector<FieldDescription> type58;

    // Channel interface
public:
    virtual QStringList getInfoHeaders();
    virtual QStringList getInfoData();
    virtual Descriptor::DataType type() const;
    virtual Descriptor::OrdinateFormat yFormat() const;
    virtual bool populated() const {return _populated;}
    virtual void setPopulated(bool populated) {_populated = populated;}
    virtual void populate();
    virtual void clear();
    virtual QString name() const;
    virtual void setName(const QString &name);
    virtual QString description() const;
    virtual void setDescription(const QString &description);
    virtual QString xName() const;
    virtual QString yName() const;
    virtual QString legendName() const;
    virtual double xBegin() const;
    virtual double xStep() const;
    virtual quint32 samplesCount() const;
    virtual QVector<double> &yValues();
    virtual QVector<double> &xValues();
    virtual double xMaxInitial() const;
    virtual double yMinInitial() const;
    virtual double yMaxInitial() const;
    virtual void addCorrection(double correctionValue, bool writeToFile);
private:
    bool _populated;
    qint64 dataPosition;

    //correction variables
    bool temporalCorrection;
    QString nameBeforeCorrection;
    double oldCorrectionValue;
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

class UffUnits
{
public:
    UffUnits();

    void read(QTextStream &stream);
    void write(QTextStream &stream);

    QVector<FieldDescription> type164;
};

class UffFileDescriptor : public FileDescriptor
{
public:
    UffFileDescriptor(const QString &fileName);
    ~UffFileDescriptor();

    UffHeader header;
    UffUnits units;
    QList<Function *> channels;
    QVector<qint64> functionPositions;

    // FileDescriptor interface
public:
    virtual void fillPreliminary(Descriptor::DataType);
    virtual void fillRest();
    virtual void read();
    virtual void write();
    virtual void writeRawFile();
    virtual void populate();
    virtual void updateDateTimeGUID();
    virtual QList<QPair<QString, QString> > dataDescriptor() const;
    virtual void setDataDescriptor(const QList<QPair<QString, QString> > &data);
    QString dataDescriptorAsString() const;

    virtual QStringList info() const;
    virtual Descriptor::DataType type() const;
    virtual QString dateTime() const;
    virtual double xStep() const;
    virtual void setXStep(const double xStep);
    virtual QString xName() const;

    virtual void setLegend(const QString &legend);
    virtual QString legend() const;

    void setDateTime(QDateTime dt);


    virtual void deleteChannels(const QVector<int> &channelsToDelete);
    virtual void copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy);
    /** Calculates mean of channels and writes to a file*/
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels);
    virtual void calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels,
                                    int windowSize);
    virtual FileDescriptor *calculateThirdOctave();
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes);

    virtual int channelsCount() const;
    virtual bool hasAttachedFile() const;
    virtual QString attachedFileName() const;
    virtual void setAttachedFileName(const QString &name);
    virtual QStringList getHeadersForChannel(int channel);
    virtual Channel *channel(int index);
    virtual bool allUnplotted() const;
    virtual bool isSourceFile() const;
    virtual bool operator ==(const FileDescriptor &descriptor);
    virtual bool dataTypeEquals(FileDescriptor *other) const;
    virtual QString fileFilters() const;
private:
    QString fileType() const;
};

#endif // UFFFILE_H
