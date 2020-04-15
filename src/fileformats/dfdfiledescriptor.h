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
    // 16 - 31 - преобразованные данные
    Envelope   =  16,		// огибающая по Гильберту
    PassAvrg  =   17,		// проходная - мат. ожидание
    PassDev   =   18,		// проходная - СКЗ
    PassAss   =   19,		// проходная - асимметрия
    PassExc   =   20,		// проходная - эксцесс
    GPhaPhase =   21, 	// фаза Гильберта
    TrFltP     =  22, 		// следящий фильтр - мощность
    TrFltF     =  23,		// следящий фильтр - частота
    // 32 - 63 -  корреляционный анализ
    AutoCorr   =  32,		// автокорреляция
    CrossCorr  =  33,		// взаимная корреляция
    // 64 - 127 - классическая статистика
    Histogram  =  64,		// гистограмма
    EDF          =  65,		// эмпирическая функция распределения
    Hist100    	=  66,		// гистограмма %
    HistP      	=  67,		// плотность вероятности
    // 128 - 159 - спектральный анализ
    Spectr     = 128,		// спектр мощности
    SpcDens = 129,		// плотность спектра мощности
    SpcDev   = 130,		// спектр СКЗ
    XSpectr   = 144,		// взаимный спектр
    XPhase   = 145,		// взаимная фаза
    Coherence  = 146,	// когерентность
    TransFunc  = 147,	// передаточная функция
    XSpectrRe  = 148,	// действ. часть взаимного спектра
    XSpectrIm  = 149,		// мнимая часть взаимного спектра
    TrFuncRe   = 150,	// действ. часть передаточной функции
    TrFuncIm   = 151,		// мнимая часть передаточной функции
    DiNike     = 152,		// диаграмма Найквиста для взаимных спектров
    Cepstr     = 153,		// кепстр
    DiNikeP    = 154,       // диаграмма Найквиста для передаточной функции
    GSpectr    = 155,		// спектр Гильберта
    OSpectr    = 156,		// октавный спектр
    ToSpectr   = 157,		// 1/3-октавный спектр
    NotDef     = 255		// неопределенный
};

DfdDataType dfdDataTypeFromDataType(Descriptor::DataType type);
Descriptor::DataType dataTypefromDfdDataType(DfdDataType type);

//enum PlotType {
//    PlotUnknown = 0,
//    PlotTime = 1,
//    PlotStatistics = 2,
//    PlotSpectre = 3,
//    PlotNykvist = 4,
//    PlotCorrelation = 5,
//    PlotOctave = 6
//};

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

//PlotType plotTypeByDataType(DfdDataType dataType);

QString dataTypeDescription(int type);

class DfdFileDescriptor;
class DfdSettings;

class DfdChannel : public Channel
{
public:
    /** [Channel#] */
    DfdChannel(DfdFileDescriptor *parent, int channelIndex);
    DfdChannel(DfdChannel &other, DfdFileDescriptor *parent=0);
    DfdChannel(Channel &other, DfdFileDescriptor *parent=0);

    virtual ~DfdChannel();

    virtual Descriptor::DataType type() const;

    virtual void read(DfdSettings &dfd, int numChans);
    virtual void write(QTextStream &dfd, int index = -1);

    virtual QVariant info(int column, bool edit) const;
    virtual int columnsCount() const;
    virtual QVariant channelHeader(int column) const;

    virtual QString correction() const {return m_correction;}
    virtual void setCorrection(const QString &s) {m_correction = s;}

    virtual QString legendName() const;
    //дополнительная обработка, напр.
    //применение смещения и усиления,
    //для получения реального значения
    virtual double postprocess(double v) {return v;}
    virtual void postprocess(QVector<double> & v) {Q_UNUSED(v);}

    void setValue(double val, QDataStream &writeStream);

    virtual QString xName() const;
    virtual QString yName() const;
    virtual void setYName(const QString &yName);

    virtual QString description() const {return ChanDscr;}
    virtual void setDescription(const QString &description) {ChanDscr = description;}

    virtual QString name() const {return ChanName;}
    virtual void setName(const QString &name);

    virtual void populate();
    void populateFloat();
    QVector<float> floatValues;

    /**
     * @brief preprocess - подготавливает значение к записи с помощью setValue
     * @param v - значение
     * @return подготовленное значение
     */
    virtual double preprocess(double v) {return v;}

    QString ChanAddress; //
    QString ChanName; //
    uint IndType; //характеристика отсчета
    int ChanBlockSize; //размер блока в отсчетах
    QString YName;
    QString YNameOld;
//    QString XName;
    QString InputType;
    QString ChanDscr;

