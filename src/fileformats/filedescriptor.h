#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include <QObject>
#include "dataholder.h"
#include "boost/property_tree/ptree.hpp"

namespace Descriptor {
enum DataType
{
    Unknown = 0,
    TimeResponse = 1,
    AutoSpectrum = 2,
    CrossSpectrum = 3,
    FrequencyResponseFunction = 4,
    Transmissibility = 5,
    Coherence = 6,
    AutoCorrelation = 7,
    CrossCorrelation = 8,
    PowerSpectralDensity = 9,
    EnergySpectralDensity = 10,
    ProbabilityDensityFunction = 11,
    Spectrum = 12,
    CumulativeFrequencyDistribution = 13,
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
    FiniteImpulseResponseFilter = 25,
    MultipleCoherence,
    OrderFunction
};

QString functionTypeDescription(int type);

}

struct DataDescription
{
    QVariantMap data;

    void put(const QString &key, const QVariant &value) {
        data.insert(key, value);
    }
    QVariant get(const QString &key) const
    {
        return data.value(key);
    }
    QJsonObject toJson() const {
        QJsonObject result;
        for (auto i = data.constBegin(); i != data.constEnd(); ++i) {
            if (i.key().contains('.')) {
                QString key = i.key().section('.',0,0);
                auto r = result.value(key).toObject();
                r.insert(i.key().section('.',1), QJsonValue::fromVariant(i.value()));
                result.insert(key, r);
            }
            else result.insert(i.key(), QJsonValue::fromVariant(i.value()));
        }
        return result;
    }
    static DataDescription fromJson(const QJsonObject &o) {
        DataDescription result;
        for (auto i = o.constBegin(); i!=o.constEnd(); ++i) {
            QString key = i.key();
            QJsonValue val = i.value();
            if (val.isArray()) {
                qDebug()<<"Array found at"<<key;
                continue;
            }
            else if (val.isObject()) {
                QJsonObject v = val.toObject();
                for (auto j = v.constBegin(); j!=v.constEnd(); ++j) {
                    if (j->isArray()) {
                        qDebug()<<"Array found at"<<j.key();
                        continue;
                    }
                    else if (j->isObject()) {
                        qDebug()<<"Object found at"<<j.key();
                        continue;
                    }
                    QString key1 = key+"."+j.key();
                    result.data.insert(key1, j->toVariant());
                }
            }
            else
                result.data.insert(key, val.toVariant());
        }
        return result;
    }

    QStringList twoStringDescription() const
    {
        QStringList result = toStringList("description");
        result = result.mid(0,2);
        return result;
    }
    QStringList toStringList(const QString &filter = QString()) const
    {
        QStringList result;

        for (auto i = data.constBegin(); i != data.constEnd(); ++i) {
            if (filter.isEmpty() || i.key().startsWith(filter+"."))
                result << i.key()+"="+i.value().toString();
        }
        return result;
    }
};

QDataStream &operator>>(QDataStream& stream, DataDescription& header);
QDataStream &operator<<(QDataStream& stream, const DataDescription& header);


class Channel;
class DataHolder;

typedef QPair<QString, QString> DescriptionEntry;
typedef QList<DescriptionEntry> DescriptionList;

//QString descriptionEntryToString(const DescriptionEntry &entry);

double threshold(const QString &name);
double convertFactor(const QString &from);
QString stringify(const QVector<int> &vec);

//QString valuesUnit(const QString &old, int unitType);

class FileDescriptor
{
public:
    FileDescriptor(const QString &fileName);
    virtual ~FileDescriptor();

    virtual bool rename(const QString &newName, const QString &newPath = QString());

    //просто вызывает updateDateTimeGUID. Переопределен в DFD
    virtual void fillPreliminary(const FileDescriptor *);
    virtual void read() = 0;
    virtual void write() = 0;
    void populate();
    void updateDateTimeGUID();
    //переопределен в DFD
    virtual bool copyTo(const QString &name);
    //переопределен в DFD
    virtual Descriptor::DataType type() const;
    //переопределен в DFD
    virtual QString typeDisplay() const;
    double roundedSize() const;

    const DataDescription & dataDescription() const {return _dataDescription;}
    DataDescription & dataDescription() {return _dataDescription;}
    void setDataDescription(const DataDescription &descr) {_dataDescription = descr;}

