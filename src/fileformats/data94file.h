#ifndef DATA94FILE_H
#define DATA94FILE_H

#include "fileformats/filedescriptor.h"
#include <QJsonArray>

/**
 * Универсальный формат данных, созданный под нужды бюро 94.41
 *
 * Описание структуры:
 *
 * //Метка файла
 * char[8]  'data94  '
 *
 * quint64 descriptionSize - длина текстового описания в байтах
 * char[descriptionSize] description - текстовое описание данных в формате utf8
 *
 *
 * //Блок описания оси Х
 * //все блоки имеют одинаковый формат, см. ниже
 *
 * //Блок данных
 * Data {
 *     quint64 sizeInBytes - размер в байтах
 *     quint64 descriptionSize - длина текстового описания в байтах
 *     char[descriptionSize] description - текстовое описание данных
 *     quint8 dataFormat - 0 или 1, 0 = данные распределены равномерно
 *                                   1 = данные распределены неравномерно
 *     quint64 sampleCount - количество отсчетов всего, включая разбиение по третьей оси;
 *                     если данные в комплексной форме, то реальная длина записи = sampleCount*2
 *     quint64 blockCount - количество блоков.
 *
 *     //Далее идут данные для каждого блока, от 1 до blockCount
 *  ┌-------------------------------------------------------------------------------------
 *  |   quint64 samples - количество отсчетов в блоке
 *  |   double zValue - значение по оси аппликат (по другой оси), характеризующее этот блок
 *  |   //далее - краткий формат, dataFormat=0
 *  |   double step - шаг по оси
 *  |   double initial - начальное значение
 *  |   //длинный формат, dataFormat=1
 *  |   quint8 valueFormat - действительное (1) или комплексное (2)
 *  |   double[samples * valueFormat] data - вектор значений
 *  └---------------------------------------------------------------------------------
 * }
 *
 * //Далее идут записи каналов
 * quint64 descriptionSize - длина текстового описания в байтах
 * char[descriptionSize] description - текстовое описание данных
 * //Блок данных для оси Y
 *
 * Текстовое описание - формат Json
 * --------------------------------
 *
 *
 *
 */
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
    virtual bool hasAttachedFile() const override;
    virtual QString attachedFileName() const override;
    virtual void setAttachedFileName(const QString &name) override;
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
    QJsonArray description;
    QList<Data94Channel*> channels;
};

class Data94Channel : public Channel
{


    // Channel interface
public:
    virtual QVariant info(int column) const override;
    virtual int columnsCount() const override;
    virtual QVariant channelHeader(int column) const override;
    virtual Descriptor::DataType type() const override;
    virtual Descriptor::OrdinateFormat yFormat() const override;
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
};

#endif // DATA94FILE_H
