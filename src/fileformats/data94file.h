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
 *   "dateTime" : "dd.MM.yyyy hh:mm", //создание базы данных
 *   "fileCreationTime": "dd.MM.yyyy hh:mm", //создание этого файла
 *   "createdBy": "",//программа, которая записала файл
 *   "legend" : "",
 *   "description": {
 *       "свойства конкретного файла" //аналогично DFD DataDescription
 *   },
 *   "guid": "",
 *   "source": {
 *     "file": "",
 *     "guid": "",
 *     "dateTime": "dd.MM.yyyy hh:mm",
 *     "channels": "1,2,3,4,5"
 *   },
 *   "channels" : [
 *       {
 *           "name" : "",
 *           "description": "",
 *           "correction": "",
 *           "dateTime": "dd.MM.yyyy hh:mm", //скорее для uff, чем для остальных форматов
 *           "yname" : "m/s^2",
 *           "ynameold": "V", //при пересчете единиц
 *           "xname" : "Hz",
 *           "zname" : "s",
 *
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
 *               "description": "H1",
 *               "logref": 0.000314,
 *               "logscale": "linear" / "quadratic" / "dimensionless",
 *               "responseName": "lop1:1",
 *               "responseDirection": "+z",
 *               "responseNode": "",
 *               "referenceName": "lop1:1",
 *               "referenceNode": "",
 *               "referenceDescription": "более полное описание из dfd",
 *               "referenceDirection": "",
 *               "format": "real", "imaginary", "complex", "amplitude", "phase", "amplitudeDb"
 *               "precision": int8 / uint8 / int16 / uint16 / int32 / uint32 / int64 / uint64 / float / double
 *               "octaveFormat" : 0 (default) / 1 / 3 / 2 / 6 / 12 / 24,
 *               //далее идут все параметры обработки
 *               "weighting": no / A / B / C / D
 *
 *               "averaging": "linear", "no", "exponential", "peak hold", "energetic"
 *               "averagingCount": 38,
 *               "window": "Hanning", "Hamming", "square", "triangular", "Gauss", "Natoll",
 *                         "force", "exponential", "Tukey", "flattop", "Bartlett", "Welch symmetric",
 *                         "Welch periodic", "Hanning broad", "impact", "Kaiser-Bessel",
 *               "windowParameter": 0.00,
 *               "blockSize": 2048,
 *               "channels": "1,2,3,4,5",
 *               ""
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
    // creates a copy of FileDescriptor with copying data
    Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>());
    ~Data94File();

    // FileDescriptor interface
public:
    virtual QString icon() const override {return ":/icons/d94.svg";}
    virtual void read() override;
    virtual void write() override;
    virtual int channelsCount() const override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes) override;
    void addChannelWithData(DataHolder *data, const DataDescription &description) override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;
    virtual Channel *channel(int index) const override;
    static QStringList fileFilters();
    static QStringList suffixes();
private:
    void updatePositions();
    friend class Data94Channel;
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
    void write(QDataStream &r, QDataStream *in, DataHolder *data);
    void setXStep(double xStep) override;

    // Channel interface
public:
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;

    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;

    bool isComplex = false; //по умолчанию real
    quint32 sampleWidth = 4; //по умолчанию float
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