    quint64 blockSizeInBytes() const; //размер блока в байтах

    DfdFileDescriptor *parent;
    virtual FileDescriptor *descriptor();
    int channelIndex; // нумерация с 0



    DfdDataType dataType;
    QList<int> dataPositions;

    QString m_correction;
private:
    int dataFormat() const;

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
    virtual void read(DfdSettings &dfd, int numChans) override;
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
    void read(DfdSettings &dfd);
    void write(QTextStream &dfd);
    QString File; // название файла источника
    QString DFDGUID; // GUID файла источника
    QDate Date; // дата создания файла источника
    QTime Time; // время создания файла источника
    // новый формат
    QString sFile; // K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
    QList<int> ProcChansList;
};

class Process
{
public:
    /** [Process] */
    Process();
    void read(DfdSettings &dfd);
    void write(QTextStream &dfd);
    QString value(const QString &key);
    DescriptionList data;
};

class DataDescription
{
public:
    DataDescription(DfdFileDescriptor *parent);
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

    // creates a copy of DfdDataDescriptor with copying data
    DfdFileDescriptor(const DfdFileDescriptor &d);
    // creates a copy of DataDescriptor with copying data
    DfdFileDescriptor(const FileDescriptor &other);
    virtual ~DfdFileDescriptor();
    virtual void read();
    virtual void write();
    virtual void writeRawFile();
    void updateDateTimeGUID();
    virtual void fillPreliminary(Descriptor::DataType);
    virtual void fillRest();
    static DfdFileDescriptor *newFile(const QString &fileName, DfdDataType type);
    virtual bool copyTo(const QString &name) override;

    QDateTime dateTime() const;
    virtual Descriptor::DataType type() const;
    virtual QString typeDisplay() const;
    virtual DescriptionList dataDescriptor() const;
    virtual void setDataDescriptor(const DescriptionList &data);

    virtual double xStep() const {return XStep;}

    virtual void setXStep(const double xStep);

    virtual bool setLegend(const QString &legend);
    virtual QString legend() const;

//    QString fileName() {return FileName;}

    void setFileName(const QString &name);
    virtual bool fileExists() const override;

    void setDataChanged(bool changed);

    virtual int channelsCount() const {return channels.size();}

    void deleteChannels(const QVector<int> &channelsToDelete);
    void copyChannelsFrom(FileDescriptor *file, const QVector<int> &indexes);
    virtual void calculateMean(const QList<QPair<FileDescriptor *, int> > &channels);
    virtual void calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels,
                                    int windowSize);
    virtual QString calculateThirdOctave();
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes);

    virtual QVariant channelHeader(int column) const;
    virtual int columnsCount() const;
    Channel *channel(int index) const;
    virtual DfdChannel* dfdChannel(int index) {return channels[index];}

    bool isSourceFile() const;

    bool dataTypeEquals(FileDescriptor *other) const;

    QString fileFilters() const;

    virtual QString xName() const {return XName;}

    virtual bool setDateTime(QDateTime dt) override;

    bool operator == (const DfdFileDescriptor &dfd)
    {
        return (this->DFDGUID == dfd.DFDGUID);
    }
    QString dataDescriptorAsString() const;
    static QString createGUID();

    DfdChannel *newChannel(int index);

    //[DataFileDescriptor]
    QString DFDGUID; //{7FD333E3-9A20-2A3E-A9443EC17B134848}
    DfdDataType DataType; // см. выше
    QDate Date;
    QTime Time;


    int BlockSize;
    int NumInd;
    QString XName;

    double XBegin;
    double realXBegin;
    double XStep;
    QString DescriptionFormat;
    QString CreatedBy;

    Source *source;
    Process *process;
    DataDescription *dataDescription;

    QString rawFileName; // путь к RAW файлу
    QList<DfdChannel *> channels;

private:
    static DfdFileDescriptor *newThirdOctaveFile(const QString &fileName);
    bool rewriteRawFile(const QVector<QPair<int,int> > &indexesVector, DfdFileDescriptor *fileToRewrite = 0);
    void copyChannelsFrom_plain(FileDescriptor *file, const QVector<int> &indexes);

    friend class DataDescription;
    friend class DfdChannel;
    QVector<double> xValues;


    QString _legend; // editable description

    // FileDescriptor interface
public:
    virtual QString saveTimeSegment(double from, double to);
    virtual int samplesCount() const;
    virtual void setSamplesCount(int count);
};

#endif // DFDFILEDESCRIPTOR_H
