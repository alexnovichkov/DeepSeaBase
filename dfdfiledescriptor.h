#ifndef DFDFILEDESCRIPTOR_H
#define DFDFILEDESCRIPTOR_H

#include <QtCore>

//#include "qcustomplot.h"

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

PlotType plotTypeByDataType(DfdDataType dataType);

QString dataTypeDescription(int type);
QString methodDescription(int methodType);
QString dllForMethod(int methodType);
int panelTypeForMethod(int methodType);
DfdDataType dataTypeForMethod(int methodType);


class DfdFileDescriptor;

class Channel
{
public:
    /** [Channel#] */
    Channel(DfdFileDescriptor *parent, int channelIndex)
        : IndType(0),
          ChanBlockSize(0),
          blockSizeInBytes(0),
          NumInd(0),
          xMin(0.0),
          xMax(0.0),
          yMin(0.0),
          yMax(0.0),
          xStep(0.0),
          xMaxInitial(0.0),
          yMinInitial(0.0),
          yMaxInitial(0.0),
          yValues(0),
          parent(parent),
          channelIndex(channelIndex),
          checkState(Qt::Unchecked),
          populated(false)
    {}
    virtual ~Channel();
    virtual void read(QSettings &dfd, int chanIndex);
    virtual QStringList getHeaders();
    virtual QStringList getData();
    virtual void populateData();
    virtual QString legendName();
    //дополнительная обработка, напр.
    //применение смещения и усиления,
    //для получения реального значения
    virtual double postprocess(double v) {return v;}
    double getValue(QDataStream &readStream);
    QString ChanAddress; //
    QString ChanName; //
    quint32 IndType; //характеристика отсчета
    quint32 ChanBlockSize; //размер блока в отсчетах
    quint32 blockSizeInBytes; //размер блока в байтах
    quint8 sampleSize; //размер отсчета в байтах
    quint32 NumInd; //общее число отсчетов
    QString YName;
    QString InputType;
    QString ChanDscr;

    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double xStep;
    double xMaxInitial; // initial xMax value to display
    double yMinInitial; // initial yMin value to display
    double yMaxInitial; // initial yMax value to display

    double *yValues;
    DfdFileDescriptor *parent;
    quint32 channelIndex;

    Qt::CheckState checkState;

    PlotType plotType;

    bool populated;
};

class RawChannel : public Channel
{
public:
    /** [Channel#] */
    RawChannel(DfdFileDescriptor *parent, int channelIndex)
        : Channel(parent, channelIndex),
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
    virtual QStringList getHeaders();
    virtual QStringList getData();
    virtual double postprocess(double v);
    void populateData();
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
    QString File; // название файла источника
    QString DFDGUID; // GUID файла источника
    QDate Date; // дата создания файла источника
    QTime Time; // время создания файла источника
    // новый формат
    QString sFile; // K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
    QList<int> ProcChansList;
};

class AbstractProcess
{
public:
    /** [Process] */
    virtual ~AbstractProcess() {}
    virtual void read(QSettings &dfd);
    QString PName; //Передаточная ф-я H1
    QList<int> ProcChansList; //1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19
};

class TransFuncProcess : public AbstractProcess
{
public:
    TransFuncProcess() : pTime(0.0),
        BlockIn(0),
        NAver(0)
    {}
    virtual ~TransFuncProcess() {}
    virtual void read(QSettings &dfd);
    double pTime; //(0000000000000000)
    QString pBaseChan; //2,MBU00002\2,Сила,Н
    quint32 BlockIn; //4096
    QString Wind; //Хеннинга
    QString TypeAver; //линейное
    quint32 NAver; //300
    QString Values; //измеряемые
    QString TypeScale; //в децибелах
};

class DataDescription
{
public:
    DataDescription();
    void read(QSettings &dfd);
    QString toString() const;
private:
    QList<QPair<QString, QString> > data;
};

class DfdFileDescriptor
{
public:
    DfdFileDescriptor(const QString &fileName);
    ~DfdFileDescriptor();
    void read();
    void writeDfd();
    AbstractProcess *getProcess(DfdDataType DataType);
    Channel *getChannel(DfdDataType DataType, int chanIndex);
    bool operator==(const DfdFileDescriptor &dfd){
        return (this->DFDGUID == dfd.DFDGUID);
    }
    QString description() const;

    QString dfdFileName;
    QString rawFileName; // путь к RAW файлу
    QDateTime dateTime;

    //[DataFileDescriptor]
    QString DFDGUID; //{7FD333E3-9A20-2A3E-A9443EC17B134848}
    DfdDataType DataType; // см. выше
    QDate Date;
    QTime Time;
    quint32 NumChans;
    quint32 NumInd;
    quint32 BlockSize;
    QString XName;
    double XBegin;
    double XStep;
    QString DescriptionFormat;
    QString CreatedBy;
    QString DataReference; // путь к RAW файлу

    QString legend; // editable description

    //[Sources], [Source]
    Source *source;
    //[Process]
    AbstractProcess *process;

    DataDescription *dataDescription;
    //[DataDescription]
    //QMap<QString, QString> userComments;
    //[Channel#]
    QList<Channel *> channels;


};

#endif // DFDFILEDESCRIPTOR_H
