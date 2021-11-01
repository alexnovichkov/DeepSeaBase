#ifndef DFDFILEDESCRIPTOR_H
#define DFDFILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include "fileformats/filedescriptor.h"
#include <qwt_series_data.h>

enum DfdDataType {
    // 0 - 15 - исходные данные
    SourceData =   0,		// исходные данные
    CuttedData =   1,		// вырезка из исходных данных
    FilterData =   2,		// фильтрованные данные
    CuttedData1,
    CuttedData2,
    CuttedData3,
    CuttedData4,
    CuttedData5,
    CuttedData6,
    CuttedData7,
    CuttedData8,
    CuttedData9,
    CuttedData10,
    CuttedData11,
    CuttedData12,
    CuttedData13,
    // 16 - 31 - преобразованные данные
    Envelope   =  16,		// огибающая по Гильберту
    PassAvrg   =  17,		// проходная - мат. ожидание
    PassDev    =  18,		// проходная - СКЗ
    PassAss    =  19,		// проходная - асимметрия
    PassExc    =  20,		// проходная - эксцесс
    GPhaPhase  =  21, 	    // фаза Гильберта
    TrFltP     =  22, 		// следящий фильтр - мощность
    TrFltF     =  23,		// следящий фильтр - частота
    // 32 - 63 -  корреляционный анализ
    AutoCorr   =  32,		// автокорреляция
    CrossCorr  =  33,		// взаимная корреляция
    // 64 - 127 - классическая статистика
    Histogram  =  64,		// гистограмма
    EDF        =  65,		// эмпирическая функция распределения
    Hist100    =  66,		// гистограмма %
    HistP      =  67,		// плотность вероятности
    // 128 - 159 - спектральный анализ
    Spectr     = 128,		// спектр мощности
    SpcDens    = 129,		// плотность спектра мощности
    SpcDev     = 130,		// спектр СКЗ
    XSpectr    = 144,		// взаимный спектр
    XPhase     = 145,		// взаимная фаза
    Coherence  = 146,	    // когерентность
    TransFunc  = 147,	    // передаточная функция
    XSpectrRe  = 148,	    // действ. часть взаимного спектра
    XSpectrIm  = 149,		// мнимая часть взаимного спектра
    TrFuncRe   = 150,	    // действ. часть передаточной функции
    TrFuncIm   = 151,		// мнимая часть передаточной функции
    DiNike     = 152,		// диаграмма Найквиста для взаимных спектров
    Cepstr     = 153,		// кепстр
    DiNikeP    = 154,       // диаграмма Найквиста для передаточной функции
    GSpectr    = 155,		// спектр Гильберта
    OSpectr    = 156,		// октавный спектр
    ToSpectr   = 157,		// 1/3-октавный спектр
    TwoOSpectr = 158,       // 1/2-октавный спектр
    SixOSpectr = 159,       // 1/6-октавный спектр
    TwlOSpectr = 160,       // 1/12-октавный спектр
    TFOSpectr  = 161,       // 1/24-октавный спектр
    NotDef     = 255		// неопределенный
};

DataHolder::YValuesFormat dataFormat(DfdDataType dataType, const QString &typeScale);

DfdDataType dfdDataTypeFromDataType(const Channel &ch);
Descriptor::DataType dataTypefromDfdDataType(DfdDataType type);

struct Method
{
    QString methodDll;
    QString methodDescription;
    DfdDataType dataType;
    int panelType;
};

