#include "dfdfiledescriptor.h"
#include <QtCore>

#include "converters.h"

#include <QUuid>

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
    qint64 lineLength;
    QString group;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    while ((lineLength = device.readLine(buf, sizeof(buf))) != -1) {
        // the line is available in buf
        QByteArray b = QByteArray(buf, lineLength);
        if (!b.isEmpty()) {
            QString s = codec->toUnicode(b);
            s = s.trimmed();
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
    }
    return true;
}

bool writeDfdFile(QIODevice &/*device*/, const QSettings::SettingsMap &/*map*/)
{
    return true;
}



DfdFileDescriptor::DfdFileDescriptor(const QString &fileName)
    : DataType(NotDef),
      NumChans(0),
      NumInd(0),
      BlockSize(0),
      XBegin(0.0),
      XStep(0.0),
      source(0),
      process(0),
      dataDescription(0)
{
    dfdFileName = fileName;
    rawFileChanged = false;
    changed = false;
}

DfdFileDescriptor::~DfdFileDescriptor()
{
    if (changed)
        write();
    if (rawFileChanged)
        writeRawFile();

    delete process;
    delete source;
    delete dataDescription;
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
    DFDGUID =   dfd.value("DFDGUID").toString();
    DataType =  DfdDataType(dfd.value("DataType").toInt());
    Date =      QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
    Time =      QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
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
    if (childGroups.contains("DataDescription")) {
        dataDescription = new DataDescription(this);
        dataDescription->read(dfd);
    }

    // [Source]
    if (childGroups.contains("Source") || childGroups.contains("Sources")) {
        source = new Source;
        source->read(dfd);
    }

    // [Process]
    if (childGroups.contains("Process")) {
        process = new Process(this);
        process->read(dfd);
    }

    // [Channel#]
    for (quint32 i=0; i<NumChans; ++i) {
        Channel *ch = getChannel(DataType, i);
        ch->read(dfd, i+1);
        channels << ch;
    }
}

void DfdFileDescriptor::write()
{
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");

    QFile file(dfdFileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) return;
    QTextStream dfd(&file);
    dfd.setCodec(codec);

    // убираем перекрытие блоков
    BlockSize = 0;

    /** [DataFileDescriptor]*/
    dfd << "[DataFileDescriptor]" << endl;
    dfd << "DFDGUID="<<DFDGUID << endl;
    dfd << "DataType="<<DataType << endl;
    dfd << "Date="<<Date.toString("dd.MM.yyyy") << endl;
    dfd << "Time="<<Time.toString("hh:mm:ss") << endl;
    dfd << "NumChans="<<NumChans << endl;
    dfd << "NumInd="<<NumInd << endl;
    dfd << "BlockSize="<<BlockSize << endl;
    dfd << "XName="<<XName << endl;
    dfd << "XBegin="<<doubletohex(XBegin).toUpper() << endl;
    dfd << "XStep="<<doubletohex(XStep).toUpper() << endl;
    dfd << "DescriptionFormat="<<DescriptionFormat << endl;
    dfd << "CreatedBy="<<CreatedBy << endl;

    /** [DataDescription] */
    if (dataDescription) {
        dataDescription->write(dfd);
    }

    /** [Source] */
    if (source) {
        source->write(dfd);
    }

    /** [Process] */
    if (process) {
        process->write(dfd);
    }

    /** Channels*/
    for (uint i=0; i<NumChans; ++i) {
        Channel *ch = channels[i];
        ch->write(dfd, i+1);
    }
}

void DfdFileDescriptor::writeRawFile()
{
    //be sure all channels were read. May take too much memory
    foreach(Channel *ch, channels) {
        if (!ch->populated) ch->populateData();
    }


    QFile rawFile(rawFileName);
    if (rawFile.open(QFile::WriteOnly)) {
        QDataStream writeStream(&rawFile);
        writeStream.setByteOrder(QDataStream::LittleEndian);

        if (BlockSize == 0) {
            // пишем поканально
            for (quint32 i = 0; i<NumChans; ++i) {
                Channel *ch = channels[i];

                if (ch->IndType==0xC0000004)
                    writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                for (quint32 val = 0; val < ch->NumInd; ++val) {
                    ch->setValue(ch->yValues[val], writeStream);
                }
            }
        }
        else {
            // пишем блоками размером BlockSize
            qDebug() << "Oops! Пытаемся писать с перекрытием?";
        }
    }
}

void DfdFileDescriptor::setLegend(const QString &legend)
{
    if (legend == _legend) return;
    _legend = legend;
    changed = true;
}

