#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include <QObject>
#include "dataholder.h"

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
class DataHolder;

typedef QPair<QString, QString> DescriptionEntry;
typedef QList<DescriptionEntry> DescriptionList;

QString descriptionEntryToString(const DescriptionEntry &entry);

double threshold(const QString &name);
double convertFactor(const QString &from);

QString valuesUnit(const QString &old, int unitType);

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
    virtual void populate();
    virtual void updateDateTimeGUID() = 0;

    virtual QMap<QString, QString> info() const = 0;
    virtual Descriptor::DataType type() const = 0;
    virtual QString typeDisplay() const = 0;
    virtual double size() const;
    virtual DescriptionList dataDescriptor() const = 0;
    virtual void setDataDescriptor(const DescriptionList &data) = 0;
    virtual QString dataDescriptorAsString() const = 0;

    virtual QDateTime dateTime() const = 0;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) = 0;
    virtual void copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy) = 0;

    /** Calculates mean of channels, writes to a file */
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels) = 0;
    virtual QString calculateThirdOctave() = 0;
    virtual void calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels,
                                    int windowSize) = 0;
    virtual QString saveTimeSegment(double from, double to) = 0;

    virtual QString fileName() const {return _fileName;}
    virtual void setFileName(const QString &name) { _fileName = name;}
    virtual bool fileExists() const;

    virtual int channelsCount() const {return 0;}

    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) = 0;

    virtual bool hasAttachedFile() const = 0;
    virtual QString attachedFileName() const = 0;
    virtual void setAttachedFileName(const QString &name) = 0;

    virtual QStringList getHeadersForChannel(int channel)  = 0;

    virtual Channel *channel(int index) const = 0;

    virtual void setChanged(bool changed);
    bool changed() const {return _changed;}

    bool dataChanged() const {return _dataChanged;}
    virtual void setDataChanged(bool changed);

    virtual bool allUnplotted() const;

    virtual bool isSourceFile() const {return false;}

    virtual QString legend() const =0;
    virtual bool setLegend(const QString &legend)=0;

    virtual double xStep() const = 0;
    virtual void setXStep(const double xStep) = 0;

    virtual int samplesCount() const = 0;//{return NumInd;}
    virtual void setSamplesCount(int count) = 0; //{NumInd = count;}

    virtual QString xName() const = 0;

    virtual void setDateTime(QDateTime dt) = 0;

    virtual bool operator == (const FileDescriptor &descriptor) {
        Q_UNUSED(descriptor);
        return false;
    }

    virtual bool dataTypeEquals(FileDescriptor *other) const = 0;

    virtual QString fileFilters() const = 0;
    bool hasGraphs() const;
//    void setHasGraphs(bool hasGraphs) {_hasGraphs = hasGraphs;}
private:
    QString _fileName;
    bool _changed;
    bool _dataChanged;
    bool _hasGraphs;
};

class Channel
{
public:
    virtual ~Channel() {
        delete _data;
    }
    Channel() : _checkState(Qt::Unchecked),
                _color(QColor()),
                _data(new DataHolder),
                temporalCorrection(false)
    {}
    Channel(Channel *other);
    Channel(Channel &other);

    virtual QStringList getInfoData() = 0;

    virtual Descriptor::DataType type() const = 0;
    virtual Descriptor::OrdinateFormat yFormat() const = 0;

    virtual bool populated() const = 0;
    virtual void setPopulated(bool populated) = 0;
    virtual void populate() = 0;
    virtual void clear();
    virtual void maybeClearData();

    virtual QString name() const = 0;
    virtual void setName(const QString &name) = 0;

    virtual QString description() const = 0;
    virtual void setDescription(const QString &description) = 0;

    virtual QString xName() const = 0;
    virtual QString yName() const = 0;

    virtual QString legendName() const = 0;

    DataHolder *data() {return _data;}
    const DataHolder *data() const {return _data;}

    virtual double xMin() const;
    virtual double xMax() const;
    virtual double xStep() const {return _data->xStep();}
    virtual int samplesCount() const {return _data->samplesCount();}
    virtual int xValuesFormat() const {return _data->xValuesFormat();}
    virtual int yValuesFormat() const {return _data->yValuesFormat();}
    virtual int units() const {return _data->yValuesUnits();}

    virtual QVector<double> yValues() const {return _data->yValues();}
    virtual QVector<cx_double> yValuesComplex() const {return _data->yValuesComplex();}
    virtual QVector<double> xValues() const {return _data->xValues();}

    virtual const double *yData() const {return _data->rawYValues();}
    virtual const double *xData() const {return _data->rawXValues();}

    virtual double yMin() const {return _data->yMin();}
    virtual double yMax() const {return _data->yMax();}

    virtual void addCorrection(double correctionValue, int type, bool writeToFile);
    virtual FileDescriptor *descriptor() = 0;

    Qt::CheckState checkState() const {return _checkState;}
    void setCheckState(Qt::CheckState checkState) {_checkState = checkState;}

    QColor color() const {return _color;}
    void setColor(QColor color) {_color = color;}

    bool correct() const {return !temporalCorrection;}
    void setCorrect(bool correct) {temporalCorrection = !correct;}
private:
    Qt::CheckState _checkState;
    QColor _color;
protected:
    DataHolder *_data;

    //correction variables
    bool temporalCorrection;
};

QList<int> filterIndexes(FileDescriptor *dfd, const QList<QPair<FileDescriptor *, int> > &channels);

#endif // FILEDESCRIPTOR_H
