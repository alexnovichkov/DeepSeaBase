#include "averagingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

AveragingFunction::AveragingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}


QString AveragingFunction::name() const
{DD;
    return "Averaging";
}

QString AveragingFunction::description() const
{DD;
    return "Усреднение";
}

QStringList AveragingFunction::properties() const
{DD;
    return QStringList()<<"type"<<"maximum";
}

QString AveragingFunction::propertyDescription(const QString &property) const
{DD;
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип усреднения\","
                                   "  \"values\"      : [\"Без усреднения\",\"Линейное\",\"Экспоненциальное\","
                                   "  \"Хранение максимума\",\"Энергетическое\"]" //для enum
                                   "}";
    if (property == "maximum") return "{"
                                      "  \"name\"        : \"maximum\"   ,"
                                      "  \"type\"        : \"int\"   ,"
                                      "  \"displayName\" : \"Число усреднений\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"Число усреднений\","
                                      "  \"values\"      : [],"
                                      "  \"minimum\"     : 0"
                                      "}";
    return "";
}

QVariant AveragingFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
//        if (property == "?/averaging")
//            return Averaging::averagingDescription(averaging.getAveragingType());
//        if (property == "?/averagingType")
//            return averaging.getAveragingType();
        if (property == "?/zCount") {
            //усреднения нет - возвращаем полное число блоков
            if (averaging.getAveragingType() == 0) return m_input->getParameter("?/zCount");
            //усреднение есть - возвращаем один блок
            return 1;
        }

        if (m_input) return m_input->getParameter(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return averaging.getAveragingType();
    if (p == "maximum") return averaging.getMaximumAverages();

    return QVariant();
}

void AveragingFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);
    int valInt = val.toInt();

    if (p == "type") {
        averaging.setAveragingType(valInt);
    }
    else if (p == "maximum") {
        averaging.setMaximumAverages(valInt);
    }
}

bool AveragingFunction::propertyShowsFor(const QString &property) const
{DD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "maximum") return (averaging.getAveragingType() != Averaging::NoAveraging);

    return true;
}


QString AveragingFunction::displayName() const
{DD;
    return "Усреднение";
}

QVector<double> AveragingFunction::getData(const QString &id)
{DD;
    const auto format = m_input->getParameter("?/dataFormat").toString();
    if (id == "input") {
        if (format=="complex") return interweavedFromComplexes<double>(averaging.getComplex());
        return averaging.get();
    }

    return QVector<double>();
}

bool AveragingFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;

    m_input->compute(file);
    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
    const int portionsCount = m_input->getParameter("?/portionsCount").toInt();
    const auto format = m_input->getParameter("?/dataFormat").toString();
    const int blockSize = data.size() / portionsCount;

    for (int block = 0; block < portionsCount; ++block) {
        if (format=="complex")
            averaging.average(complexesFromInterweaved(data.mid(block*blockSize, blockSize)));
        else
            averaging.average(data.mid(block*blockSize, blockSize));
        if (averaging.averagingDone()) break;
    }

    return true;
}

void AveragingFunction::reset()
{DD;
    averaging.reset();
}

DataDescription AveragingFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    if (averaging.getAveragingType() != Averaging::NoAveraging) {
        result.put("function.averaging", Averaging::averagingDescriptionEng(averaging.getAveragingType()));
        result.put("function.averagingCount", averaging.getAveragesMade());
    }

    return result;
}
