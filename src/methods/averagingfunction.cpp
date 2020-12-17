#include "averagingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

AveragingFunction::AveragingFunction(QObject *parent) :
    AbstractFunction(parent)
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
        if (property == "?/averaging")
            return Averaging::averagingDescription(averaging.getAveragingType());
        if (property == "?/averagingType")
            return averaging.getAveragingType();
        if (property == "?/zCount") {
            //усреднения нет - возвращаем полное число блоков
            if (averaging.getAveragingType() == 0) return m_input->getProperty("?/zCount");
            //усреднение есть - возвращаем один блок
            return 1;
        }

        if (m_input) return m_input->getProperty(property);
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
    if (id == "input") return averaging.get();

    return QVector<double>();
}

bool AveragingFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;

    while (1) {
        m_input->compute(file);

        QVector<double> data = m_input->getData("input");

        if (!data.isEmpty()) {
            averaging.average(data);
        }
        else break;

        if (averaging.averagingDone()) return true;
    }
    return true;
}

void AveragingFunction::reset()
{DD;
    averaging.reset();
}
