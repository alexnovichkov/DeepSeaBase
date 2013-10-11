#include "dfdfiledescriptor.h"
#include <QtGui>

QString dataTypeDescription(int type)
{
    switch (type) {
        case 0: return QString("Данные"); break;
        case 1: return QString("Данные1"); break;
        case 2: return QString("Фильтр. данные"); break;
        case 3: return QString("Данные3"); break;
        case 4: return QString("Данные4"); break;
        case 5: return QString("Данные5"); break;
        case 6: return QString("Данные6"); break;
        case 7: return QString("Данные7"); break;
        case 8: return QString("Данные8"); break;
        case 9: return QString("Данные9"); break;
        case 10: return QString("Данные10"); break;
        case 11: return QString("Данные11"); break;
        case 12: return QString("Данные12"); break;
        case 13: return QString("Данные13"); break;
        case 14: return QString("Данные14"); break;
        case 15: return QString("Данные15"); break;
        case 16: return QString("Огибающая"); break;
        case 17: return QString("Мат. ож."); break;
        case 18: return QString("СКЗ"); break;
        case 19: return QString("Ассиметрия"); break;
        case 20: return QString("Эксцесс"); break;
        case 21: return QString("Фаза (Гильб.)"); break;
        case 32: return QString("Корреляция"); break;
        case 33: return QString("X-Корр."); break;
        case 64: return QString("Гистограмма"); break;
        case 65: return QString("ЭФР"); break;
        case 66: return QString("Гистограмма (%)"); break;
        case 67: return QString("Плотн. вероятн."); break;
        case 128: return QString("Спектр"); break;
        case 129: return QString("ПСМ"); break;
        case 130: return QString("Спектр СКЗ"); break;
        case 144: return QString("X-Спектр"); break;
        case 145: return QString("X-Фаза"); break;
        case 146: return QString("Когерентность"); break;
        case 147: return QString("ПФ"); break;
        case 148: return QString("X-Спектр Re"); break;
        case 149: return QString("X-Спектр Im"); break;
        case 150: return QString("ПФ Re"); break;
        case 151: return QString("ПФ Im"); break;
        case 152: return QString("ДН X-Спектр"); break;
        case 153: return QString("Кепстр"); break;
        case 154: return QString("ДН ПФ"); break;
        case 155: return QString("Спектр огиб."); break;
        case 156: return QString("Окт.спектр"); break;
        case 157: return QString("Тр.окт.спектр"); break;
        default: return QString("Неопр.");
    }
    return QString("Неопр.");
}

PlotType plotTypeByDataType(DfdDataType dataType)
{
    if ((dataType>=0 && dataType<=31) || dataType == 153) return PlotTime;
    if (dataType>=32 && dataType<=33) return PlotCorrelation;
    if (dataType>=64 && dataType<=127) return PlotStatistics;
    if (dataType>=128 && dataType<=151) return PlotSpectre;
    if (dataType==156 || dataType==157) return PlotOctave;
    if (dataType==152 || dataType==154) return PlotNykvist;
    return PlotUnknown;
}

bool readDfdFile(QIODevice &device, QSettings::SettingsMap &map)
{
    char buf[2048];
    qint64 lineLength = device.readLine(buf, sizeof(buf));
    QString group;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    while (lineLength != -1) {
        // the line is available in buf
        QByteArray b = QByteArray(buf, lineLength);
        if (!b.isEmpty()) {
            //b.replace('\\',"\\\\");
            //b.replace("\\\\",'\\');
            QString s = codec->toUnicode(b);
            s.chop(2);
            if (s.startsWith('[')) {
                s.chop(1);
                s.remove(0,1);
                group = s;
            }
            else {
                int ind = s.indexOf('=');
                if (ind>0) {
                    QString key = group+"/"+s.mid(0,ind);
                    QString val = s.mid(ind+1);
                    map.insert(key,val);
                }
            }
        }
        lineLength = device.readLine(buf, sizeof(buf));
    }
    return true;
}

bool writeDfdFile(QIODevice &device, const QSettings::SettingsMap &map)
{
    Q_UNUSED(device)
    Q_UNUSED(map)
    return true;
}

