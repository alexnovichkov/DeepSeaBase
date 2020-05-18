#ifndef DATA94FILE_H
#define DATA94FILE_H

#include "fileformats/filedescriptor.h"
#include <QJsonObject>
#include <QDataStream>

#define PADDING_SIZE 1024

/**
 * Универсальный формат данных, созданный под нужды бюро 94.41
 *
 * Описание структуры:
 *
 * //Метка файла
 * |data|94  |ui32| *  |  'data94  '
 *                        quint32 descriptionSize - длина текстового описания в байтах
 * | *  | *  | *  | *  |  -> char[descriptionSize] description - текстовое описание данных в формате utf8
 * |ui32|0000|0000|0000|  -> quint32 paddingSize - размер паддинга (1).
 * |0000|0000|0000|0000|  -> byte[paddingSize] - паддинг
 * |     XBlk          |  -> xBlock
 * |     ZBlk          |  -> zBlock
 *
 *
 * //Блок описания оси Х и Z
 * quint32 uniform     0 - шкала неравномерная, 1 - шкала равномерная
 * quint32 count
 * float begin - если шкала равномерная
 * float step - если шкала равномерная
 * float[count] - если шкала неравномерная
 *
 *
 * //Далее идут записи каналов
 * quint32 channelsCount - количество каналов
 *
 * //Блок данных для оси Y
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
 *
 * (1) При создании файла паддинг по умолчанию равен
 *     PADDING_SIZE, он может уменьшаться, если мы увеличиваем размер описания.
 *     Если при сохранении файла descriptionSize > oldDescriptionSize+paddingSize, то:
 *     1. снова добавляем паддинг PADDING_SIZE
 *     2. перезаписываем весь файл с учетом новых размеров
 */

/**
 * @brief The Data94Channel class
 * Блок данных
 *     quint32 valueFormat - действительное (1) или комплексное (2)
 *
 *  ┌-------------------------------------------------------------------------------------
 *  |
 *  |   float[samples * blockCount * valueFormat] data - вектор значений
 *  |   отформатирован следующим образом:
 *  |   Block 0 - sample0_re sample0_im sample1_re sample1_im .....
 *  |   Block 1 - sample0_re sample0_im sample1_re sample1_im .....
 *  └---------------------------------------------------------------------------------
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

    bool isValid = false;
};

class Data94Channel;

class Data94File : public FileDescriptor
{
public:
    Data94File(const QString &fileName);
    // creates a copy of Data94File with copying data
    Data94File(const Data94File &d, const QString &fileName, QVector<int> indexes = QVector<int>());
    // creates a copy of FileDescriptor with copying data
    Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>());

    // FileDescriptor interface
public:
    virtual void fillPreliminary(Descriptor::DataType) override;
    virtual void fillRest() override;
    virtual void read() override;
    virtual void write() override;
    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    virtual int channelsCount() const override;
    virtual Descriptor::DataType type() const override;
    virtual QString typeDisplay() const override;
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &data) override;
    virtual QString dataDescriptorAsString() const override;
    virtual QDateTime dateTime() const override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes) override;
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels) override;
    virtual QString calculateThirdOctave() override;
    virtual void calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize) override;
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
    friend class Data94Channel;
    QJsonObject description;
    QList<Data94Channel*> channels;

    AxisBlock xAxisBlock;
    AxisBlock zAxisBlock;

    quint32 descriptionSize = 0;
    quint32 paddingSize = PADDING_SIZE;
};

class Data94Channel : public Channel
{
public:
    Data94Channel(Data94File *parent);
    Data94Channel(Data94Channel *other);
    Data94Channel(Channel *other);
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
    virtual QString legendName() const override;
    virtual FileDescriptor *descriptor() override;
    virtual int index() const override;
    virtual QString correction() const override;
    virtual void setCorrection(const QString &s) override;
private:
    friend class Data94File;
    Data94File *parent = 0;

    bool isComplex = false;
    qint64 dataPosition = -1;
    QJsonObject _description;
};

#endif // DATA94FILE_H
