#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <QtCore>
//#include <QColor>
#include <QObject>
#include "dataholder.h"
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
    MultipleCoherence = 26,
    OrderFunction = 27,
    Transit = 28
};

QString functionTypeDescription(int type);

}

struct DataDescription
{
    QVariantMap data;

    void put(const QString &key, const QVariant &value);
    QVariant get(const QString &key) const;
    QJsonObject toJson() const;
    static DataDescription fromJson(const QJsonObject &o);
    QDateTime dateTime(const QString &key = "dateTime") const;

    QStringList twoStringDescription() const;
    QStringList toStringList(const QString &filter = QString(), bool includeKeys = true) const;
    QVariantMap filter(const QString &filter = QString()) const;
};

QDataStream &operator>>(QDataStream& stream, DataDescription& header);
QDataStream &operator<<(QDataStream& stream, const DataDescription& header);


class Channel;
class DataHolder;

typedef QPair<QString, QString> DescriptionEntry;
typedef QList<DescriptionEntry> DescriptionList;

//QString descriptionEntryToString(const DescriptionEntry &entry);

QString stringify(const QVector<int> &vec);

//QString valuesUnit(const QString &old, int unitType);

class FileDescriptor
{
public:
    FileDescriptor(const QString &fileName);
    virtual ~FileDescriptor();

    //чисто виртуальные
    virtual void read() = 0;
    virtual int channelsCount() const = 0;
    virtual Channel *channel(int index) const = 0;
    virtual QString fileType() const = 0;

    //переопределите эти методы, если файл поддерживает изменение
    virtual void write() {};
    virtual void deleteChannels(const QVector<int> &) {};
    virtual void copyChannelsFrom(const QVector<Channel*> &) {};
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) {};

    //виртуальные
    virtual QString icon() const {return "";}

    virtual bool rename(const QString &newName, const QString &newPath);
    virtual bool rename(const QString &newNamePath);
    virtual bool remove();
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
    //переопределен в DFD и в ANP
    virtual qint64 fileSize() const;

    //все остальные
    void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes);
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

    inline QString fileName() const {return _fileName;}
    inline void setFileName(const QString &name) { _fileName = name;}

    QVariant channelHeader(int column) const;
    int columnsCount() const;

    inline bool changed() const {return _changed;}
    //переопределен в UFF
    virtual void setChanged(bool changed);
    inline bool dataChanged() const {return _dataChanged;}
    inline void setDataChanged(bool changed) {_dataChanged = changed;}

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
    Channel() : _data(new DataHolder)
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

    bool populated() const {return _populated;}
    void setPopulated(bool populated) {_populated = populated;}
    virtual void populate() = 0;
    void clear();
    void maybeClearData();

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);

    QString xName() const;
    QString yName() const;
    /**
     * @brief yNameOld нужен из-за того, что в файлах dfd, в которых записаны уровни, уже
     * пересчитанные к дБ, единица измерения пишется дБ, а прежняя единица измерения (м/с^2)
     * записывается в YNameOld.
     * В файлах других форматов различие исходных данных и пересчитанных сохраняется другими способами
     * @return
     */
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

    QByteArray wavData(qint64 pos, qint64 samples, int format);

    virtual FileDescriptor *descriptor() const = 0;

    virtual int index() const = 0;

    inline QString correction() const {return dataDescription().get("correction").toString();}
    void setCorrection(const QString &s) {dataDescription().put("correction", s);}

    inline bool changed() const {return _changed;}
    void setChanged(bool changed) {_changed = changed;}

    bool dataChanged() const {return _dataChanged;}
    void setDataChanged(bool changed) {_dataChanged = changed;}
private:
    bool _populated = false;
    bool _changed = false;
    bool _dataChanged = false;
protected:
    DataHolder *_data;
    DataDescription _dataDescription;
};

#endif // FILEDESCRIPTOR_H
