#include "savingfunction.h"
#include "filedescriptor.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "logging.h"

//returns "uff" by default
QString getSuffixByType(int type)
{DD;
    switch (type) {
        case SavingFunction::DfdFile: return "dfd";
            break;
        case SavingFunction::UffFile: return "uff";
            break;
    }
    return "uff";
}

DescriptionList processData(const QStringList &data)
{DD;
    DescriptionList result;
    foreach (const QString &s, data) {
        QString h = s.section("=",0,0);
        QString v = s.section("=",1);
        result.append({h,v});
    }
    return result;
}

SavingFunction::SavingFunction(QObject *parent) :
    AbstractFunction(parent), m_file(0)
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
                                   "  \"toolTip\"     : \"DFD | UFF\","
                                   "  \"values\"      : [\"DFD\", \"UFF\"],"
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

bool SavingFunction::propertyShowsFor(const QString &property) const
{DD;
    Q_UNUSED(property);
    return true;
}

QVariant SavingFunction::getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return type;
    if (p == "destination") return destination;
    if (p == "name") return newFileName;

    return QVariant();
}

void SavingFunction::setProperty(const QString &property, const QVariant &val)
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
{DD;
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
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;
    int dataSize = data.size();
    bool dataIsComplex = getProperty("?/dataComplex").toBool();
    if (dataIsComplex) dataSize /= 2;

    Channel *ch = createChannel(file, dataSize);
    if (!ch) return false;

    const int channelIndex = getProperty("?/channelIndex").toInt();
    bool abscissaEven = getProperty("?/abscissaEven").toBool();
    if (abscissaEven)
        ch->data()->setXValues(0.0, m_file->xStep(), dataSize);
    else {
        QList<QVariant> abscissaData = getProperty("?/abscissaData").toList();
        QVector<double> aData;
        foreach (QVariant v,abscissaData) aData << v.toDouble();
        ch->data()->setXValues(aData);
    }
    double thr = threshold(file->channel(channelIndex)->yName());

    ch->data()->setThreshold(thr);
    ch->data()->setYValuesUnits(getProperty("?/yValuesUnits").toInt());

    if (dataIsComplex) {
        // мы должны разбить поступившие данные на два массива
        QVector<cx_double> complexData(dataSize);
        for (int i=0; i<dataSize; i++) {
            complexData[i]={data[i*2], data[i*2+1]};
        }
        ch->data()->setYValues(complexData);
    }
    else ch->data()->setYValues(data, DataHolder::YValuesFormat(getProperty("?/yValuesFormat").toInt()));

    ch->setPopulated(true);
    ch->setName(file->channel(channelIndex)->name());

    return true;
}

void SavingFunction::reset()
{DD;
    // вызывается для каждого файла в базе
    //1. настраивает выходное название файла
    //2. подготавливает данные для выходного файла
    //(или в функции сохранения?)

    if (m_file) {
        if (m_file->channelsCount()>0)
            m_file->setSamplesCount(m_file->channel(0)->samplesCount());
        m_file->setChanged(true);
        m_file->setDataChanged(true);
        m_file->write();
        m_file->writeRawFile();
        newFiles << m_file->fileName();
        qDebug()<<"added"<<m_file->fileName();
    }
    delete m_file;
    m_file = 0;

    data.clear();
}

FileDescriptor *SavingFunction::createFile(FileDescriptor *file)
{DD;
    FileDescriptor *f = 0;

    if (type == DfdFile) f = createDfdFile(file);
    if (type == UffFile) f = createUffFile(file);

    return f;
}

FileDescriptor *SavingFunction::createDfdFile(FileDescriptor *file)
{DD;
    //DfdFileDescriptor *newDfd = AbstractMethod::createNewDfdFile(fileName, dfd, p);

    int dataType = getProperty("?/dataType").toInt();

    DfdFileDescriptor *newDfd = new DfdFileDescriptor(newFileName);

    newDfd->rawFileName = newFileName.left(newFileName.length()-4)+".raw";
    newDfd->updateDateTimeGUID();
    newDfd->BlockSize = 0;
    newDfd->DataType = DfdDataType(dataType);

    if (DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file)) {
        // [DataDescription]
        if (!dfd->dataDescriptor().isEmpty()) {
            newDfd->dataDescription = new DataDescription(newDfd);
            newDfd->dataDescription->data = dfd->dataDescriptor();
        }
        QMap<QString, QString> info = dfd->info();
        newDfd->DescriptionFormat = info.value("descriptionFormat");

        // [Sources]
        newDfd->source = new Source();
        QStringList l; for (int i=1; i<=file->channelsCount(); ++i) l << QString::number(i);
        newDfd->source->sFile = file->fileName()+"["+l.join(",")+"]"+info.value("guid");
    }

    // [Process]
    QStringList data = getProperty("?/processData").toStringList();
    if (!data.isEmpty()) {
        newDfd->process = new Process();
        newDfd->process->data = processData(data);
    }

    // rest
    newDfd->XName = getProperty("?/xName").toString();
    newDfd->XStep = getProperty("?/xDelta").toDouble();
    newDfd->XBegin = getProperty("?/xBegin").toDouble();

    return newDfd;
}

FileDescriptor *SavingFunction::createUffFile(FileDescriptor *file)
{DD;
    UffFileDescriptor *newUff = new UffFileDescriptor(newFileName);

    newUff->updateDateTimeGUID();

    if (!file->dataDescriptor().isEmpty()) {
        newUff->setDataDescriptor(file->dataDescriptor());
    }
    newUff->setLegend(file->legend());

    return newUff;
}

Channel *SavingFunction::createChannel(FileDescriptor *file, int dataSize)
{DD;
    Channel *c = 0;
    if (type == DfdFile) c = createDfdChannel(file, dataSize);
    if (type == UffFile) c = createUffChannel(file, dataSize);

    return c;
}