const Method methods[26] = {
    {"TimeWv.dll", "Осциллограф", NotDef, 0},
    {"spectr.dll", "Спектроанализатор", Spectr, 0},
    {"zoom.dll", "Лупа спектральная", NotDef, 0},
    {"InEx.dll", "Проходная функция", NotDef, 0},
    {"xspect.dll", "Модуль взаимного сп.", NotDef, 0},
    {"xcospect.dll", "Действ.взаимн.спектра", NotDef, 0},
    {"xqspect.dll", "Мнимая взаимн.спектра", NotDef, 0},
    {"nike.dll", "Д.Найквиста (взаимного)", NotDef, 1},
    {"xphase.dll", "Фазовый спектр", NotDef, 0},
    {"xresponcH1.dll", "Передаточная ф-я H1", NotDef, 0},
    {"xresponcH2.dll", "Передаточная ф-я H2", NotDef, 0},
    {"xrespreal.dll", "Действ.передаточной Н1", NotDef, 0},
    {"xrespimage.dll", "Мнимая передаточной Н1", NotDef, 0},
    {"xrespcon.dll", "Характерист.конструкций", NotDef, 0},
    {"nikePer.dll", "Д.Найквиста(передаточной)", NotDef, 1},
    {"xcogerent.dll", "Когерентость", NotDef, 0},
    {"gistogr.dll", "Гистограмма", NotDef, 0},
    {"cepstr.dll", "Кепстр", NotDef, 0},
    {"octSpect8.dll", "Октавный спектр", OSpectr, 0},
    {"corel.dll", "АКорреляция", NotDef, 0},
    {"corelF.dll", "АКорреляция (Фурье)", NotDef, 0},
    {"xCorel.dll", "ХКорреляция", NotDef, 0},
    {"xCorelF.dll", "ХКорреляция (Фурье)", NotDef, 0},
    {"ogib.dll", "Огибающая по Гильберту", NotDef, 0},
    {"xresponce.dll", "Модуль передаточной", NotDef, 0},
    {"traceFlt.dll", "Следящий фильтр", NotDef, 0}
};


class DfdFileDescriptor;
class DfdSettings;

class DfdChannel : public Channel
{
public:
    /** [Channel#] */
    DfdChannel(DfdFileDescriptor *parent, int channelIndex);
    DfdChannel(DfdChannel &other, DfdFileDescriptor *parent);
    DfdChannel(Channel &other, DfdFileDescriptor *parent);

    virtual ~DfdChannel();

    virtual Descriptor::DataType type() const override;

    virtual void read(DfdSettings &dfd, int numChans, double xBegin, double xStep);
    virtual void write(QTextStream &dfd, int index = -1);

    virtual QVariant info(int column, bool edit) const override;
    virtual int columnsCount() const override;
    virtual QVariant channelHeader(int column) const override;

    //дополнительная обработка, напр.
    //применение смещения и усиления,
    //для получения реального значения
    virtual double postprocess(double v) {return v;}
    virtual void postprocess(QVector<double> & v) {Q_UNUSED(v);}

    void setValue(double val, QDataStream &writeStream);

    virtual QString yNameOld() const override;

    virtual void populate() override;

    /**
     * @brief preprocess - подготавливает значение к записи с помощью setValue
     * @param v - значение
     * @return подготовленное значение
     */
    virtual double preprocess(double v) {return v;}

    void appendDataTo(const QString &rawFileName);
    void write(QDataStream &s, DataHolder *d);

    //QString ChanAddress; //
    //QString ChanName; //
    uint IndType; //характеристика отсчета
    int ChanBlockSize; //размер блока в отсчетах
    //QString YName;
    //QString YNameOld;
//    QString XName;
//    QString InputType;
    //QString ChanDscr;

    quint64 blockSizeInBytes() const; //размер блока в байтах

    DfdFileDescriptor *parent;
    virtual FileDescriptor *descriptor() const override;
    int channelIndex; // нумерация с 0



    DfdDataType dataType;
    QList<qint64> dataPositions;

    // Channel interface
public:
    virtual int index() const override;
};

class RawChannel : public DfdChannel
{
public:
    /** [Channel#] */
    RawChannel(DfdFileDescriptor *parent, int channelIndex)
        : DfdChannel(parent, channelIndex),
          ADC0(0.0),
          ADCStep(0.0),
          AmplShift(0.0),
          AmplLevel(0.0),
          Sens0Shift(0.0),
          SensSensitivity(0.0),
          BandWidth(0.0)
    {}
    RawChannel(RawChannel &other, DfdFileDescriptor *parent=0);

