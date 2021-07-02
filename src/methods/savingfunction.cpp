#include "savingfunction.h"
//#include "fileformats/filedescriptor.h"
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "fileformats/data94file.h"
#include "logging.h"

//returns "uff" by default
QString getSuffixByType(int type)
{DD;
    switch (type) {
        case SavingFunction::DfdFile: return "dfd";
        case SavingFunction::UffFile: return "uff";
        case SavingFunction::D94File: return "d94";
    }
    return "d94";
}

DescriptionList processData(const QStringList &data)
{DD;
    DescriptionList result;
    for (const QString &s: data) {
        QString h = s.section("=",0,0);
        QString v = s.section("=",1);
        result.append({h,v});
    }
    return result;
}

SavingFunction::SavingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}


QString SavingFunction::name() const
{DD;
    return "Saver";
}

QString SavingFunction::displayName() const
{DD;
    return "Тип файла";
}

QString SavingFunction::description() const
{DD;
    return "Сохранение файлов";
}

QStringList SavingFunction::properties() const
{DD;
    return QStringList()<<"type"<<"destination";
}

QString SavingFunction::propertyDescription(const QString &property) const
{DD;
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип файла\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"DFD | UFF | D94\","
                                   "  \"values\"      : [\"DFD\", \"UFF\", \"D94\"],"
                                   "  \"minimum\"     : 0,"
                                   "  \"maximum\"     : 0"
                                   "}";
    if (property == "destination") return "{"
                                     "  \"name\"        : \"destination\"   ,"
                                     "  \"type\"        : \"string\"   ,"
                                     "  \"displayName\" : \"Сохранять в\"   ,"
                                     "  \"defaultValue\": \"\"         ,"
                                     "  \"toolTip\"     : \"Папка сохранения\""
                                     "}";
    return "";
}

QVariant SavingFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        // do not know anything about these broadcast properties, propagating
        if (m_input) return m_input->getProperty(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return type;
    if (p == "destination") return destination;
    if (p == "name") return newFileName;

    return QVariant();
}

void SavingFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type") type = val.toInt();
    else if (p == "destination") destination = val.toString();
}

QVector<double> SavingFunction::getData(const QString &id)
{DD;
    Q_UNUSED(id);
    return QVector<double>();
}

