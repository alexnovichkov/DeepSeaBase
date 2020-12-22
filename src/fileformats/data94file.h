#ifndef DATA94FILE_H
#define DATA94FILE_H

#include "fileformats/filedescriptor.h"
#include <QJsonObject>
#include <QDataStream>

/**
 *
 * Текстовое описание - формат Json
 * --------------------------------
 * Описание файла:
 * {
 *   "dateTime" : "dd.MM.yyyy hh:mm",
 *   "sourceFile" : "",
 *   "legend" : "",
 *   "dataDescription": {
 *       "свойства конкретного файла" //аналогично DFD DataDescription
 *   },
 *   "channels" : [
 *       {
 *           "name" : "",
 *           "description": "",
 *           "correction": "",
 *           "yname" : "m/s^2",
 *           "xname" : "Hz",
 *           "zname" : "s",
 *           "responseName": "lop1:1",
 *           "responseDirection": "+z",
 *           "referenceName": "lop1:1",
 *           "referenceDirection": "",
 *           "samples": 3200, //количество отсчетов на один блок
 *           "blocks": 1, //количество блоков, соответствующих отдельным значениям по Z
 *                        //не читаем это значение, так как оно дублируется в zAxisBlock
 *           "sensorID" : "", //ChanAddress
 *           "sensorName": "", //ChanName
 *           "samplerate": 8192,
 *           "bandwidth": 3200, //обычно samplerate/2.56, но может и отличаться при полосной фильтрации
 *           "function": {
 *               "name": "FRF", //или "time" - для тонкой настройки типа функции
 *               "type": 5, //тип функции согласно UFF - обобщенный тип
 *               "logref": 0.000314,
 *               "logscale": "linear" / "quadratic" / "dimensionless",
 *               "format": "real", "imaginary", "complex", "amplitude", "phase", "amplitudeDb"
 *               "octaveFormat" : 0 (default) / 1 / 3 / 2 / 6 / 12 / 24,
 *               //далее идут все параметры обработки
 *           }
 *       },
 *       {
 *           "name" : "",
 *       }
 *   ]
 * }
 * Описание канала:
 */


class AxisBlock
{
public:
    void read(QDataStream &r);
    void write(QDataStream &r);
    //размер в байтах
    quint32 size() const;

    quint32 uniform = 1;//     0 - шкала неравномерная, 1 - шкала равномерная
    quint32 count = 0;
    float begin = 0.0;// - если шкала равномерная
    float step = 0.0;// - если шкала равномерная
    QVector<double> values;// - если шкала неравномерная
};


class Data94Channel;

class Data94File : public FileDescriptor
{
public:
    Data94File(const QString &fileName);
    // creates a copy of Data94File with copying data
    Data94File(const Data94File &other, const QString &fileName, QVector<int> indexes = QVector<int>());
    // creates a copy of FileDescriptor with copying data
    Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>());
    ~Data94File();

    // FileDescriptor interface
public:
    virtual void fillPreliminary(FileDescriptor *file) override;
    virtual void read() override;
    virtual void write() override;
    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    virtual int channelsCount() const override;
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &data) override;
    virtual QString dataDescriptorAsString() const override;
    virtual QDateTime dateTime() const override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes) override;
    void addChannelWithData(DataHolder *data, const QList<Channel *> &source) override;
    virtual QString calculateThirdOctave() override;
    virtual void calculateMovingAvg(const QList<Channel *> &list, int windowSize) override;
    virtual QString saveTimeSegment(double from, double to) override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;
    virtual QVariant channelHeader(int column) const override;
    virtual int columnsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString legend() const override;
    virtual bool setLegend(const QString &legend) override;
    virtual double xBegin() const override;
    virtual double xStep() const override;
    virtual void setXStep(const double xStep) override;
    virtual int samplesCount() const override;
    virtual void setSamplesCount(int count) override;
    virtual QString xName() const override;
    virtual bool setDateTime(QDateTime dt) override;
    virtual bool dataTypeEquals(FileDescriptor *other) const override;
    static QStringList fileFilters();
    static QStringList suffixes();
private:
    void updatePositions();
    friend class Data94Channel;
    QJsonObject description;
    QList<Data94Channel*> channels;
    quint32 descriptionSize = 0;
};

class Data94Channel : public Channel
{
public:
    Data94Channel(Data94File *parent);
    Data94Channel(Data94Channel *other, Data94File *parent);
    Data94Channel(Channel *other, Data94File *parent);
    void read(QDataStream &r);
    void setXStep(double xStep);

    // Channel interface
public:
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
    virtual FileDescriptor *descriptor() override;
    virtual int index() const override;
    virtual QString correction() const override;
    virtual void setCorrection(const QString &s) override;

    bool isComplex = false; //по умолчанию real
    quint32 sampleWidth = 4; //по умолчанию float
    QJsonObject _description;
    AxisBlock xAxisBlock;
    AxisBlock zAxisBlock;
private:
    friend class Data94File;
    Data94File *parent = 0;

    qint64 dataPosition = -1;
    qint64 position = -1;
    quint32 descriptionSize = 0;
    quint32 size = 0;
};

#endif // DATA94FILE_H