    virtual ~RawChannel() {}
    virtual void read(DfdSettings &dfd, int numChans, double xBegin, double xStep) override;
    virtual void write(QTextStream &dfd, int index = -1) override;
    virtual int columnsCount() const override;
    virtual double postprocess(double v) override;
    virtual void postprocess(QVector<double> &v) override;
    virtual double preprocess(double v) override;

    QString SensName;
    double ADC0;
    double ADCStep;
    double AmplShift;
    double AmplLevel;
    double Sens0Shift;
    double SensSensitivity;
    float BandWidth;
    double coef1, coef2, coef3, coef4;

    // Channel interface
public:
    virtual QVariant channelHeader(int column) const override;
    virtual QVariant info(int column, bool edit) const override;
};

class Source
{
public:
    /** [Source] */
    /** [Sources] */
    Source(FileDescriptor *parent) : parent(parent) {}
    void read(DfdSettings &dfd);
    void write(QTextStream &dfd);
private:
    FileDescriptor *parent;
};

class Process
{
public:
    /** [Process] */
    Process(FileDescriptor *parent) : parent(parent) {}
    Process(DataDescription *data) : data(data) {}
    void read(DfdSettings &dfd, DfdDataType dataType);
    void write(QTextStream &dfd);
//    QString value(const QString &key);
private:
    FileDescriptor *parent = 0;
    DataDescription *data = 0;
};

class Description
{
public:
    Description(DfdFileDescriptor *parent);
    void read(DfdSettings &dfd);
    void write(QTextStream &dfd);
    QString toString() const;
    DescriptionList data;
private:
    DfdFileDescriptor *parent;
};

class DfdFileDescriptor : public FileDescriptor
{
public:
    DfdFileDescriptor(const QString &fileName);
    DfdFileDescriptor(const DfdFileDescriptor &d, const QString &fileName, QVector<int> indexes = QVector<int>());
    DfdFileDescriptor(const FileDescriptor &other, const QString &fileName, QVector<int> indexes = QVector<int>());
    DfdFileDescriptor(const QVector<Channel *> &source, const QString &fileName);
    virtual ~DfdFileDescriptor();

    virtual QString icon() const override {return ":/icons/dfd.svg";}

    virtual void read() override;
    virtual void write() override;
    virtual void fillPreliminary(const FileDescriptor *file) override;
    static DfdFileDescriptor *newFile(const QString &fileName, DfdDataType type);
    virtual bool copyTo(const QString &name) override;

    virtual Descriptor::DataType type() const override;
    virtual QString typeDisplay() const override;
    virtual qint64 fileSize() const override;

    virtual bool fileExists() const override;

    virtual int channelsCount() const override {return channels.size();}

    void deleteChannels(const QVector<int> &channelsToDelete) override;
    void copyChannelsFrom(const QVector<Channel*> &source) override;
    void addChannelWithData(DataHolder *data, const DataDescription &description) override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;

    Channel *channel(int index) const override;
    virtual DfdChannel* dfdChannel(int index) {return channels[index];}

    bool isSourceFile() const override;

    bool dataTypeEquals(FileDescriptor *other) const override;
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const override;
    bool canTakeAnyChannels() const override;

    static QStringList fileFilters();
    static QStringList suffixes();

    bool operator == (const DfdFileDescriptor &dfd)
    {
        return (this->dataDescription().get("guid") == dfd.dataDescription().get("guid"));
    }

    DfdChannel *newChannel(int index);

    DfdDataType DataType; // см. выше

    int BlockSize;
    QString rawFileName; // путь к RAW файлу
    QList<DfdChannel *> channels;
private:
    void init(const QVector<Channel *> &source);
    bool rewriteRawFile(const QVector<QPair<int,int> > &indexesVector);
    void copyChannelsFrom_plain(const QVector<Channel *> &source);
    bool appendRawFile(const QVector<int> &channelsToKeep, DfdFileDescriptor *sourceFile);
    void writeDfd(QTextStream &dfdStream);

    friend class Description;
    friend class DfdChannel;
    bool xChannel = false;

    // FileDescriptor interface
public:
    virtual bool rename(const QString &newName, const QString &newPath) override;
};

#endif // DFDFILEDESCRIPTOR_H