Channel *DfdFileDescriptor::getChannel(DfdDataType DataType, int chanIndex)
{
    switch (DataType) {
        case SourceData: return new RawChannel(this, chanIndex); break;
        default: break;
    }
    return new Channel(this, chanIndex);
}

QString DfdFileDescriptor::description() const
{
    if (dataDescription) return dataDescription->toString();
    return dfdFileName;
}

QString DfdFileDescriptor::createGUID()
{
    QString result;
    result = QUuid::createUuid().toString().toUpper();
    if (result.at(24) == '-') result.remove(24,1);
    else result.remove(25,1);
    return result;
}

Process::Process(DfdFileDescriptor *parent) : parent(parent)
{

}

void Process::read(QSettings &dfd)
{
    dfd.beginGroup("Process");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
        data.append(QPair<QString, QString>(key, v));
    }

    dfd.endGroup();
}

void Process::write(QTextStream &dfd)
{
    dfd << "[Process]" << endl;
    for (int i = 0; i<data.size(); ++i) {
        QPair<QString, QString> item = data.at(i);
        dfd << item.first << "=" << item.second << endl;
    }
}

QString Process::value(const QString &key)
{
    for (int i = 0; i<data.size(); ++i) {
        QPair<QString, QString> item = data.at(i);
        if (item.first == key) return item.second;
    }
    return QString();
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

void Source::write(QTextStream &dfd)
{
    if (!sFile.isEmpty()) {
        dfd << "[Sources]" << endl;
        dfd << "sFile=" << sFile << endl;
    }
    else {
        dfd << "[Source]" << endl;
        dfd << "File=" << File << endl;
        dfd << "DFDGUID=" << DFDGUID << endl;
        dfd << "Date=" << Date.toString("dd.MM.yyyy") << endl;
        dfd << "Time=" << Time.toString("hh:mm:ss") << endl;
    }
}

Channel::~Channel()
{
    delete [] yValues;
}

void Channel::read(QSettings &dfd, int chanIndex)
{
    dfd.beginGroup(QString("Channel%1").arg(chanIndex));
    ChanAddress = dfd.value("ChanAddress").toString();
    ChanName = dfd.value("ChanName").toString();
    IndType = dfd.value("IndType").toUInt();
    ChanBlockSize = dfd.value("ChanBlockSize").toInt();

    blockSizeInBytes = ChanBlockSize * (IndType % 16);
    YName = dfd.value("YName").toString();
    YNameOld = dfd.value("YNameOld").toString();
    InputType = dfd.value("InputType").toString();
    ChanDscr = dfd.value("ChanDscr").toString();
    dfd.endGroup();

    NumInd = parent->BlockSize==0?ChanBlockSize:parent->NumInd*ChanBlockSize/parent->BlockSize;
    xStep = double(parent->XStep * NumInd/parent->NumInd);
}

void Channel::write(QTextStream &dfd, int chanIndex)
{
    dfd << QString("[Channel%1]").arg(chanIndex) << endl;
    dfd << "ChanAddress=" << ChanAddress << endl;
    dfd << "ChanName=" << ChanName << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << ChanBlockSize << endl;
    dfd << "YName=" << YName << endl;
    if (!YNameOld.isEmpty())
        dfd << "YNameOld=" << YNameOld << endl;
    dfd << "InputType="<<InputType << endl;
    dfd << "ChanDscr="<<ChanDscr << endl;
}

QStringList Channel::getInfoHeaders()
{
    return QStringList()
            //<< QString("Канал") //ChanAddress
            << QString("       Имя") //ChanName
            //<< QString("Вход") //InputType
            << QString("Ед.изм.") //YName
            << QString("Описание") //ChanDscr
               ;
}

QStringList Channel::getInfoData()
{
    return QStringList()
          //  << ChanAddress
            << ChanName
          //  << InputType
            << YName
            << ChanDscr
               ;
}

void Channel::populateData()
{
    // clear previous data;
    delete [] yValues;
    yValues = 0;

    QFile rawFile(parent->rawFileName);
    if (rawFile.open(QFile::ReadOnly)) {

        // число отсчетов в канале
        quint32 NI = NumInd;
        yValues = new double[NI];

        quint32 xCount = 0;

        yMin = 1.0e100;
        yMax = -1.0e100;

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

                        if (yValue < yMin) yMin = yValue;
                        if (yValue > yMax) yMax = yValue;

                        yValues[xCount] = yValue;

                        xCount++;
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

        populated = true;
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->rawFileName;
    }
}

QString Channel::legendName()
{
    QString result = parent->legend();
    if (!result.isEmpty()) result.prepend(" ");

    return (ChanName.isEmpty()?ChanAddress:ChanName) + result;
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

    return postprocess(realValue);
}

void Channel::setValue(double val, QDataStream &writeStream)
{
    double realValue = preprocess(val);

    switch (IndType) {
        case 0x00000001: {
            quint8 v = (quint8)realValue;
            writeStream << v;
            break;}
        case 0x80000001: {
            qint8 v = (qint8)realValue;
            writeStream << v;
            break;}
        case 0x00000002: {
            quint16 v = (quint16)realValue;
            writeStream << v;
            break;}
        case 0x80000002: {
            qint16 v = (qint16)realValue;
            writeStream << v;
            break;}
        case 0x00000004: {
            quint32 v = (quint32)realValue;
            writeStream << v;
            break;}
        case 0x80000004: {
            qint32 v = (qint32)realValue;
            writeStream << v;
            break;}
        case 0x80000008: {
            qint64 v = (qint64)realValue;
            writeStream << v;
            break;}
        case 0xC0000004: {
            float v = (float)realValue;
            writeStream << v;
            break;}
        case 0xC0000008:
        case 0xC000000A:
            writeStream << realValue;
            break;
        default: break;
    }
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

void RawChannel::write(QTextStream &dfd, int chanIndex)
{
    Channel::write(dfd,chanIndex);

    dfd << "ADC0=" <<  doubletohex(ADC0).toUpper() << endl;
    dfd << "ADCStep=" <<  doubletohex(ADCStep).toUpper() << endl; //qDebug()<< "ADCStep"<< ch.adcStep;
    dfd << "AmplShift=" << doubletohex(AmplShift).toUpper() << endl; //qDebug()<<"AmplShift" <<ch.amplShift ;
    dfd << "AmplLevel=" << doubletohex(AmplLevel).toUpper() << endl; //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    dfd << "Sens0Shift=" << doubletohex(Sens0Shift).toUpper() << endl; //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    dfd << "SensSensitivity=" << doubletohex(SensSensitivity).toUpper() << endl; //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    dfd << "BandWidth=" <<  floattohex(BandWidth).toUpper() << endl;
    dfd << "SensName=" <<  SensName << endl;
}

QStringList RawChannel::getInfoHeaders()
{
    QStringList result = Channel::getInfoHeaders();
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

QStringList RawChannel::getInfoData()
{
    // пересчитываем усиление из В в дБ
    double ampllevel = AmplLevel;
    if (ampllevel <= 0.0) ampllevel = 1.0;
    QStringList result = Channel::getInfoData();
    result.append(QStringList()
                  << QString::number(ADC0)
                  << QString::number(ADCStep)
                  << QString::number(AmplShift)
                  << QString::number(qRound(20*log(ampllevel)))
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

double RawChannel::preprocess(double v)
{
    return ((v * SensSensitivity + Sens0Shift + AmplShift) * AmplLevel - ADC0) / ADCStep;
}

void RawChannel::populateData()
{
    Channel::populateData();

    // rescale initial min and max values with first 200 values
    xMaxInitial = parent->XBegin + xStep*200;
    yMinInitial = 0.0;
    yMaxInitial = 0.0;

    if (!yValues) return;
    const int steps = qMin(NumInd, 200u);
    for (int i=0; i<steps; ++i) {
        double val = yValues[i];
        if (val > yMaxInitial) yMaxInitial = val;
        if (val < yMinInitial) yMinInitial = val;
    }
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


DataDescription::DataDescription(DfdFileDescriptor *parent) : parent(parent)
{

}

void DataDescription::read(QSettings &dfd)
{
    dfd.beginGroup("DataDescription");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
        if (v.startsWith("\"")) v.remove(0,1);
        if (v.endsWith("\"")) v.chop(1);
        v = v.trimmed();
        if (!v.isEmpty()) {
            if (key == "Legend")
                parent->_legend = v;
            else
                data.append(QPair<QString, QString>(key, v));
        }
    }
    dfd.endGroup();
}

void DataDescription::write(QTextStream &dfd)
{
    if (!data.isEmpty() || !parent->legend().isEmpty()) {
        dfd << "[DataDescription]" << endl;
        for (int i = 0; i<data.size(); ++i) {
            QPair<QString, QString> item = data.at(i);
            dfd << item.first << "=\"" << item.second << "\"" << endl;
        }
        if (!parent->legend().isEmpty())
            dfd << "Legend=" << "\"" << parent->legend() << "\"" << endl;
    }
}

QString DataDescription::toString() const
{
    QStringList result;
    for (int i=0; i<data.size(); ++i) {
        QPair<QString, QString> item = data.at(i);
        result.append(/*item.first+"="+*/item.second);
    }
    return result.join("; ");
}
