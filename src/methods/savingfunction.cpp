#include "savingfunction.h"
//#include "fileformats/filedescriptor.h"
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "fileformats/data94file.h"
#include "logging.h"

//returns "d94" by default
QString getSuffixByType(int type)
{DDD;
    switch (type) {
        case SavingFunction::DfdFile: return "dfd";
        case SavingFunction::UffFile: return "uff";
        case SavingFunction::D94File: return "d94";
    }
    return "d94";
}

SavingFunction::SavingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DDD;

}


QString SavingFunction::name() const
{DDD;
    return "Saver";
}

QString SavingFunction::displayName() const
{DDD;
    return "Записывать результат";
}

QString SavingFunction::description() const
{DDD;
    return "Сохранение файлов";
}

QStringList SavingFunction::properties() const
{DDD;
    return {"append", "type", "destination"};
}

QString SavingFunction::propertyDescription(const QString &property) const
{DDD;
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
                                     "  \"type\"        : \"url\"   ,"
                                     "  \"displayName\" : \"Сохранять в\"   ,"
                                     "  \"defaultValue\": \"\"         ,"
                                     "  \"toolTip\"     : \"Папка сохранения\""
                                     "}";
    if (property == "append") return "{"
                                   "  \"name\"        : \"append\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Записывать результат\"   ,"
                                   "  \"defaultValue\": 0,"
                                   "  \"toolTip\"     : \"в новый файл | в текущий файл\","
                                   "  \"values\"      : [\"в новый файл\", \"в текущий файл\"],"
                                   "  \"minimum\"     : 0,"
                                   "  \"maximum\"     : 0"
                                   "}";
    return "";
}

QVariant SavingFunction::m_getProperty(const QString &property) const
{DDD;
    if (property.startsWith("?/")) {
        // do not know anything about these broadcast properties, propagating
        if (m_input) return m_input->getParameter(property);
    }

    if (property.startsWith(name()+"/")) {
        QString p = property.section("/",1);

        if (p == "type") return type;
        if (p == "destination") return destination;
        if (p == "name") return newFileName;
        if (p == "append") return append;
    }

    return QVariant();
}

void SavingFunction::m_setProperty(const QString &property, const QVariant &val)
{DDD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type") type = val.toInt();
    else if (p == "destination") destination = val.toString();
    else if (p == "append") append = val.toBool();
}

bool SavingFunction::propertyShowsFor(const QString &property) const
{DDD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "type") return !append;
    if (p == "destination") return !append;

    return true;
}

