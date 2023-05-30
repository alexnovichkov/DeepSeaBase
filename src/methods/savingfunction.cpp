#include "savingfunction.h"
//#include "fileformats/filedescriptor.h"
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "fileformats/data94file.h"

#include "fileformats/d94io.h"
#include "fileformats/dfdio.h"
#include "fileformats/uffio.h"

#include "logging.h"

SavingFunction::SavingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;
    updateAvailableTypes();
}


QString SavingFunction::name() const
{DD;
    return "Saver";
}

QString SavingFunction::displayName() const
{DD;
    return "Записывать результат";
}

QString SavingFunction::description() const
{DD;
    return "Сохранение файлов";
}

QStringList SavingFunction::parameters() const
{DD;
    return {"append", "type", "precision", "destination"};
}

QString SavingFunction::m_parameterDescription(const QString &property) const
{DD;
    if (property == "type") {
        QStringList types = availableTypes;
        for (QString &type: types) type = "\""+type+"\"";
        return QString("{"
               "  \"name\"        : \"type\"   ,"
               "  \"type\"        : \"enum\"   ,"
               "  \"displayName\" : \"Тип файла\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"DFD | UFF | D94\","
               "  \"values\"      : [%1],"
               "  \"minimum\"     : 0,"
               "  \"maximum\"     : 0"
               "}").arg(types.join(","));
    }
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
    if (property == "precision") return "{"
                                        "  \"name\"        : \"precision\"   ,"
                                        "  \"type\"        : \"enum\"   ,"
                                        "  \"displayName\" : \"Точность\"   ,"
                                        "  \"defaultValue\": 0         ,"
                                        "  \"toolTip\"     : \"Точность представления отсчетов\","
                                        "  \"values\"      : [\"Одинарная (single)\",\"Двойная (double)\"]"
                                        "}";
    return "";
}

QVariant SavingFunction::m_getParameter(const QString &parameter) const
{DD;
    if (parameter.startsWith("?/")) {
        // do not know anything about these broadcast properties, propagating
        if (m_input) return m_input->getParameter(parameter);
    }

    if (parameter.startsWith(name()+"/")) {
        QString p = parameter.section("/",1);

        if (p == "type") return type;
        if (p == "destination") return destination;
        if (p == "name") return newFileName;
        if (p == "append") return append;
        if (p == "precision") return precision;
    }

    return QVariant();
}

void SavingFunction::m_setParameter(const QString &parameter, const QVariant &val)
{DD;
    if (!parameter.startsWith(name()+"/")) return;
    QString p = parameter.section("/",1);

    if (p == "type") type = val.toInt();
    else if (p == "destination") destination = val.toString();
    else if (p == "append") {
        if (append != val.toBool()) {
            append = val.toBool();
            updateAvailableTypes();
            emit attributeChanged(this, name()+"/type", availableTypes, "enumNames");
        }
    }
    else if (p == "precision") precision = val.toInt();
}

bool SavingFunction::m_parameterShowsFor(const QString &p) const
{DD;
    if (p == "type") return !append;
    if (p == "destination") return !append;
    if (p == "precision") return availableTypes.value(type, "D94") == "D94";

    return true;
}

void SavingFunction::updateAvailableTypes()
{
    QStringList availableTypes = {"DFD", "UFF", "D94"};
    if (m_input) {
        auto val = m_input->getParameter("?/dataFormat").toString();
        auto averagingType = m_input->getParameter("?/averagingType").toInt();
        if (val == "complex" || averagingType == 0 /*no averaging*/ || append)
            availableTypes.removeFirst();
    }
    this->availableTypes = availableTypes;
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
    sourceFileName.clear();

    if (!m_input) return false;
    LOG(INFO) << QString("Запуск расчета для функции сохранения");

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
                                               availableTypes.value(type, "D94").toLower(), true);

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
    description.put("function.precision", precision==0?"float":"double");

    if (availableTypes.value(type, "D94") == "DFD" && blocksCount > 1) {
        emit message("ВНИМАНИЕ! При сохранении сонограммы в файл DFD будет сохранен только первый блок!");
    }

    m_file->addChannelWithData(d, description);

    return true;
}

void SavingFunction::reset()
{DD;
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
{DD;
    FileDescriptor *f = nullptr;
    if (auto t = availableTypes.value(type, "D94"); t == "DFD")
        f = createDfdFile();
    else if (t == "UFF")
        f = createUffFile();
    else if (t == "D94")
        f = createD94File();

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
{DD;
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
{DD;
    return new UffFileDescriptor(newFileName);
}

FileDescriptor *SavingFunction::createD94File()
{DD;
    return new Data94File(newFileName);
}

FileIO *SavingFunction::createFileIO(FileDescriptor *file)
{DD;
    DataDescription d = file->dataDescription();
    QString channels = m_input->getParameter("?/channels").toString();
    d.put("source.file", file->fileName());
    d.put("source.guid", file->dataDescription().get("guid"));
    d.put("source.dateTime", file->dataDescription().get("dateTime"));
    d.put("source.channels", channels);


    FileIO *f = nullptr;
    if (availableTypes.value(type, "D94") == "DFD") {
            f = new DfdIO(d, newFileName, this);
            int dataType = m_input->getParameter("?/dataType").toInt();
            f->setParameter("dataType", dataType);
//        case UffFile: f = new UffIO(d, newFileName, this); break;
//        case D94File: f = new D94IO(d, newFileName, this); break;
    }
    if (f) {
        //f->updateDateTimeGUID();
    }
    return f;
}


void SavingFunction::updateParameter(const QString &parameter, const QVariant &val)
{DD;
    Q_UNUSED(val);
    if (parameter == "?/dataFormat" || parameter == "?/averagingType") {
        updateAvailableTypes();
        emit attributeChanged(this, name()+"/type", availableTypes, "enumNames");
    }
}
