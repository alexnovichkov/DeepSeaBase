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

//QString valuesUnit(const QString &old, int unitType);

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

    virtual Descriptor::DataType type() const = 0;
    virtual QString typeDisplay() const = 0;
    virtual double roundedSize() const;
    virtual DescriptionList dataDescriptor() const = 0;
    virtual void setDataDescriptor(const DescriptionList &data) = 0;
    virtual QString dataDescriptorAsString() const = 0;

    virtual QDateTime dateTime() const = 0;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) = 0;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) = 0;

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

    virtual QVariant channelHeader(int column) const = 0;
    virtual int columnsCount() const = 0;

    virtual Channel *channel(int index) const = 0;

    virtual void setChanged(bool changed);
    bool changed() const {return _changed;}

    bool dataChanged() const {return _dataChanged;}
    virtual void setDataChanged(bool changed);

    int plottedCount() const;

    virtual bool isSourceFile() const {return false;}

    virtual QString legend() const =0;
    virtual bool setLegend(const QString &legend)=0;

    virtual double xStep() const = 0;
    virtual void setXStep(const double xStep) = 0;

    virtual int samplesCount() const = 0;//{return NumInd;}
    virtual void setSamplesCount(int count) = 0; //{NumInd = count;}

    virtual QString xName() const = 0;

    virtual bool setDateTime(QDateTime dt) = 0;

    virtual bool operator == (const FileDescriptor &descriptor) {
        Q_UNUSED(descriptor);
        return false;
    }

    virtual bool dataTypeEquals(FileDescriptor *other) const = 0;

    virtual QString fileFilters() const = 0;
    bool hasCurves() const;
private:
    QString _fileName;
    bool _changed;
    bool _dataChanged;
};

class Channel
{
public:
    virtual ~Channel() {
        delete _data;
    }
    Channel() : _color(QColor()),
                _plotted(0),
                _populated(false),
                _data(new DataHolder)
    {}
    Channel(Channel *other);
    Channel(Channel &other);

    virtual QVariant info(int column, bool edit) const = 0;
    virtual int columnsCount() const = 0;
    virtual QVariant channelHeader(int column) const = 0;

    virtual Descriptor::DataType type() const = 0;

    virtual bool populated() const {return _populated;}
    virtual void setPopulated(bool populated) {_populated = populated;}
    virtual void populate() = 0;
    virtual void clear();
    virtual void maybeClearData();

    virtual QString name() const = 0;
    virtual void setName(const QString &name) = 0;

    virtual QString description() const = 0;
    virtual void setDescription(const QString &description) = 0;

    virtual QString xName() const = 0;
    virtual QString yName() const = 0;
    virtual void setYName(const QString &yName) = 0;

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

    virtual QVector<double> yData() const {return _data->rawYValues();}
    virtual const double *xData() const {return _data->rawXValues();}

    virtual double yMin() const {return _data->yMin();}
    virtual double yMax() const {return _data->yMax();}

    virtual FileDescriptor *descriptor() = 0;

    virtual int index() const = 0;

    QColor color() const {return _color;}
    void setColor(QColor color) {_color = color;}

    virtual QString correction() const = 0;
    virtual void setCorrection(const QString &s) = 0;

    int plotted() const {return _plotted;}
    void setPlotted(int plotted) {_plotted = plotted;}
private:
    QColor _color;
    int _plotted; //0=not plotted, 1=plotted on left, 2=plotted on right
    bool _populated;
protected:
    DataHolder *_data;
};

#endif // FILEDESCRIPTOR_H