bool SavingFunction::compute(FileDescriptor *file)
{DD; //qDebug()<<debugName();
    /* что нужно для сохранения:
     * 1. тип файла
     * 2. папка, куда сохранять файл - конструируется из имени исходного файла
     * 3. номер файла
     * 4. как аккумулировать посчитанные каналы?
     * 5. данные каждого канала
     * 6. тип данных: универсальный или следующий deepsea?
     * 7. дополнительные параметры, подсасываемые из предыдущих функций
     */
    if (!m_input) return false;

    if (!m_file) {
        QString fileName = file->fileName();
        QString method = m_input->getProperty("?/functionDescription").toString();
        newFileName = createUniqueFileName(destination, fileName, method,
                                                getSuffixByType(type), true);

        m_file = createFile(file);
    }

    if (!m_file) return false;

    // создаем канал
    QVector<double> data;
    int i=1;
    while (1) {
        //qDebug()<<"saving iter"<<i;
        bool computed = m_input->compute(file);
        //qDebug()<<"saving iter"<<i<<computed;
        QVector<double> data1 = m_input->getData("input");
        if (data1.isEmpty()) {
            //qDebug()<<"data for saving at iter"<<i<<"is empty";
            break;
        }
        data.append(data1);
        //qDebug()<<"done saving iter"<<i++;
    }
    //qDebug()<<"finished saving at iter"<<i;

    if (data.isEmpty()) return false;

    //определяем число отсчетов в одном блоке
    int dataSize = data.size();
    const int blocksCount = m_input->getProperty("?/zCount").toInt();

    const bool dataIsComplex = m_input->getProperty("?/dataFormat").toString() == "complex";
    if (dataIsComplex) dataSize /= 2;

    //здесь заполняются все свойства канала, не связанные с данными

    Channel *ch = createChannel(file, dataSize / blocksCount, blocksCount);
    if (!ch) return false;

    bool abscissaEven = m_input->getProperty("?/abscissaEven").toBool();
    if (abscissaEven)
        ch->data()->setXValues(0.0, m_input->getProperty("?/xStep").toDouble(), dataSize / blocksCount);
    else {
        const QList<QVariant> abscissaData = m_input->getProperty("?/abscissaData").toList();
        QVector<double> aData;
        for (QVariant v: abscissaData) aData << v.toDouble();
        ch->data()->setXValues(aData);
    }

    // z values

    bool zUniform = m_input->getProperty("?/zAxisUniform").toBool();
    if (!zUniform) {
        const QList<QVariant> zData = m_input->getProperty("?/zData").toList();
        QVector<double> vals;
        for (const QVariant &val: zData)
            vals << val.toDouble();
        ch->data()->setZValues(vals);
    }
    else {
        const double zStep = m_input->getProperty("?/zStep").toDouble();//29 Abscissa increment
        const double zBegin = m_input->getProperty("?/zBegin").toDouble();
        ch->data()->setZValues(zBegin, zStep, blocksCount);
    }


    ch->data()->setThreshold(m_input->getProperty("?/threshold").toDouble());
    ch->data()->setYValuesUnits(DataHolder::YValuesUnits(m_input->getProperty("?/yValuesUnits").toInt()));

    if (dataIsComplex) {
        // мы должны разбить поступившие данные на два массива
        QVector<cx_double> complexData(dataSize);
        for (int i=0; i<dataSize; i++) {
            complexData[i]={data[i*2], data[i*2+1]};
        }
        //данные сразу всех блоков
        ch->data()->setYValues(complexData, -1);
    }
    else {
        QString format = m_input->getProperty("?/dataFormat").toString();
        int f = -1;
        if (format == "complex") f = 0;
        else if (format == "real") f = 1;
        else if (format == "imaginary") f = 2;
        else if (format == "amplitude") f = 3;
        else if (format == "phase") f = 5;
        else if (format == "amplitudeDb") f = 4;
        //данные сразу всех блоков
        ch->data()->setYValues(data, DataHolder::YValuesFormat(f), -1);
    }

    ch->setPopulated(true);

    return true;
}

void SavingFunction::reset()
{DD;
    // вызывается для каждого файла в базе
    //1. настраивает выходное название файла
    //2. подготавливает данные для выходного файла
    //(или в функции сохранения?)

    if (m_file) {
//        if (m_file->channelsCount()>0)
//            m_file->setSamplesCount(m_file->channel(0)->data()->samplesCount());
        m_file->setChanged(true);
        m_file->setDataChanged(true);
        for (int i=0; i<m_file->channelsCount(); ++i) {
            m_file->channel(i)->setChanged(true);
            m_file->channel(i)->setDataChanged(true);
        }
        m_file->write();
        newFiles << m_file->fileName();
        qDebug()<<"added"<<m_file->fileName();
    }
    delete m_file;
    m_file = 0;

    data.clear();
}

FileDescriptor *SavingFunction::createFile(FileDescriptor *file)
{DD;
    switch (type) {
        case DfdFile: return createDfdFile(file);
        case UffFile: return createUffFile(file);
        case D94File: return createD94File(file);
        default: break;
    }

    return 0;
}

FileDescriptor *SavingFunction::createDfdFile(FileDescriptor *file)
{DD;
    int dataType = m_input->getProperty("?/dataType").toInt();

    DfdFileDescriptor *newDfd = DfdFileDescriptor::newFile(newFileName, DfdDataType(dataType));
    newDfd->BlockSize = 0;
    newDfd->setDataDescription(file->dataDescription());

    if (DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file)) {
        //newDfd->DescriptionFormat = dfd->DescriptionFormat;

        // [Sources]
        QString channels = m_input->getProperty("?/channels").toString();
        newDfd->dataDescription().put("source.file", file->fileName());
        newDfd->dataDescription().put("source.guid", file->dataDescription().get("guid"));
        newDfd->dataDescription().put("source.dateTime", file->dataDescription().get("dateTime"));
        newDfd->dataDescription().put("source.channels", channels);
   }

    // [Process]
    auto data = m_input->getProperty("?/processData").toMap();
    if (!data.isEmpty()) {
        for (auto it = data.constBegin(); it != data.constEnd(); ++it)
            newDfd->dataDescription().put("function."+it.key(), it.value());
    }

    // rest
