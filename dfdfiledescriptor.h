#ifndef DFDFILEDESCRIPTOR_H
#define DFDFILEDESCRIPTOR_H

#include <QtCore>

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
    DiNikeP    = 154,           // диаграмма Найквиста для передаточной функции
    GSpectr    = 155,		// спектр Гильберта
    OSpectr    = 156,		// октавный спектр
    ToSpectr   = 157,		// 1/3-октавный спектр
    NotDef     = 255		// неопределенный
};

class Channel
{
public:
    /** [Channel#] */
    Channel() : IndType(0),
        ChanBlockSize(0),
        blockSizeInBytes(0),
        ADC0(0.0),
        ADCStep(0.0),
        AmplShift(0.0),
        AmplLevel(0.0),
        Sens0Shift(0.0),
        SensSensitivity(0.0),
        BandWidth(0.0)
    {}
    void read(QSettings &dfd, int chanIndex);
    QString ChanAddress;
    QString ChanName;
    quint32 IndType;
    int ChanBlockSize;
    int blockSizeInBytes;
    double ADC0;
    double ADCStep;
    double AmplShift;
    double AmplLevel;
    double Sens0Shift;
    double SensSensitivity;
    QString YName;
    QString YNameOld;
    QString SensName;
    QString InputType;
    float BandWidth;
    QString ChanDscr;
    //QByteArray data;
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
    int BlockIn; //4096
    QString Wind; //Хеннинга
    QString TypeAver; //линейное
    int NAver; //300
    QString Values; //измеряемые
    QString TypeScale; //в децибелах
};

class DfdFileDescriptor
{
public:
    DfdFileDescriptor(const QString &fileName);
    ~DfdFileDescriptor();
    void read();
    void writeDfd();
    AbstractProcess *getProcess(DfdDataType DataType);
    bool operator==(const DfdFileDescriptor &dfd){
        return (this->DFDGUID == dfd.DFDGUID);
    }

    QString dfdFileName;
    QString rawFileName; // путь к RAW файлу
    QDateTime dateTime;

    //[DataFileDescriptor]
    QString DFDGUID; //{7FD333E3-9A20-2A3E-A9443EC17B134848}
    DfdDataType DataType; // см. выше
    QDate Date;
    QTime Time;
    int NumChans;
    int NumInd;
    int BlockSize;
    QString XName;
    double XBegin;
    double XStep;
    QString DescriptionFormat;
    QString CreatedBy;
    QString DataReference; // путь к RAW файлу

    //[Sources], [Source]
    Source *source;
    //[Process]
    AbstractProcess *process;


    QMap<QString, QString> userComments;
    //[Channel#]
    QList<Channel> channels;
};

#endif // DFDFILEDESCRIPTOR_H