    //дата и время создания базы данных
    QDateTime dateTime() const;
    bool setDateTime(const QDateTime &dt);

    //дата и время создания этого файла
    QDateTime fileCreationTime() const;
    bool setFileCreationTime(const QDateTime &dt);

    QString legend() const;
    bool setLegend(const QString &s);

    //QString guid() const;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) = 0;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) = 0;

    /** Calculates average of channels, writes to a file */
    void calculateMean(const QList<Channel*> &channels);
    void calculateMovingAvg(const QList<Channel *> &channels, int windowSize);
    void calculateThirdOctave(FileDescriptor *source);

    QString saveTimeSegment(double from, double to);

    QString fileName() const {return _fileName;}
    void setFileName(const QString &name) { _fileName = name;}
    //переопределен в DFD
    virtual bool fileExists() const;

    virtual int channelsCount() const = 0;

    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) = 0;

    QVariant channelHeader(int column) const;
    int columnsCount() const;

    virtual Channel *channel(int index) const = 0;

    //переопределен в UFF
    virtual void setChanged(bool changed);
    bool changed() const {return _changed;}

    bool dataChanged() const {return _dataChanged;}
    void setDataChanged(bool changed);

    int plottedCount() const;

    virtual bool isSourceFile() const;


    double xStep() const;
    void setXStep(const double xStep);
    double xBegin() const;
    int samplesCount() const;
    QString xName() const;



    virtual bool operator == (const FileDescriptor &descriptor) {
        Q_UNUSED(descriptor);
        return false;
    }

    virtual bool dataTypeEquals(FileDescriptor *other) const;

    //By default returns true, redefined in Dfd
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const;
    //By default returns true, redefined in Dfd
    virtual bool canTakeAnyChannels() const;

    bool hasCurves() const;

    virtual void addChannelWithData(DataHolder *data, const QJsonObject & description) {
        Q_UNUSED(data);
        Q_UNUSED(description);
    };

    static QString createGUID();
private:
    QString _fileName;
    bool _changed = false;
    bool _dataChanged = false;
    DataDescription _dataDescription;
};

class Channel
{
public:
    virtual ~Channel() {
        delete _data;
    }
    Channel() : _color(QColor()),
                _data(new DataHolder)
    {}
    Channel(Channel *other);
    Channel(Channel &other);

    virtual QVariant info(int column, bool edit) const = 0;
    virtual int columnsCount() const = 0;
    virtual QVariant channelHeader(int column) const = 0;

    virtual Descriptor::DataType type() const = 0;
    //octave format, 0 = no octave, 1 = 1-octave, 3 = 1/3-octave etc.
    virtual int octaveType() const = 0;

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
    virtual QString yNameOld() const;
    virtual QString zName() const = 0;
    virtual void setYName(const QString &yName) = 0;
    virtual void setXName(const QString &xName) = 0;
    virtual void setZName(const QString &zName) = 0;

    const DataDescription & dataDescription() const {return _dataDescription;}
    DataDescription & dataDescription() {return _dataDescription;}
    void setDataDescription(const DataDescription &descr) {_dataDescription = descr;}

    virtual QString legendName() const = 0;

    DataHolder *data() {return _data;}
    const DataHolder *data() const {return _data;}
    void setData(DataHolder *data) {delete _data; _data = data;}

    QByteArray wavData(qint64 pos, qint64 samples);

    virtual FileDescriptor *descriptor() = 0;

    virtual int index() const = 0;

    QColor color() const {return _color;}
    void setColor(QColor color) {_color = color;}

    QString correction() const {return _correction;}
    virtual void setCorrection(const QString &s) {_correction = s;} //виртуальна, т.к. перекрывается в uff и d94

    int plotted() const {return _plotted;}
    void setPlotted(int plotted) {_plotted = plotted;}

    bool changed() const {return _changed;}
    void setChanged(bool changed) {_changed = changed;}

    bool dataChanged() const {return _dataChanged;}
    void setDataChanged(bool changed) {_dataChanged = changed;}
private:
    QColor _color;
    int _plotted = 0; //0=not plotted, 1=plotted on left, 2=plotted on right
    bool _populated = false;
    bool _changed = false;
    bool _dataChanged = false;
    QString _correction;
protected:
    DataHolder *_data;
    DataDescription _dataDescription;
};

#endif // FILEDESCRIPTOR_H
