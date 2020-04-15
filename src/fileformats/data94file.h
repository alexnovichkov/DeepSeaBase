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
 * |data|94  |ui32| *  |  'data94 -> 'quint32 descriptionSize - длина текстового описания в байтах
 * | *  | *  | *  | *  |  -> char[descriptionSize] description - текстовое описание данных в формате utf8
 * |ui32|0000|0000|0000|  -> quint32 paddingSize - размер паддинга (1).
 * |0000|0000|0000|0000|  -> byte[paddingSize] - паддинг
 * |ui32|XBlk          |  -> quint32 xBlockPresent: 0 = absent, 1 = present
 * |                   |  -> xBlock если xBlockPresent==1
 *
 *
 * //Блок описания оси Х
 * //все блоки имеют одинаковый формат
 * quint32 labelSize
 * char[labelSize]     метка оси Х в кодировке utf8
 * quint32 uniform     0 - шкала неравномерная, 1 - шкала равномерная
 * quint64 samplesCount
 * double xBegin - если шкала равномерная
 * double xStep - если шкала равномерная
 * double[samplesCount] - если шкала неравномерная
 *
 *
 * //Далее идут записи каналов
 * quint32 channelsCount - количество каналов
 *
 * quint64 descriptionSize - длина текстового описания в байтах
 * char[descriptionSize] description - текстовое описание данных
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
 *   }
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
 * @brief The Data94Block class
 * Блок данных
 *     quint64 sizeInBytes - размер в байтах
 *     quint32 dataFormat - 0 или 1, 0 = данные распределены равномерно
 *                                   1 = данные распределены неравномерно
 *     quint64 sampleCount - количество отсчетов всего, включая разбиение по третьей оси;
 *                     если данные в комплексной форме, то реальная длина записи = sampleCount*2
 *     quint32 valueFormat - действительное (1) или комплексное (2)
 *     quint64 blockCount - количество блоков.
 *             blockCount == 1 - обычный файл
 *             blockCount > 1 - файл время-частотных характеристик, каждый блок соответствует
 *                              одному отсчету по времени, в блоке записываются АЧХ
 *
 *     //Далее идут данные для каждого блока, от 1 до blockCount
 *  ┌-------------------------------------------------------------------------------------
 *  |   quint64 samples - количество отсчетов в блоке
 *  |   double zValue - значение по оси аппликат (по другой оси), характеризующее этот блок
 *  |   //далее - краткий формат, dataFormat=0
 *  |   double step - шаг по оси
 *  |   double initial - начальное значение
 *  |   //длинный формат, dataFormat=1
 *  |
 *  |   double[samples * valueFormat] data - вектор значений
 *  └---------------------------------------------------------------------------------
 */

class XAxisBlock
{
public:
    void read(QDataStream &r);
    void write(QDataStream &r);
//    quint64 position = 0;
//    quint64 dataPosition = 0;
    // quint32 labelSize
    // char[labelSize]     метка оси Х в кодировке utf8
    QString label;
    quint32 uniform = 1;//     0 - шкала неравномерная, 1 - шкала равномерная
    quint64 samplesCount = 0;
    double xBegin = 0.0;// - если шкала равномерная
    double xStep = 0.0;// - если шкала равномерная
    QVector<double> values;// - если шкала неравномерная

    bool isValid = false;
};

class Data94Block
{
public:
    void read(QDataStream &r);
    void write(QDataStream &r);
    QVector<double> getValues(quint64 block);

    quint64 position;
    quint64 dataPosition;
    bool complex;
    quint64 sampleCount;
    quint64 blockCount;
    //если данные рапределены равномерно
    QVector<int> count;
    QVector<double> startValue;
    QVector<double> step;
private:
    //если данные рапределены неравномерно
    QVector<double> values; //читаются только через getValues, так как заполняются
};

class Data94Channel;

class Data94File : public FileDescriptor
{
public:
    Data94File(const QString &fileName);
    // creates a copy of Data94File with copying data
    Data94File(const Data94File &d);
    // creates a copy of FileDescriptor with copying data
    Data94File(const FileDescriptor &other);

    // FileDescriptor interface
public:
    virtual void fillPreliminary(Descriptor::DataType) override;
    virtual void fillRest() override;
    virtual void read() override;
    virtual void write() override;
    virtual void writeRawFile() override;
    virtual void updateDateTimeGUID() override;
    virtual Descriptor::DataType type() const override;
    virtual QString typeDisplay() const override;
    virtual DescriptionList dataDescriptor() const override;
    virtual void setDataDescriptor(const DescriptionList &data) override;
    virtual QString dataDescriptorAsString() const override;
    virtual QDateTime dateTime() const override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *, const QVector<int> &) override;
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
    virtual double xStep() const override;
    virtual void setXStep(const double xStep) override;
    virtual int samplesCount() const override;
    virtual void setSamplesCount(int count) override;
    virtual QString xName() const override;
    virtual bool setDateTime(QDateTime dt) override;
    virtual bool dataTypeEquals(FileDescriptor *other) const override;
    virtual QString fileFilters() const override;
private:
    QJsonObject description;
    QList<Data94Channel*> channels;

    XAxisBlock xAxisBlock;
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
    virtual bool populated() const override;
    virtual void setPopulated(bool populated) override;
    virtual void populate() override;
    virtual QString name() const override;
    virtual void setName(const QString &name) override;
    virtual QString description() const override;
    virtual void setDescription(const QString &description) override;
    virtual QString xName() const override;
    virtual QString yName() const override;
    virtual void setYName(const QString &yName) override;
    virtual QString legendName() const override;
    virtual FileDescriptor *descriptor() override;
    virtual int index() const override;
    virtual QString correction() const override;
    virtual void setCorrection(const QString &s) override;
private:
    friend class Data94File;
    Data94File *parent;

};

#endif // DATA94FILE_H
