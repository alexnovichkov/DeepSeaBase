#include "savingfunction.h"
#include "filedescriptor.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "logging.h"

//returns "uff" by default
QString getSuffixByType(int type)
{
    switch (type) {
        case SavingFunction::DfdFile: return "dfd";
            break;
        case SavingFunction::UffFile: return "uff";
            break;
    }
    return "uff";
}

DescriptionList processData(const QStringList &data)
{
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
{

}


QString SavingFunction::name() const
{
    return "Saver";
}

QString SavingFunction::displayName() const
{
    return "Тип файла";
}

QString SavingFunction::description() const
{
    return "Сохранение файлов";
}

QStringList SavingFunction::properties() const
{
    return QStringList()<<"type"<<"destination";
}

QString SavingFunction::propertyDescription(const QString &property) const
{
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
{
    return true;
}

QVariant SavingFunction::getProperty(const QString &property) const
{
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
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type") type = val.toInt();
    else if (p == "destination") destination = val.toString();
}

QVector<double> SavingFunction::getData(const QString &id)
{
    return QVector<double>();
}

bool SavingFunction::compute(FileDescriptor *file)
{
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
        QString method = m_input->getProperty("?/method").toString();
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
    ch->data()->setXValues(0.0, m_file->xStep(), dataSize);
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
{
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
{
    FileDescriptor *f = 0;

    if (type == DfdFile) f = createDfdFile(file);
    if (type == UffFile) f = createUffFile(file);

    return f;
}

FileDescriptor *SavingFunction::createDfdFile(FileDescriptor *file)
{
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
{

}

Channel *SavingFunction::createChannel(FileDescriptor *file, int dataSize)
{
    Channel *c = 0;
    if (type == DfdFile) c = createDfdChannel(file, dataSize);
    if (type == UffFile) c = createUffChannel(file, dataSize);

    return c;
}

Channel *SavingFunction::createDfdChannel(FileDescriptor *file, int dataSize)
{
    DfdChannel *ch = 0;
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
{

}