//    newDfd->XName = m_input->getProperty("?/xName").toString();
//    newDfd->XStep = m_input->getProperty("?/xStep").toDouble();
//    newDfd->XBegin = m_input->getProperty("?/xBegin").toDouble();

    return newDfd;
}

FileDescriptor *SavingFunction::createUffFile(FileDescriptor *file)
{DD;
    UffFileDescriptor *newUff = new UffFileDescriptor(newFileName);

    newUff->setDataDescription(file->dataDescription());
    newUff->updateDateTimeGUID();

    QString channels = m_input->getProperty("?/channels").toString();
    newUff->dataDescription().put("source.file", file->fileName());
    newUff->dataDescription().put("source.guid", file->dataDescription().get("guid"));
    newUff->dataDescription().put("source.dateTime", file->dataDescription().get("dateTime"));
    newUff->dataDescription().put("source.channels", channels);

    return newUff;
}

FileDescriptor *SavingFunction::createD94File(FileDescriptor *file)
{
    Data94File *newFile = new Data94File(newFileName);
    newFile->setDataDescription(file->dataDescription());
    newFile->updateDateTimeGUID();

    QString channels = m_input->getProperty("?/channels").toString();
    newFile->dataDescription().put("source.file", file->fileName());
    newFile->dataDescription().put("source.guid", file->dataDescription().get("guid"));
    newFile->dataDescription().put("source.dateTime", file->dataDescription().get("dateTime"));
    newFile->dataDescription().put("source.channels", channels);

    qDebug()<<newFile->dataDescription().toStringList();

    return newFile;
}

Channel *SavingFunction::createChannel(FileDescriptor *file, int dataSize, int blocksCount)
{DD;
    DataDescription functionDescription = m_input->getFunctionDescription();
    switch (type) {
        case DfdFile: return createDfdChannel(file, dataSize, blocksCount, functionDescription);
        case UffFile: return createUffChannel(file, dataSize, blocksCount, functionDescription);
        case D94File: return createD94Channel(file, dataSize, blocksCount, functionDescription);
        default: break;
    }

    return 0;
}

Channel *SavingFunction::createDfdChannel(FileDescriptor *file, int dataSize, int blocksCount, DataDescription &descr)
{DD;
    DfdChannel *ch = 0;

    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor*>(file);

    //Преобразуем для удобства работы
    //подразумеваем, что работаем с файлом dfd
    if (DfdFileDescriptor *newDfd = dynamic_cast<DfdFileDescriptor *>(m_file)) {
        //    ch = p.method->createDfdChannel(newDfd, file, spectrum, p, i);
        ch = new DfdChannel(newDfd, newDfd->channelsCount());
        int i = m_input->getProperty("?/channelIndex").toInt();

        /*ch->ChanName = file->channel(i)->name();
        ch->ChanDscr = file->channel(i)->description();
        if (dfd)
            ch->ChanAddress = dfd->channels[i]->ChanAddress;

        ch->ChanBlockSize = dataSize;
        ch->IndType = 3221225476;

        ch->YName = file->channel(i)->yName();
        if (dfd)
            ch->YNameOld = dfd->channels[i]->YNameOld;
        else
            ch->YNameOld = ch->YName;*/
    }

    return ch;
}

