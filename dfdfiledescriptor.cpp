#include "dfdfiledescriptor.h"
#include <QtGui>

#define DebugPrint(s) qDebug()<<#s<<s;

bool readDfdFile(QIODevice &device, QSettings::SettingsMap &map)
{
    char buf[1024];
    qint64 lineLength = device.readLine(buf, sizeof(buf));
    QString group;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    while (lineLength != -1) {
        // the line is available in buf
        QByteArray b = QByteArray(buf, lineLength);
        if (b.isEmpty()) continue;

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
        lineLength = device.readLine(buf, sizeof(buf));
    }
    return true;
}

bool writeDfdFile(QIODevice &device, const QSettings::SettingsMap &map)
{
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
}

void DfdFileDescriptor::read()
{
    const QSettings::Format dfdFormat = QSettings::registerFormat("DFD", readDfdFile, writeDfdFile);

    QSettings dfd(dfdFileName,dfdFormat);

    QStringList childGroups = dfd.childGroups();

    //[DataFileDescriptor]
    dfd.beginGroup("DataFileDescriptor");
    rawFileName = dfd.value("DataReference").toString();
    if (rawFileName.isEmpty()) rawFileName = dfdFileName.left(dfdFileName.length()-4)+".raw";
    DataReference = rawFileName;
    DFDGUID = dfd.value("DFDGUID").toString();
    DataType = DfdDataType(dfd.value("DataType").toInt());
    Date=QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
    Time=QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
    dateTime = QDateTime(Date, Time);
    NumChans = dfd.value("NumChans").toInt(); //DebugPrint(NumChans);
    NumInd = dfd.value("NumInd").toInt();      //DebugPrint(NumInd);
    BlockSize = dfd.value("BlockSize").toInt();   //DebugPrint(BlockSize);
    XName = dfd.value("XName").toString();
    XBegin = hextodouble(dfd.value("XBegin").toString());   //DebugPrint(XBegin);
    XStep = hextodouble(dfd.value("XStep").toString());
    DescriptionFormat = dfd.value("DescriptionFormat").toString();
    CreatedBy = dfd.value("CreatedBy").toString();
    dfd.endGroup();

    // [DataDescription]
    dfd.beginGroup("DataDescription");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
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
    for (int i=0; i<NumChans; ++i) {
        Channel ch;
        ch.read(dfd, i+1);
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
        default: return new AbstractProcess();
    }
    return new AbstractProcess();
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
    blockSizeInBytes = ChanBlockSize * (IndType % 16);

    ADC0 = hextodouble( dfd.value("ADC0").toString()); //qDebug()<<"ADC0" << ch.adc0;
    ADCStep = hextodouble(dfd.value("ADCStep").toString()); //qDebug()<< "ADCStep"<< ch.adcStep;
    AmplShift = hextodouble(dfd.value("AmplShift").toString()); //qDebug()<<"AmplShift" <<ch.amplShift ;
    AmplLevel = hextodouble(dfd.value("AmplLevel").toString()); //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    YName = dfd.value("YName").toString();
    YNameOld = dfd.value("YNameOld").toString();
    Sens0Shift = hextodouble(dfd.value("Sens0Shift").toString()); //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    SensSensitivity = hextodouble(dfd.value("SensSensitivity").toString()); //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    SensName = dfd.value("SensName").toString();
    InputType = dfd.value("InputType").toString();
    BandWidth = hextofloat(dfd.value("BandWidth").toString()); //qDebug()<< "BandWidth"<< ch.bandwidth;
    ChanDscr = dfd.value("ChanDscr").toString();
    dfd.endGroup();
}