bool SavingFunction::compute(FileDescriptor *file)
{DDD;
    /* что нужно для сохранения:
     * 1. тип файла
     * 2. папка, куда сохранять файл - конструируется из имени исходного файла
     * 3. номер файла
     * 4. как аккумулировать посчитанные каналы?
     * 5. данные каждого канала
     * 6. тип данных: универсальный или следующий deepsea?
     * 7. дополнительные параметры, подсасываемые из предыдущих функций
     */
    sourceFileName.clear();

    if (!m_input) return false;

    if (!m_file) {
        //we cannot write channels of different type info DFD
        if (append) {
            if (file->canTakeAnyChannels()) {
                m_file = file;
            }
            else {
                emit message("Исходный файл не поддерживает сохранение каналов разных типов в один файл");
                return false;
            }
        }
        else {
            QString fileName = file->fileName();
            QString method = m_input->getParameter("?/functionDescription").toString();
            newFileName = createUniqueFileName(destination, fileName, method,
                                               getSuffixByType(type), true);

            m_file = createFile(file);
        }
    }

    if (!m_file) return false;

    // создаем канал
    if (!m_input->compute(file)) return false;
    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    sourceFileName = file->fileName();

    //определяем число отсчетов в одном блоке
    int dataSize = data.size();
    const int blocksCount = m_input->getParameter("?/zCount").toInt();

    const QString dataFormatString = m_input->getParameter("?/dataFormat").toString();
    const bool dataIsComplex = dataFormatString == "complex";
    if (dataIsComplex) dataSize /= 2;

    //здесь заполняются данные канала

    DataHolder *d = new DataHolder();

    // x values
    bool abscissaEven = m_input->getParameter("?/abscissaEven").toBool();
    if (abscissaEven)
        d->setXValues(m_input->getParameter("?/xBegin").toDouble(), m_input->getParameter("?/xStep").toDouble(), dataSize / blocksCount);
    else {
        const QList<QVariant> abscissaData = m_input->getParameter("?/abscissaData").toList();
        QVector<double> aData;
        for (const QVariant &v: abscissaData) aData << v.toDouble();
        d->setXValues(aData);
    }

    // z values

    bool zUniform = m_input->getParameter("?/zAxisUniform").toBool();
    if (!zUniform) {
        const QList<QVariant> zData = m_input->getParameter("?/zData").toList();
        QVector<double> vals;
        for (const QVariant &val: zData) vals << val.toDouble();
        d->setZValues(vals);
    }
    else {
        const double zStep = m_input->getParameter("?/zStep").toDouble();
        const double zBegin = m_input->getParameter("?/zBegin").toDouble();
        d->setZValues(zBegin, zStep, blocksCount);
    }

    // y values

    d->setThreshold(m_input->getParameter("?/logref").toDouble());
    d->setYValuesUnits(DataHolder::YValuesUnits(m_input->getParameter("?/yValuesUnits").toInt()));

    if (dataIsComplex) {
        // мы должны разбить поступившие данные на два массива
        QVector<cx_double> complexData(dataSize);
        for (int i=0; i<dataSize; i++)
            complexData[i]={data[i*2], data[i*2+1]};
        d->setYValues(complexData, -1);
    }
    else {
        d->setYValues(data, DataHolder::formatFromString(dataFormatString), -1);
    }

    //Далее - описание канала
    const int i = m_input->getParameter("?/channelIndex").toInt();
    DataDescription description = file->channel(i)->dataDescription();
    description.put("yname", m_input->getParameter("?/yName").toString());
    description.put("xname", m_input->getParameter("?/xName").toString());

    //при отсутствии метки оси Z пишем 'с' по умолчанию
    auto zname = m_input->getParameter("?/zName").toString();
    description.put("zname", zname.isEmpty()?"с":zname);

    description.put("samples", QString::number(dataSize/blocksCount));
    description.put("dateTime", QDateTime::currentDateTime());
    description.put("samplerate", int(m_input->getParameter("?/sampleRate").toDouble()));
    description.put("blocks", blocksCount);
    description.put("ynameold", file->channel(i)->yName());

    DataDescription functionDescription = m_input->getFunctionDescription();
    for (const auto [key, val]: asKeyValueRange(functionDescription.data))
        description.put(key, val);

    m_file->addChannelWithData(d, description);

    return true;
}

void SavingFunction::reset()
{DDD;
    // вызывается для каждого файла в базе
    //1. настраивает выходное название файла
    //2. подготавливает данные для выходного файла
    //(или в функции сохранения?)

    if (m_file) {
        m_file->setChanged(true);
        m_file->setDataChanged(true);

        m_file->write();
        newFiles << m_file->fileName();
    }
    //do not delete file if we added channels to the source file
    if (m_file && m_file->fileName() != sourceFileName) delete m_file;

    m_file = nullptr;
    sourceFileName.clear();

    data.clear();
}

FileDescriptor *SavingFunction::createFile(FileDescriptor *file)
{DDD;
    FileDescriptor *f = nullptr;
    switch (type) {
        case DfdFile: f = createDfdFile(); break;
        case UffFile: f = createUffFile(); break;
        case D94File: f = createD94File(); break;
        default: break;
    }
    if (f) {
        f->setDataDescription(file->dataDescription());
        f->updateDateTimeGUID();

        QString channels = m_input->getParameter("?/channels").toString();
        f->dataDescription().put("source.file", file->fileName());
        f->dataDescription().put("source.guid", file->dataDescription().get("guid"));
        f->dataDescription().put("source.dateTime", file->dataDescription().get("dateTime"));
        f->dataDescription().put("source.channels", channels);
    }
    return f;
}

FileDescriptor *SavingFunction::createDfdFile()
{DDD;
    int dataType = m_input->getParameter("?/dataType").toInt();

    DfdFileDescriptor *newDfd = DfdFileDescriptor::newFile(newFileName, DfdDataType(dataType));
    newDfd->BlockSize = 0;

    // [Process]
//    auto data = m_input->getParameter("?/processData").toMap();
//    if (!data.isEmpty()) {
//        for (auto it = data.constBegin(); it != data.constEnd(); ++it)
//            newDfd->dataDescription().put("function."+it.key(), it.value());
//    }

    return newDfd;
}

FileDescriptor *SavingFunction::createUffFile()
{DDD;
    return new UffFileDescriptor(newFileName);
}

FileDescriptor *SavingFunction::createD94File()
{
    return new Data94File(newFileName);
}