Channel *SavingFunction::createDfdChannel(FileDescriptor *file, int dataSize)
{DD;
    DfdChannel *ch = 0;

    //Преобразуем для удобства работы
    //подразумеваем, что работаем с файлом dfd
    if (DfdFileDescriptor *newDfd = dynamic_cast<DfdFileDescriptor *>(m_file)) {
        //    ch = p.method->createDfdChannel(newDfd, file, spectrum, p, i);
        ch = new DfdChannel(newDfd, newDfd->channelsCount());
        int i = getProperty("?/channelIndex").toInt();

        ch->setName(file->channel(i)->name());
        ch->ChanDscr = file->channel(i)->description();
        //skip this
//        ch->ChanAddress = file->channel(i)->ChanAddress;

        ch->ChanBlockSize = dataSize;
        ch->IndType = 3221225476;

        ch->YName = /*getProperty("/newYName").toString();*/file->channel(i)->yName();
        ch->YNameOld = file->channel(i)->yName();

        newDfd->channels << ch;
    }

    return ch;
}

Channel *SavingFunction::createUffChannel(FileDescriptor *file, int dataSize)
{DD;
    Function *ch = 0;
    if (UffFileDescriptor *newUff = dynamic_cast<UffFileDescriptor *>(m_file)) {
        UffFileDescriptor *uffFile = dynamic_cast<UffFileDescriptor *>(file);

        ch = new Function(newUff);
        int i = getProperty("?/channelIndex").toInt();

        //тип оконной функции
        int windowType = getProperty("?/windowType").toInt();
        ch->header.type1858[12].value = uffWindowType(windowType);

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

        //ID line 5
        if (uffFile)
            ch->type58[12].value = uffFile->channels[i]->type58[12].value;
        else
            ch->type58[12].value = "NONE";
        //ch->type58[13].value = //FTEmpty

        //тип функции
        int functionType = getProperty("?/functionType").toInt();
        ch->type58[14].value = functionType;

        //номер канала
        ch->type58[15].value = newUff->channelsCount()+1;

        //Version Number, or sequence number
        //ch->type58[16].value =
        //Load Case Identification Number
        //ch->type58[17].value =

        //RESPONSE
        //точка измерения
        if (uffFile)
            ch->type58[18].value = uffFile->channels[i]->type58[18].value;
        else
            ch->type58[18].value = QString("p%1").arg(newUff->channelsCount()+1);
        //Response Node
        //ch->type58[19].value = 0;
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
        //ch->type58[22].value = 0;
        //направление возбуждения
        if (uffFile)
            ch->type58[23].value = uffFile->channels[i]->type58[23].value;
        else
            ch->type58[23].value = 3; //20 Reference Direction +Z
        //ch->type58[24].value = //FTEmpty

        ch->type58[25].value = getProperty("?/dataComplex").toBool() ? 5 : 2; //25 Ordinate Data Type
        ch->type58[26].value = dataSize; //number of data values for abscissa
        ch->type58[27].value = getProperty("?/abscissaEven").toBool() ? 1 : 0;//27 Abscissa Spacing (1=even, 0=uneven,
        //28 Abscissa minimum
        QList<QVariant> abscissaData = getProperty("?/abscissaData").toList();
        if (!abscissaData.isEmpty())
            ch->type58[28].value = abscissaData.first().toDouble();
        else
            ch->type58[28].value = 0.0;
        ch->type58[29].value = getProperty("?/xStep").toDouble(); //29 Abscissa increment
        if (uffFile)
            ch->type58[30].value = uffFile->channels[i]->type58[30].value; //30 Z-axis value
        else
            ch->type58[30].value = 0.0; //30 Z-axis value
        //ch->type58[31].value = //FTEmpty

        ch->type58[32].value = getProperty("?/xType").toInt(); // Abscissa Data Characteristics
//        ch->type58[33].value = 0;//33 Length units exponent
//        ch->type58[34].value = 0;//34 Force units exponent
//        ch->type58[35].value = 0;//35 Temperature units exponent
        ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt()); //32 Abscissa type description
        ch->type58[37].value = getProperty("?/xName").toString(); //37 Abscissa name
        //ch->type58[38].value = //FTEmpty

        ch->type58[39].value = getProperty("?/yType").toInt(); //39 Ordinate (or ordinate numerator) Data Characteristics // 1 = General
//        ch->type58[40].value = 0;// Length units exponent
//        ch->type58[41].value = 0;// Force units exponent
//        ch->type58[42].value = 0;// Temperature units exponent
        ch->type58[43].value = abscissaTypeDescription(ch->type58[39].value.toInt());
        ch->type58[44].value = getProperty("?/yName").toString(); //44 Ordinate name
        //ch->type58[45].value = //FTEmpty

        ch->type58[46].value = 0; //39 Ordinate (or ordinate denominator) Data Characteristics // 1 = General
//        ch->type58[47].value = 0;// Length units exponent
//        ch->type58[48].value = 0;// Force units exponent
//        ch->type58[49].value = 0;// Temperature units exponent
        ch->type58[50].value = "NONE";
        ch->type58[51].value = "NONE"; // Ordinate name
        //ch->type58[52].value = //FTEmpty

        ch->type58[53].value = 0; //39 Z axis Data Characteristics // 1 = General
//        ch->type58[54].value = 0;// Length units exponent
//        ch->type58[55].value = 0;// Force units exponent
//        ch->type58[56].value = 0;// Temperature units exponent
        ch->type58[57].value = "NONE";
        ch->type58[58].value = "NONE"; // Ordinate name
        //ch->type58[59].value = //FTEmpty

        newUff->channels << ch;
    }
    return ch;
}