QString doubletohex(const double d)
{
    QString s;
    QByteArray ba;
    QDataStream stream(&ba,QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << d;
    s = "("+ba.toHex()+")";
    return s;
}

double hextodouble(QString hex)
{
    if (hex.startsWith("("))
        hex.remove(0,1);
    if (hex.endsWith(")"))
        hex.chop(1);

    double result=0.0l;

    QByteArray ba;
    for (int i=0; i<16; i+=2) {
        quint8 c = hex.mid(i,2).toUInt(0,16);
        ba.append(c);
    }

    QDataStream stream(ba);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> result;

    return result;
}



float hextofloat(QString hex)
{
    if (hex.startsWith("("))
        hex.remove(0,1);
    if (hex.endsWith(")"))
        hex.chop(1);

    float result=0.0;

    QByteArray ba;
    for (int i=0; i<8; i+=2) {
        quint8 c = hex.mid(i,2).toUInt(0,16);
        ba.append(c);
    }

    QDataStream stream(ba);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> result;

    return result;
}

QString floattohex(const float f)
{
    QString s;
    QByteArray ba;
    QDataStream stream(&ba,QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << f;
    s = "("+ba.toHex()+")";
    return s;
}

DfdFileDescriptor::DfdFileDescriptor(const QString &fileName)
    : DataType(NotDef),
      NumChans(0),
      NumInd(0),
      BlockSize(0),
      XBegin(0.0),
      XStep(0.0),
      source(0),
      process(0)
{
    dfdFileName = fileName;
}

DfdFileDescriptor::~DfdFileDescriptor()
{
    delete process;
    delete source;
    qDeleteAll(channels);
}

void DfdFileDescriptor::read()
{
    static const QSettings::Format dfdFormat = QSettings::registerFormat("DFD", readDfdFile, writeDfdFile);

    QSettings dfd(dfdFileName,dfdFormat);

    QStringList childGroups = dfd.childGroups();

    //[DataFileDescriptor]
    dfd.beginGroup("DataFileDescriptor");
    rawFileName = dfd.value("DataReference").toString();
    if (rawFileName.isEmpty())
        rawFileName = dfdFileName.left(dfdFileName.length()-4)+".raw";
    DataReference = rawFileName;
    DFDGUID =   dfd.value("DFDGUID").toString();
    DataType =  DfdDataType(dfd.value("DataType").toInt());
    Date =      QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
    Time =      QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
    dateTime =  QDateTime(Date, Time);
    NumChans =  dfd.value("NumChans").toInt(); //DebugPrint(NumChans);
    NumInd =    dfd.value("NumInd").toUInt();      //DebugPrint(NumInd);
    BlockSize = dfd.value("BlockSize").toInt();   //DebugPrint(BlockSize);
    XName =     dfd.value("XName").toString();  //DebugPrint(XName);
    XBegin =    hextodouble(dfd.value("XBegin").toString());   //DebugPrint(XBegin);
    XStep =     hextodouble(dfd.value("XStep").toString()); //DebugPrint(XStep);
    DescriptionFormat = dfd.value("DescriptionFormat").toString(); //DebugPrint(DescriptionFormat);
    CreatedBy = dfd.value("CreatedBy").toString(); //DebugPrint(CreatedBy);
    dfd.endGroup();

    // [DataDescription]
    dfd.beginGroup("DataDescription");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
        if (v.startsWith("\"")) v.remove(0,1);
        if (v.endsWith("\"")) v.chop(1);
        v = v.trimmed();
        userComments.insert(key,v);
    }
    dfd.endGroup();

    // [Source]
    if (childGroups.contains("Source") || childGroups.contains("Sources")) {
        source = new Source;
        source->read(dfd);
    }

    // [Process]
    if (childGroups.contains("Process")) {
        process = getProcess(DataType);
        process->read(dfd);
    }

    // [Channel#]
    for (quint32 i=0; i<NumChans; ++i) {
        Channel *ch = getChannel(DataType, i);
        ch->read(dfd, i+1);
        channels << ch;
    }
}

void DfdFileDescriptor::writeDfd()
{
    //    QSettings dfd(dfdFileName,QSettings::IniFormat);
    //    dfd.setIniCodec(QTextCodec::codecForName("CP1251"));


    //    if (rawFileName.isEmpty()) rawFileName = dfdFileName.left(dfdFileName.length()-4)+".raw";

    //    /** [DataFileDescriptor]*/
    //    dfd.beginGroup("DataFileDescriptor");
    //    dfd.setValue("DFDGUID",QUuid::createUuid().toString().toUpper());
    //    //dfd.setValue("DataReference",rawFileName);
    //    dfd.setValue("DataType",dataType); //we write only raw time data
    //    dfd.setValue("Date",date.toString("dd.MM.yyyy"));
    //    dfd.setValue("Time",time.toString("hh:mm:ss"));
    //    dfd.setValue("NumChans",numChans);
    //    dfd.setValue("NumInd",numInd);
    //    dfd.setValue("BlockSize",blockSize);
    //    dfd.setValue("XName",xName);

    //    dfd.setValue("XBegin",doubletohex(xBegin).toUpper());
    //    dfd.setValue("XStep",doubletohex(xStep).toUpper());
    //    dfd.setValue("DescriptionFormat","");
    //    dfd.setValue("CreatedBy","UFF2DeepSea");
    //    dfd.endGroup();

    //    /** [DataDescription]*/
    //    dfd.beginGroup("[DataDescription]");
    //    dfd.endGroup();

    //    /** Channels*/
    //    // [Channel1]
    //    for (int i=0; i<numChans; ++i) {
    //        dfd.beginGroup(QString("Channel%1").arg(i+1));
    //        const Channel &ch = channels.at(i);
    //        dfd.setValue("ChanAddress",ch.chanAddress);
    //        dfd.setValue("ChanName",ch.chanName);
    //        dfd.setValue("IndType",ch.indType);
    //        dfd.setValue("ChanBlockSize",ch.chanBlockSize);
    //        dfd.setValue("ADC0",doubletohex(ch.adc0).toUpper());
    //        dfd.setValue("ADCStep",doubletohex(ch.adcStep).toUpper());
    //        dfd.setValue("AmplShift",doubletohex(ch.amplShift).toUpper());
    //        dfd.setValue("AmplLevel",doubletohex(ch.amplLevel).toUpper());
    //        dfd.setValue("YName",ch.yName);
    //        dfd.setValue("Sens0Shift",doubletohex(ch.sens0Shift).toUpper());
    //        dfd.setValue("SensSensitivity",doubletohex(ch.sensSensitivity).toUpper());
    //        dfd.setValue("SensName",ch.sensName);
    //        dfd.setValue("InputType",ch.inputType);
    //        dfd.setValue("BandWidth",floattohex(ch.bandwidth).toUpper());
    //        dfd.setValue("ChanDscr",ch.chanDescr);
    //        dfd.endGroup();
    //    }
}

AbstractProcess *DfdFileDescriptor::getProcess(DfdDataType DataType)
{
    switch (DataType) {
        case SourceData: return new AbstractProcess(); break;
        case TransFunc: return new TransFuncProcess(); break;
        default: break;
    }
    return new AbstractProcess();
}

Channel *DfdFileDescriptor::getChannel(DfdDataType DataType, int chanIndex)
{
    switch (DataType) {
        case SourceData: return new RawChannel(this, chanIndex); break;
        default: break;
    }
    return new Channel(this, chanIndex);
}

void TransFuncProcess::read(QSettings &dfd)
{
    AbstractProcess::read(dfd);
    dfd.beginGroup("Process");
    pTime = hextodouble(dfd.value("pTime").toString()); //(0000000000000000)
    pBaseChan = dfd.value("pBaseChan").toString(); //2,MBU00002\2,Сила,Н
    BlockIn = dfd.value("BlockIn").toInt(); //4096
    Wind = dfd.value("Wind").toString();  //Хеннинга
    TypeAver = dfd.value("TypeAver").toString();  //линейное
    NAver = dfd.value("NAver").toInt(); //300
    Values = dfd.value("Values").toString(); //измеряемые
    TypeScale = dfd.value("TypeScale").toString(); //в децибелах
    dfd.endGroup();
}


void AbstractProcess::read(QSettings &dfd)
{
    dfd.beginGroup("Process");
    PName = dfd.value("PName").toString();
    QStringList procChansList = dfd.value("ProcChansList").toString().split(",");
    foreach (QString s, procChansList) ProcChansList << s.toInt();
    dfd.endGroup();
}


void Source::read(QSettings &dfd)
{
    dfd.beginGroup("Sources");
    sFile = dfd.value("sFile").toString();
    if (!sFile.isEmpty()) {
        //K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
        File = sFile.section("[",0,0);
        QStringList procChansList = sFile.section("[",1).section("]",0,0).split(",");
        foreach (QString s, procChansList) ProcChansList << s.toInt();
        DFDGUID = "{"+sFile.section("{",1);
        dfd.endGroup();
    }
    else {
        dfd.beginGroup("Source");
        File = dfd.value("File").toString();
        DFDGUID = dfd.value("DFDGUID").toString();
        Date=QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
        Time=QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
        dfd.endGroup();
    }
}

void Channel::read(QSettings &dfd, int chanIndex)
{
    dfd.beginGroup(QString("Channel%1").arg(chanIndex));
    ChanAddress = dfd.value("ChanAddress").toString();
    ChanName = dfd.value("ChanName").toString();
    IndType = dfd.value("IndType").toUInt();
    ChanBlockSize = dfd.value("ChanBlockSize").toInt();
    sampleSize = IndType % 16;
    blockSizeInBytes = ChanBlockSize * sampleSize;
    YName = dfd.value("YName").toString();
    InputType = dfd.value("InputType").toString();
    ChanDscr = dfd.value("ChanDscr").toString();
    dfd.endGroup();
    legendName = parent->dfdFileName+"/"+(ChanName.isEmpty()?ChanAddress:ChanName);

    NumInd = parent->BlockSize==0?ChanBlockSize:parent->NumInd*ChanBlockSize/parent->BlockSize;
    xStep = double(parent->XStep * NumInd/parent->NumInd);
}

QStringList Channel::getHeaders()
{
    return QStringList()
            << QString("Канал") //ChanAddress
            << QString("Имя") //ChanName
            << QString("Вход") //InputType
            << QString("Ед.изм.") //YName
            << QString("Описание") //ChanDscr
               ;
}

QStringList Channel::getData()
{
    return QStringList()
            << ChanAddress
            << ChanName
            << InputType
            << YName
            << ChanDscr
               ;
}

void Channel::populateData()
{
    // clear previous data;
    if (data) delete data;
    data = new QCPDataMap();

    QFile rawFile(parent->rawFileName);
    if (rawFile.open(QFile::ReadOnly)) {

        // число отсчетов в канале

        quint32 NI=NumInd;

        quint32 xCount = 0;

        while (NI>0) {
            //read NumChans chunks
            //так как размеры блока разных каналов могут не совпадать, приходится учитывать все каналы
            for (quint32 i=0; i<parent->NumChans; ++i) {
                if (i==channelIndex) {
                    QByteArray rawBlock = rawFile.read(parent->channels.at(i)->blockSizeInBytes);
                    QDataStream readStream(rawBlock);
                    readStream.setByteOrder(QDataStream::LittleEndian);
                    if (IndType==0xC0000004)
                        readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                    while (!readStream.atEnd()) {
                        double yValue = getValue(readStream);
                        double xValue = parent->XBegin + xStep*(xCount++);

                        if (yValue < yMin) yMin = yValue;
                        if (yValue > yMax) yMax = yValue;

                        data->insertMulti(xValue, yValue);
                    }
                    NI -= ChanBlockSize;
                }
                else {
                    rawFile.seek(rawFile.pos()+parent->channels.at(i)->blockSizeInBytes);
                }
            }
        }
        xMin = parent->XBegin;
        xMax = xMin + xStep * xCount;
        xMaxInitial = xMax;
        yMinInitial = yMin;
        yMaxInitial = yMax;
    }
    else {
        delete data;
        data=0;
        qDebug()<<"Cannot read raw file"<<parent->rawFileName;
    }
}

double Channel::getValue(QDataStream &readStream)
{
    double realValue = 0.0;
    switch (IndType) {
        case 0x00000001: {
            quint8 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x80000001: {
            qint8 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x00000002: {
            quint16 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x80000002: {
            qint16 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x00000004: {
            quint32 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x80000004: {
            qint32 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0x80000008: {
            qint64 v;
            readStream >> v;
            realValue = (double)v;
            break;}
        case 0xC0000004: {
            float v;
            readStream >> v;
            realValue = v;
            break;}
        case 0xC0000008:
        case 0xC000000A:
            readStream >> realValue;
            break;
        default: break;
    }
//    qDebug()<<realValue;

    return postprocess(realValue);
}

void RawChannel::read(QSettings &dfd, int chanIndex)
{
    Channel::read(dfd,chanIndex);
    dfd.beginGroup(QString("Channel%1").arg(chanIndex));
    ADC0 = hextodouble( dfd.value("ADC0").toString()); //qDebug()<<"ADC0" << ch.adc0;
    ADCStep = hextodouble(dfd.value("ADCStep").toString()); //qDebug()<< "ADCStep"<< ch.adcStep;
    AmplShift = hextodouble(dfd.value("AmplShift").toString()); //qDebug()<<"AmplShift" <<ch.amplShift ;
    AmplLevel = hextodouble(dfd.value("AmplLevel").toString()); //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    Sens0Shift = hextodouble(dfd.value("Sens0Shift").toString()); //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    SensSensitivity = hextodouble(dfd.value("SensSensitivity").toString()); //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    BandWidth = hextofloat(dfd.value("BandWidth").toString()); //qDebug()<< "BandWidth"<< ch.bandwidth;
    SensName = dfd.value("SensName").toString();
    dfd.endGroup();
}

QStringList RawChannel::getHeaders()
{
    QStringList result = Channel::getHeaders();
    result.append(QStringList()
                  << QString("Смещ.") //ADC0
                  << QString("Множ.") //ADCStep
                  << QString("Смещ.ус.(В)") //AmplShift
                  << QString("Ус.(дБ)") //AmplLevel
                  << QString("Смещ.датч.(мВ)") //Sens0Shift
                  << QString("Чувств.") //SensSensitivity
                  << QString("Полоса") //BandWidth
                  << QString("Датчик") //SensName
                  );
    return result;
}

QStringList RawChannel::getData()
{
    QStringList result = Channel::getData();
    result.append(QStringList()
                  << QString::number(ADC0)
                  << QString::number(ADCStep)
                  << QString::number(AmplShift)
                  << QString::number(AmplLevel)
                  << QString::number(Sens0Shift)
                  << QString::number(SensSensitivity)
                  << QString::number(BandWidth)
                  << SensName
                  );
    return result;
}

double RawChannel::postprocess(double v)
{
    return ((v*ADCStep+ADC0)/AmplLevel - AmplShift - Sens0Shift)/SensSensitivity;
}

void RawChannel::populateData()
{
    Channel::populateData();

//    DebugPrint(xMax);
//    DebugPrint(xMin);
//    DebugPrint(yMax);
//    DebugPrint(yMin);
//    DebugPrint(xMaxInitial);
//    DebugPrint(xMin);
//    DebugPrint(yMaxInitial);
//    DebugPrint(yMinInitial);

    // rescale initial min and max values with first 200 values
    xMaxInitial = parent->XBegin + xStep*200;
    yMinInitial = 0.0;
    yMaxInitial = 0.0;
    QCPDataMapIterator it(*data);
    int steps = 200;
    while (it.hasNext() && steps > 0) {
        it.next();
        double val = it.value();
        if (val > yMaxInitial) yMaxInitial = val;
        if (val < yMinInitial) yMinInitial = val;
        steps--;
    }
//    DebugPrint("after:");
//    DebugPrint(xMax);
//    DebugPrint(xMin);
//    DebugPrint(yMax);
//    DebugPrint(yMin);
//    DebugPrint(xMaxInitial);
//    DebugPrint(xMin);
//    DebugPrint(yMaxInitial);
//    DebugPrint(yMinInitial);
}


QString dllForMethod(int methodType)
{
    if (methodType<0 || methodType>25) return "";
    return methods[methodType].methodDll;
}


QString methodDescription(int methodType)
{
    if (methodType<0 || methodType>25) return "";
    return methods[methodType].methodDescription;
}


int panelTypeForMethod(int methodType)
{
    if (methodType<0 || methodType>25) return -1;
    return methods[methodType].panelType;
}


DfdDataType dataTypeForMethod(int methodType)
{
    if (methodType<0 || methodType>25) return NotDef;
    return methods[methodType].dataType;
}