Channel *SavingFunction::createUffChannel(FileDescriptor *file, int dataSize, int blocksCount, DataDescription &descr)
{DD;
    Function *ch = 0;
    if (UffFileDescriptor *newUff = dynamic_cast<UffFileDescriptor *>(m_file)) {
        UffFileDescriptor *uffFile = dynamic_cast<UffFileDescriptor *>(file);

        ch = new Function(newUff);
        const int i = m_input->getProperty("?/channelIndex").toInt();

       /* //Заполнение заголовка функции
        ch->header.type1858[4].value = 1; //4 set record number
        //5 octave format, 0=not in octave format(default), 1=octave, n=1/n oct
        ch->header.type1858[5].value = m_input->getProperty("?/octaveFormat");
        //6 measurement run number
        ch->header.type1858[6].value = 0;
        //11 weighting type, 0=no, 1=A wei, 2=B wei, 3=C wei, 4=D wei
        ch->header.type1858[11].value = weightingType(m_input->getProperty("?/weighting").toString());
        //тип оконной функции  0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
        //                     4=exponential, 5=impact, 6=impact and exponential
        int windowType = m_input->getProperty("?/windowType").toInt();
        ch->header.type1858[12].value = uffWindowType(windowType);
        //13 amplitude units, 0=unknown, 1=half-peak, 2=peak, 3=RMS
        ch->header.type1858[13].value = m_input->getProperty("?/amplitudeScaling");
        //14 normalization method, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                               //3=Units squared seconds per Hz (ESD)
        ch->header.type1858[14].value = m_input->getProperty("?/normalization");
//        {FTInteger6, 0}, //15  Abscissa Data Type Qualifier,
//                          //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
//        {FTInteger6, 0}, //16 Ordinate Numerator Data Type Qualifier, see 15
//        {FTInteger6, 0}, //17 Ordinate Denominator Data Type Qualifier, see 15
//        {FTInteger6, 0}, //18 Z-axis Data Type Qualifier, see 15
//        {FTInteger6, 0}, //19 sampling type, 0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
//        {FTInteger6, 0}, {FTInteger6, 0}, {FTInteger6, 0}, {FTEmpty, ""}, //20-23 not used

//        {FTFloat15_7, 0.0}, //24 Z RPM value
//        {FTFloat15_7, 0.0}, //25 Z time value
//        {FTFloat15_7, 0.0}, //26 Z order value
//        {FTFloat15_7, 0.0}, //27 number of samples
//        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //28-29 not used

//        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, //30-33 user values
//        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //34-35 Exponential window damping factor
//        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTEmpty, ""}, //36-41 not used
//        {FTString10a,"NONE  NONE"}, //42 response direction, reference direction



//        {FTString80,"NONE" }, {FTEmpty,""},  //12-13 ID line 5
        //ch->type58[0].value = //FTDelimiter
        //ch->type58[1].value = //FTEmpty
        //ch->type58[2].value = 58;//
        //ch->type58[3].value = //FTEmpty

        //название канала
        ch->type58[4].value = file->channel(i)->name();
        //ch->type58[5].value = //FTEmpty

        //описание
        ch->type58[6].value = file->channel(i)->description();
        //ch->type58[7].value = //FTEmpty

        //время создания канала
        ch->type58[8].value = QDateTime::currentDateTime();
        //ch->type58[9].value = //FTEmpty

        //ID line 4
        ch->type58[10].value = QString("Record %1").arg(newUff->channelsCount()+1);
        //ch->type58[11].value = //FTEmpty

        //ID line 5 - correction ?
        if (uffFile)
            ch->type58[12].value = uffFile->channels[i]->type58[12].value;
        else
            ch->type58[12].value = "NONE";
        //ch->type58[13].value = //FTEmpty

        //тип функции
        int functionType = m_input->getProperty("?/functionType").toInt();
        ch->type58[14].value = functionType;

        // Нумерация каналов и порций данных для каналов.
        // 1. Каждая порция данных записывается отдельным каналом.
        // 2. Все порции данных имеют одно название канала type58[4]
        // 3. Каждая порция имеет свой номер
        //номер канала, сквозная нумерация по всему файлу
        ch->type58[15].value = newUff->channelsCount()+1;

        //Version Number, or sequence number,  начинается заново для каждого нового канала
        ch->type58[16].value = newUff->channelsCount()+1;
        //Load Case Identification Number
        ch->type58[17].value = 0;

        //RESPONSE
        //точка измерения
        if (uffFile)
            ch->type58[18].value = uffFile->channels[i]->type58[18].value;
        else
            ch->type58[18].value = QString("p%1").arg(newUff->channelsCount()+1);
        //Response Node
        if (uffFile)
            ch->type58[19].value = uffFile->channels[i]->type58[19].value;
        //направление отклика
        if (uffFile)
            ch->type58[20].value = uffFile->channels[i]->type58[20].value;
        else
            ch->type58[20].value = 3; //20 Response Direction +Z

        //REFERENCE
        //Reference Entity Name ("NONE" if unused)
        if (uffFile)
            ch->type58[21].value = uffFile->channels[i]->type58[21].value;
        else
            ch->type58[21].value = "NONE";
        //Reference Node
        if (uffFile)
            ch->type58[22].value = uffFile->channels[i]->type58[22].value;
        //направление возбуждения
        if (uffFile)
            ch->type58[23].value = uffFile->channels[i]->type58[23].value;
        else
            ch->type58[23].value = 3; //20 Reference Direction +Z
        //ch->type58[24].value = //FTEmpty

        ch->type58[25].value = m_input->getProperty("?/dataFormat").toString() == "complex" ? 5 : 2; //25 Ordinate Data Type
        ch->type58[26].value = dataSize; //number of data values for abscissa
        ch->type58[27].value = m_input->getProperty("?/abscissaEven").toBool() ? 1 : 0;//27 Abscissa Spacing (1=even, 0=uneven,
        //28 Abscissa minimum
        QList<QVariant> abscissaData = m_input->getProperty("?/abscissaData").toList();
        if (!abscissaData.isEmpty())
            ch->type58[28].value = abscissaData.constFirst().toDouble();
        else
            ch->type58[28].value = 0.0;
        ch->type58[29].value = m_input->getProperty("?/xStep").toDouble(); //29 Abscissa increment
        if (uffFile)
            ch->type58[30].value = uffFile->channels[i]->type58[30].value; //30 Z-axis value
        else
            ch->type58[30].value = 0.0; //30 Z-axis value
        //ch->type58[31].value = //FTEmpty

        ch->type58[32].value = m_input->getProperty("?/xType").toInt(); // Abscissa Data Characteristics
//        ch->type58[33].value = 0;//33 Length units exponent
//        ch->type58[34].value = 0;//34 Force units exponent
//        ch->type58[35].value = 0;//35 Temperature units exponent
        ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt()); //32 Abscissa type description
        ch->type58[37].value = m_input->getProperty("?/xName").toString(); //37 Abscissa name
        //ch->type58[38].value = //FTEmpty

        ch->type58[39].value = m_input->getProperty("?/yType").toInt(); //39 Ordinate (or ordinate numerator) Data Characteristics // 1 = General
//        ch->type58[40].value = 0;// Length units exponent
//        ch->type58[41].value = 0;// Force units exponent
//        ch->type58[42].value = 0;// Temperature units exponent
        ch->type58[43].value = abscissaTypeDescription(ch->type58[39].value.toInt());
        ch->type58[44].value = m_input->getProperty("?/yName").toString(); //44 Ordinate name
        //ch->type58[45].value = //FTEmpty

        //Заполняем при передаточной для силы
        ch->type58[46].value = 0; //39 Ordinate (or ordinate denominator) Data Characteristics // 1 = General
//        ch->type58[47].value = 0;// Length units exponent
//        ch->type58[48].value = 0;// Force units exponent
//        ch->type58[49].value = 0;// Temperature units exponent
        ch->type58[50].value = "NONE";
        ch->type58[51].value = "NONE"; // Ordinate name
        //ch->type58[52].value = //FTEmpty

        QString zName = m_input->getProperty("?/zName").toString().toLower();
        if (zName == "s" || zName == "с")
            ch->type58[53].value = 17; //39 Z axis Data Characteristics // 17 = Time
        else
            ch->type58[53].value = 1; //39 Z axis Data Characteristics // 1 = General
//        ch->type58[54].value = 0;// Length units exponent
//        ch->type58[55].value = 0;// Force units exponent
//        ch->type58[56].value = 0;// Temperature units exponent
        ch->type58[57].value = abscissaTypeDescription(ch->type58[53].value.toInt());
        ch->type58[58].value = m_input->getProperty("?/zName").toString(); // Applicate name
        //ch->type58[59].value = //FTEmpty
        */
    }
    return ch;
}

