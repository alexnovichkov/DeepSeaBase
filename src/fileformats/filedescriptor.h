#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include <QObject>
#include "dataholder.h"
#include "boost/property_tree/ptree.hpp"
#include <algorithm>

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
                if (key == "dateTime")
                    r.insert(i.key().section('.',1), QJsonValue(i.value().toDateTime().toString("dd.MM.yyyy hh:mm:ss")));
                else
                    r.insert(i.key().section('.',1), QJsonValue::fromVariant(i.value()));
                result.insert(key, r);
            }
            else {
                if (i.key() == "dateTime" || i.key() == "fileCreationTime")
                    result.insert(i.key(), QJsonValue(i.value().toDateTime().toString("dd.MM.yyyy hh:mm:ss")));
                else
                    result.insert(i.key(), QJsonValue::fromVariant(i.value()));
            }
        }
        return result;
    }
    static DataDescription fromJson(const QJsonObject &o) {
        DataDescription result;
        for (auto i = o.constBegin(); i!=o.constEnd(); ++i) {
            QString key = i.key();
            QJsonValue val = i.value();
            //qDebug()<<key<<val;
            if (val.isArray()) {
                //qDebug()<<"Array found at"<<key;
                continue;
            }
            else if (val.isObject()) {
                QJsonObject v = val.toObject();
                for (auto j = v.constBegin(); j!=v.constEnd(); ++j) {
                    if (j->isArray()) {
                        //qDebug()<<"Array found at"<<j.key();
                        continue;
                    }
                    else if (j->isObject()) {
                        //qDebug()<<"Object found at"<<j.key();
                        continue;
                    }
                    QString key1 = key+"."+j.key();
                    QVariant v = j->toVariant();
                    //дата-время требуют специальной обработки
                    if (j.key() == "dateTime")
                        result.data.insert(key1, dateTimeFromString(v.toString()));
                    else
                        result.put(key1, v);
                }
            }
            else {
                QVariant v = val.toVariant();
                //дата-время требуют специальной обработки
                if (key == "dateTime" || key=="fileCreationTime")
                    result.data.insert(key, dateTimeFromString(v.toString()));
                else
                    result.put(key, v);
            }
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
        for (auto [key, val] : asKeyValueRange(data)) {
            if (filter.isEmpty())
                result << key+"="+val.toString();
            else if (key.startsWith(filter+"."))
                result << key.mid(filter.length()+1)+"="+val.toString();
        }
        return result;
    }
    QVariantMap filter(const QString &filter = QString()) const
    {
        if (filter.isEmpty()) return data;

        QVariantMap result;
        for (auto [key, val] : asKeyValueRange(data)) {
            if (key.startsWith(filter+".")) result.insert(key.mid(filter.length()+1), val);
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

    //чисто виртуальные
    virtual void read() = 0;
    virtual void write() = 0;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) = 0;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) = 0;
    virtual int channelsCount() const = 0;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) = 0;
    virtual Channel *channel(int index) const = 0;

    //виртуальные
    virtual bool rename(const QString &newName, const QString &newPath = QString());
    //просто вызывает updateDateTimeGUID. Переопределен в DFD
    virtual void fillPreliminary(const FileDescriptor *);
    //переопределен в DFD
    virtual bool copyTo(const QString &name);
    //переопределен в DFD
    virtual Descriptor::DataType type() const;
    //переопределен в DFD
    virtual QString typeDisplay() const;
    //переопределен в DFD
    virtual bool fileExists() const;
    //переопределен в DFD
    virtual bool isSourceFile() const;
    virtual bool operator == (const FileDescriptor &descriptor) {
        Q_UNUSED(descriptor);
        return false;
    }
    virtual bool dataTypeEquals(FileDescriptor *other) const;
    //By default returns true, redefined in Dfd
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const;
    //By default returns true, redefined in Dfd
    virtual bool canTakeAnyChannels() const;
    virtual void addChannelWithData(DataHolder *data, const DataDescription & description) {
        Q_UNUSED(data);
        Q_UNUSED(description);
    };
    //переопределен в DFD
    virtual qint64 fileSize() const;

    //все остальные
    void populate();
    void updateDateTimeGUID();

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

    /** Calculates average of channels, writes to a file */
    void calculateMean(const QList<Channel*> &channels);
    void calculateMovingAvg(const QList<Channel *> &channels, int windowSize);
    void calculateThirdOctave(FileDescriptor *source);

    QString saveTimeSegment(double from, double to);

    inline QString fileName() const {return _fileName;}
    inline void setFileName(const QString &name) { _fileName = name;}

    QVariant channelHeader(int column) const;
    int columnsCount() const;

    inline bool changed() const {return _changed;}
    //переопределен в UFF
    virtual void setChanged(bool changed);
    inline bool dataChanged() const {return _dataChanged;}
    inline void setDataChanged(bool changed) {_dataChanged = changed;}

    int plottedCount() const;
    bool hasCurves() const;

    double xStep() const;
    void setXStep(const double xStep);
    double xBegin() const;
    int samplesCount() const;
    QString xName() const;

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

    //переопределен в DFD
    virtual QVariant info(int column, bool edit) const;
    //переопределен в DFD
    virtual int columnsCount() const;
    //переопределен в DFD
    virtual QVariant channelHeader(int column) const;

    virtual Descriptor::DataType type() const = 0;
    //octave format, 0 = no octave, 1 = 1-octave, 3 = 1/3-octave etc.
    int octaveType() const;

    virtual bool populated() const {return _populated;}
    virtual void setPopulated(bool populated) {_populated = populated;}
    virtual void populate() = 0;
    virtual void clear();
    virtual void maybeClearData();

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString xName() const;
    QString yName() const;
    virtual QString yNameOld() const; //переопределен в DFD
    QString zName() const;
    void setYName(const QString &yName);
    void setXName(const QString &xName);
    void setZName(const QString &zName);

    virtual void setXStep(double xStep); //переопределен в D94

    const DataDescription & dataDescription() const {return _dataDescription;}
    DataDescription & dataDescription() {return _dataDescription;}
    void setDataDescription(const DataDescription &descr) {_dataDescription = descr;}

    QString legendName() const;

    DataHolder *data() {return _data;}
    const DataHolder *data() const {return _data;}
    void setData(DataHolder *data) {delete _data; _data = data;}

    QByteArray wavData(qint64 pos, qint64 samples);

    virtual FileDescriptor *descriptor() const = 0;

    virtual int index() const = 0;

    QColor color() const {return _color;}
    void setColor(QColor color) {_color = color;}

    inline QString correction() const {return dataDescription().get("correction").toString();}
    void setCorrection(const QString &s) {dataDescription().put("correction", s);}

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
protected:
    DataHolder *_data;
    DataDescription _dataDescription;
};

#endif // FILEDESCRIPTOR_H
