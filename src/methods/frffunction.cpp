#include "frffunction.h"
#include "fileformats/filedescriptor.h"
#include "fft.h"

FrfFunction::FrfFunction(QObject *parent) : AbstractFunction(parent)
{

}

QString FrfFunction::name() const
{
    return "FRF";
}

QString FrfFunction::description() const
{
    return "Передаточная";
}

QStringList FrfFunction::properties() const
{
    return QStringList()<<"type"<<"output"<<"referenceChannel";
}

QString FrfFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип передаточной функции\","
                                   "  \"values\"      : [\"H1\",\"H2\"]"
                                   "}";
    if (property == "output") return "{"
                                     "  \"name\"        : \"output\"   ,"
                                     "  \"type\"        : \"enum\"   ,"
                                     "  \"displayName\" : \"Значения\"   ,"
                                     "  \"defaultValue\": 0         ,"
                                     "  \"toolTip\"     : \"Тип результата\","
                                     "  \"values\"      : [\"Комплексные\",\"Действительные\",\"Мнимые\",\"Амплитуды\",\"Фазы\"]"
                                     "}";
    if (property == "referenceChannel") return "{"
                                    "  \"name\"        : \"referenceChannel\"   ,"
                                    "  \"type\"        : \"int\"   ,"
                                    "  \"displayName\" : \"Опорный канал\"   ,"
                                    "  \"defaultValue\": 1,"
                                    "  \"toolTip\"     : \"Номер опорного канала (1-n)\","
                                    "  \"values\"      : [],"
                                    "  \"minimum\"     : 1,"
                                    "  \"maximum\"     : 1000"
                                    "}";
    return QString();
}

QVariant FrfFunction::getProperty(const QString &property) const
{

}

void FrfFunction::setProperty(const QString &property, const QVariant &val)
{

}

QString FrfFunction::displayName() const
{
    return "Передаточная";
}

bool FrfFunction::propertyShowsFor(const QString &property) const
{
    Q_UNUSED(property);
    return true;
}

QVector<double> FrfFunction::getData(const QString &id)
{
    if (id == "input")
        return output;

    return QVector<double>();
}

bool FrfFunction::compute(FileDescriptor *file)
{

}

void FrfFunction::reset()
{
    output.clear();
}