Channel *SavingFunction::createD94Channel(FileDescriptor *file, int dataSize, int blocksCount, DataDescription &descr)
{DD;
    Data94Channel *ch = 0;
    if (Data94File *newD94 = dynamic_cast<Data94File *>(m_file)) {
//        Data94File *d94File = dynamic_cast<Data94File *>(file);

        ch = new Data94Channel(newD94);
        const int i = m_input->getProperty("?/channelIndex").toInt();
        Channel *channel = file->channel(i);

        //qDebug()<<ch->dataDescription().data.isEmpty();

        ch->dataDescription() = channel->dataDescription();

        ch->isComplex = (m_input->getProperty("?/dataFormat").toString() == "complex");
//        ch->setName(channel->name());
//        ch->setDescription(channel->description());
        ch->setYName(m_input->getProperty("?/yName").toString());
        ch->dataDescription().put("ynameold", channel->yName());
        ch->setXName(m_input->getProperty("?/xName").toString());
        ch->setZName(m_input->getProperty("?/zName").toString());
        ch->dataDescription().put("samples", QString::number(dataSize));
        ch->dataDescription().put("dateTime", QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm"));

        // x values
        const bool xEven = m_input->getProperty("?/abscissaEven").toBool();
        const QList<QVariant> abscissaData = m_input->getProperty("?/abscissaData").toList();
        const double xStep = m_input->getProperty("?/xStep").toDouble();
        if (!abscissaData.isEmpty())
            ch->xAxisBlock.begin = abscissaData.constFirst().toDouble();
        else
            ch->xAxisBlock.begin = 0.0;
        if (xEven)
            ch->dataDescription().put("samplerate", int(m_input->getProperty("?/sampleRate").toDouble()));
        ch->xAxisBlock.uniform = xEven ? 1:0;
        if (!xEven) {
            QVector<double> vals;
            for (const QVariant &val: abscissaData)
                vals << val.toDouble();
            ch->xAxisBlock.values = vals;
        }
        ch->xAxisBlock.count = dataSize;
        ch->xAxisBlock.step = xStep;

        // z values
        const double zStep = m_input->getProperty("?/zStep").toDouble();
        const double zBegin = m_input->getProperty("?/zBegin").toDouble();
        ch->dataDescription().put("blocks", blocksCount);
        ch->zAxisBlock.step = zStep;
        ch->zAxisBlock.uniform = m_input->getProperty("?/zAxisUniform").toBool() ? 1 : 0;
        ch->zAxisBlock.count = blocksCount;
        const QList<QVariant> zData = m_input->getProperty("?/zData").toList();
        ch->zAxisBlock.begin = zBegin;
        if (ch->zAxisBlock.uniform == 0) {
            QVector<double> vals;
            for (const QVariant &val: zData)
                vals << val.toDouble();
            ch->zAxisBlock.values = vals;
        }
        for (auto i=descr.data.begin(); i!=descr.data.end(); ++i)
            ch->dataDescription().put(i.key(), i.value());
        //qDebug()<<ch->dataDescription().toStringList();

//        ch->dataDescription().put("function.name", m_input->getProperty("?/functionDescription").toString());
//        ch->dataDescription().put("function.type", m_input->getProperty("?/functionType").toInt());
//        ch->dataDescription().put("function.format", m_input->getProperty("?/dataFormat").toString());
//        ch->dataDescription().put("function.logref", m_input->getProperty("?/threshold").toDouble());
//        int units = m_input->getProperty("?/normalization").toInt();
//        switch (units) {
//            case 0: ch->dataDescription().put("function.logscale", "linear"); break;
//            case 1:
//            case 2:
//            case 3: ch->dataDescription().put("function.logscale", "quadratic"); break;
//            default: break;
//        }
//        ch->dataDescription().put("function.octaveFormat", m_input->getProperty("?/octaveFormat").toInt());
//        "responseName": "lop1:1",
//         *               "responseDirection": "+z",
//         *               "responseNode": "",
//         *               "referenceName": "lop1:1",
//         *               "referenceNode": "",
//         *               "referenceDescription": "более полное описание из dfd",
//         *               "referenceDirection": "",
//        *               "precision": int8 / uint8 / int16 / uint16 / int32 / uint32 / int64 / uint64 / float / double
//        *               //далее идут все параметры обработки
//        *               "weighting": no / A / B / C / D
//        *               "averaging": "linear", "no", "exponential", "peak hold", "energetic"
//        *               "averagingCount": 38,
//        *               "window": "Hanning", "Hamming", "square", "triangular", "Gauss", "Natoll",
//        *                         "force", "exponential", "Tukey", "flattop", "Bartlett", "Welch symmetric",
//        *                         "Welch periodic", "Hanning broad", "impact", "Kaiser-Bessel",
//        *               "windowParameter": 0.00,
//        *               "blockSize": 2048,

    }
    return ch;
}
