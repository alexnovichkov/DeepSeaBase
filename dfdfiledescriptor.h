#ifndef DFDFILEDESCRIPTOR_H
#define DFDFILEDESCRIPTOR_H

#include <QtCore>
#include <QColor>
#include "filedescriptor.h"
#include <qwt_series_data.h>

#define DebugPrint(s) qDebug()<<#s<<s;

enum DfdDataType {
    // 0 - 15 - исходные данные
    SourceData =   0,		// исходные данные
    CuttedData =   1,		// вырезка из исходных данных
    FilterData =   2,		// фильтрованные данные
    // 16 - 31 - преобразованные данные
    Envelope   =  16,		// огибающая
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

enum PlotType {
    PlotUnknown = 0,
    PlotTime = 1,
    PlotStatistics = 2,
    PlotSpectre = 3,
    PlotNykvist = 4,
    PlotCorrelation = 5,
    PlotOctave = 6
};

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
    {"octSpect8.dll", "Октавный спектр", NotDef, 0},
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
//QString methodDescription(int methodType);
//QString dllForMethod(int methodType);
//int panelTypeForMethod(int methodType);
//DfdDataType dataTypeForMethod(int methodType);


class DfdFileDescriptor;

class DfdChannel : public Channel
{
public:
    /** [Channel#] */
    DfdChannel(DfdFileDescriptor *parent, int channelIndex);
    DfdChannel(const DfdChannel &other);
    DfdChannel(Channel &other);

    virtual ~DfdChannel();

    virtual Descriptor::DataType type() const;
    virtual Descriptor::OrdinateFormat yFormat() const;

    virtual void read(QSettings &dfd, int chanIndex);
    virtual void write(QTextStream &dfd, int chanIndex);
    virtual QStringList getInfoHeaders();
    virtual QStringList getInfoData();



    virtual QString legendName() const;
    //дополнительная обработка, напр.
    //применение смещения и усиления,
    //для получения реального значения
    virtual double postprocess(double v) {return v;}
    double getValue(QDataStream &readStream);
    void setValue(double val, QDataStream &writeStream);



    virtual QString xName() const;
    virtual QString yName() const {return YName;}
    virtual double xBegin() const;
    virtual double xStep() const {return XStep;}
    virtual quint32 samplesCount() const;
    virtual double *yValues() {return YValues;}
    virtual double xMaxInitial() const {return XMaxInitial;}
    virtual double yMinInitial() const {return YMinInitial;}
    virtual double yMaxInitial() const {return YMaxInitial;}
    virtual void addCorrection(double correctionValue, bool writeToFile);

    virtual QString description() const {return ChanDscr;}
    virtual void setDescription(const QString &description) {ChanDscr = description;}

    virtual QString name() const {return ChanName;}
    virtual void setName(const QString &name) {ChanName = name;}

    virtual bool populated() const {return _populated;}
    virtual void setPopulated(bool populated) {_populated = populated;}
    virtual void populate();

//    virtual bool typeDiffers(Channel *other);

    /**
     * @brief preprocess - подготавливает значение к записи с помощью setValue
     * @param v - значение
     * @return подготовленное значение
     */
    virtual double preprocess(double v) {return v;}

    QString ChanAddress; //
    QString ChanName; //
    quint32 IndType; //характеристика отсчета
    quint32 ChanBlockSize; //размер блока в отсчетах
    QString YName;
    QString YNameOld;
    QString XName;
    QString InputType;
    QString ChanDscr;

    quint32 blockSizeInBytes() const; //размер блока в байтах

    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double XStep;
    double XMaxInitial; // initial xMax value to display
    double YMinInitial; // initial yMin value to display
    double YMaxInitial; // initial yMax value to display

    double *YValues;
    DfdFileDescriptor *parent;
    quint32 channelIndex; // нумерация с 0

    bool _populated;
    quint32 NumInd;

    bool corrected;
    QString nameBeforeCorrection;
    double oldCorrectionValue;
private:
    DfdDataType dataType;


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
    virtual ~RawChannel() {}
    virtual void read(QSettings &dfd, int chanIndex);
    virtual void write(QTextStream &dfd, int chanIndex);
    virtual QStringList getInfoHeaders();
    virtual QStringList getInfoData();
    virtual double postprocess(double v);
    virtual double preprocess(double v);
    void populate();
    QString SensName;
    double ADC0;
    double ADCStep;
    double AmplShift;
    double AmplLevel;
    double Sens0Shift;
    double SensSensitivity;
    float BandWidth;
};

class Source
{
public:
    /** [Source] */
    /** [Sources] */
    void read(QSettings &dfd);
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
    Process(DfdFileDescriptor *parent);
    void read(QSettings &dfd);
    void write(QTextStream &dfd);
    QString value(const QString &key);
private:
    DfdFileDescriptor *parent;
    QList<QPair<QString, QString> > data;
};

class DataDescription
{
public:
    DataDescription(DfdFileDescriptor *parent);
    void read(QSettings &dfd);
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
    ~DfdFileDescriptor();
    void read();
    void write();
    void writeRawFile();
    void populate();
    void updateDateTimeGUID();
    virtual void fillPreliminary(Descriptor::DataType);
    virtual void fillRest();

    QStringList info() const;
    QString dateTime() const;
    virtual Descriptor::DataType type() const;
    virtual DescriptionList dataDescriptor() const;
    virtual void setDataDescriptor(const DescriptionList &data);

    virtual double xStep() const {return XStep;}

//    QString fileName() {return FileName;}

    void setFileName(const QString &name);
    bool fileExists();

    void setDataChanged(bool changed);

    bool hasAttachedFile() const {return true;}
    QString attachedFileName() const {return rawFileName;}
    void setAttachedFileName(const QString &name) {rawFileName = name;}

    int channelsCount() const {return channels.size();}

    void deleteChannels(const QVector<int> &channelsToDelete);
    void copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy);
    virtual void calculateMean(const QMultiHash<FileDescriptor *, int> &channels);
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes);

    QStringList getHeadersForChannel(int channel);
    Channel *channel(int index);

    bool allUnplotted() const;
    bool isSourceFile() const;

    bool dataTypeEquals(FileDescriptor *other);

    QString fileFilters() const;

    virtual QString xName() const {return XName;}

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
    quint32 NumChans;
//    quint32 NumInd;
    quint32 BlockSize;
    QString XName;
//    double XBegin;
    double XStep;
    QString DescriptionFormat;
    QString CreatedBy;

    Source *source;
    Process *process;
    DataDescription *dataDescription;
    QList<DfdChannel *> channels;
private:
    friend class DataDescription;

    QString rawFileName; // путь к RAW файлу
    QString _legend; // editable description
};

#endif // DFDFILEDESCRIPTOR_H
