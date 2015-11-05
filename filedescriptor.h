#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include <QObject>

namespace Descriptor {
enum DataType
{
    Unknown = 0,
    TimeResponse,
    AutoSpectrum,
    CrossSpectrum,
    FrequencyResponseFunction,
    Transmissibility,
    Coherence,
    AutoCorrelation,
    CrossCorrelation,
    PowerSpectralDensity,
    EnergySpectralDensity,
    ProbabilityDensityFunction,
    Spectrum,
    CumulativeFrequencyDistribution,
    PeaksValley,
    StressCycles,
    StrainCycles,
    Orbit,
    ModeIndicator,
    ForcePattern,
    PartialPower,
    PartialCoherence,
    Eigenvalue,
    Eigenvector,
    ShockResponseSpectrum,
    FiniteImpulseResponseFilter,
    MultipleCoherence,
    OrderFunction
};

enum OrdinateFormat {
    RealSingle = 2,
    RealDouble = 4,
    ComplexSingle = 5,
    ComplexDouble = 6
};
}


class Channel;

typedef QPair<QString, QString> DescriptionEntry;
typedef QList<DescriptionEntry> DescriptionList;

QString descriptionEntryToString(const DescriptionEntry &entry);

double threshold(const QString &name);

class SignalHandler : public QObject
{
    Q_OBJECT
signals:
    void changed(bool);
public slots:
    void setChanged(bool);
public:
    bool _changed;
};

class FileDescriptor
{
public:
    FileDescriptor(const QString &fileName);
    virtual ~FileDescriptor();

    virtual void fillPreliminary(Descriptor::DataType) = 0;
    virtual void fillRest() = 0;
    virtual void read() = 0;
    virtual void write() = 0;
    virtual void writeRawFile() = 0;
    virtual void populate() = 0;
    virtual void updateDateTimeGUID() = 0;

//    virtual Channel *newChannel(int index) = 0;

    virtual QStringList info() const = 0;
    virtual Descriptor::DataType type() const = 0;
//    virtual QString typeDescription() const = 0;
    virtual DescriptionList dataDescriptor() const = 0;
    virtual void setDataDescriptor(const DescriptionList &data) = 0;
    virtual QString dataDescriptorAsString() const = 0;

    virtual QString dateTime() const = 0;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) = 0;
    virtual void copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy) = 0;

    /** Calculates mean of channels, writes to a file */
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels) = 0;
    virtual FileDescriptor *calculateThirdOctave() = 0;

    virtual QString fileName() const {return _fileName;}
    virtual void setFileName(const QString &name) { _fileName = name;}
    virtual bool fileExists() const;

    virtual int channelsCount() const {return 0;}

    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) = 0;

    virtual bool hasAttachedFile() const = 0;
    virtual QString attachedFileName() const = 0;
    virtual void setAttachedFileName(const QString &name) = 0;

    virtual QStringList getHeadersForChannel(int channel)  = 0;

    virtual Channel *channel(int index) = 0;

    virtual void setChanged(bool changed);
    bool changed() const {return _changed;}

    bool dataChanged() const {return _dataChanged;}
    virtual void setDataChanged(bool changed);

    virtual bool allUnplotted() const = 0;

    virtual bool isSourceFile() const {return false;}

    virtual QString legend() const =0;
    virtual void setLegend(const QString &legend)=0;

    virtual double xStep() const = 0;
    virtual void setXStep(const double xStep) = 0;

    quint32 samplesCount() const {return NumInd;}
    void setSamplesCount(quint32 count) {NumInd = count;}

    virtual QString xName() const = 0;

    virtual void setDateTime(QDateTime dt) = 0;

    virtual bool operator == (const FileDescriptor &descriptor) {
        Q_UNUSED(descriptor);
        return false;
    }

    virtual bool dataTypeEquals(FileDescriptor *other) = 0;

    virtual QString fileFilters() const = 0;

    SignalHandler *signalHandler;

private:
    QString _fileName;
    bool _changed;
    bool _dataChanged;

    quint32 NumInd;
};

class Channel
{
public:
    virtual ~Channel() {}
    Channel() : _checkState(Qt::Unchecked),
                _color(QColor())
    {}

    virtual QStringList getInfoData() = 0;

    virtual Descriptor::DataType type() const = 0;
    virtual Descriptor::OrdinateFormat yFormat() const = 0;

    virtual bool populated() const = 0;
    virtual void setPopulated(bool populated) = 0;
    virtual void populate() = 0;
    virtual void clear() = 0;

    virtual QString name() const = 0;
    virtual void setName(const QString &name) = 0;

    virtual QString description() const = 0;
    virtual void setDescription(const QString &description) = 0;

    virtual QString xName() const = 0;
    virtual QString yName() const = 0;

    virtual QString legendName() const = 0;

    virtual double xBegin() const = 0;
    virtual double xStep() const = 0;
    virtual quint32 samplesCount() const = 0;
    virtual QVector<double> &yValues() = 0;
    virtual QVector<double> &xValues() = 0;
    virtual double xMaxInitial() const = 0;
    virtual double yMinInitial() const = 0;
    virtual double yMaxInitial() const = 0;

    virtual void addCorrection(double correctionValue, bool writeToFile) = 0;

    Qt::CheckState checkState() const {return _checkState;}
    void setCheckState(Qt::CheckState checkState) {_checkState = checkState;}

    QColor color() const {return _color;}
    void setColor(QColor color) {_color = color;}
private:
    Qt::CheckState _checkState;
    QColor _color;
};

QList<int> filterIndexes(FileDescriptor *dfd, const QList<QPair<FileDescriptor *, int> > &channels);

#endif // FILEDESCRIPTOR_H
