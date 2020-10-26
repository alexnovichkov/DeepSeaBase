#include "windowingfunction.h"

#include "fileformats/filedescriptor.h"

WindowingFunction::WindowingFunction(QObject *parent) :
    AbstractFunction(parent)
{

}


QString WindowingFunction::name() const
{
    return "Windowing";
}

QString WindowingFunction::description() const
{
    return "Применение оконной функции";
}

QStringList WindowingFunction::properties() const
{
    return QStringList()<<"type"<<"parameter"<<"referenceType"<<"referenceParameter";
}

QString WindowingFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Функция\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип оконной функции\","
                                   "  \"values\"      : [\"Прямоугольное\",\"Треугольное\",\"Хеннинга\","
                                   "  \"Хемминга\",\"Наттолла\", \"Гаусса\", \"Сила\", \"Экспоненциальное\", \"Тьюки\"]"
                                   "}";
    if (property == "parameter") return "{"
                                      "  \"name\"        : \"parameter\"   ,"
                                      "  \"type\"        : \"double\"   ,"
                                      "  \"displayName\" : \"%\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"Параметр (%), зависящий от типа окна\","
                                      "  \"values\"      : [],"
                                      "  \"minimum\"     : 0.0,"
                                      "  \"maximum\"     : 100.0"
                                      "}";
    if (property == "referenceType") return "{"
                                   "  \"name\"        : \"referenceType\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Опорное окно\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип оконной функции для опорного канала\","
                                   "  \"values\"      : [\"Прямоугольное\",\"Треугольное\",\"Хеннинга\","
                                   "  \"Хемминга\",\"Наттолла\", \"Гаусса\", \"Сила\", \"Экспоненциальное\", \"Тьюки\"]"
                                   "}";
    if (property == "referenceParameter") return "{"
                                      "  \"name\"        : \"referenceParameter\"   ,"
                                      "  \"type\"        : \"double\"   ,"
                                      "  \"displayName\" : \"%\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"Параметр (%), зависящий от типа окна\","
                                      "  \"values\"      : [],"
                                      "  \"minimum\"     : 0.0,"
                                      "  \"maximum\"     : 100.0"
                                      "}";
    return "";
}

QVariant WindowingFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        if (property == "?/functionDescription") return "WIN";
        if (property == "?/windowDescription") return Windowing::windowDescription(windowing.getWindowType());
        if (property == "?/windowType") return windowing.getWindowType();
        if (property == "?/referenceWindowDescription") return Windowing::windowDescription(refWindowing.getWindowType());
        if (property == "?/referenceWindowType") return refWindowing.getWindowType();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return windowing.getWindowType();
    if (p == "parameter") return windowing.getParameter();
    if (p == "referenceType") return refWindowing.getWindowType();
    if (p == "referenceParameter") return refWindowing.getParameter();

    return QVariant();
}

void WindowingFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type") windowing.setWindowType(val.toInt());
    else if (p == "parameter") windowing.setParameter(val.toDouble());
    else if (p == "referenceType") refWindowing.setWindowType(val.toInt());
    else if (p == "referenceParameter") refWindowing.setParameter(val.toDouble());
}

bool WindowingFunction::propertyShowsFor(const QString &property) const
{
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    bool use = m_input->getProperty("?/useReferenceChannel").toBool();

    if (p == "parameter") return (windowing.windowAcceptsParameter(windowing.getWindowType()));
    if (p == "referenceParameter") return (refWindowing.windowAcceptsParameter(refWindowing.getWindowType())
                                           && use);
    if (p == "referenceType") return use;


    return true;
}


QString WindowingFunction::displayName() const
{
    return "Окно";
}


QVector<double> WindowingFunction::getData(const QString &id)
{
    if (id == "input") return output;
    if (id == "referenceInput") return output;

    return QVector<double>();
}

bool WindowingFunction::compute(FileDescriptor *file)
{
    output.clear();

    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    output = m_input->getData("input");
    if (output.isEmpty()) return false;

    refOutput = m_input->getData("referenceInput");
    refWindowing.applyTo(refOutput);

    return true;
}

void WindowingFunction::reset()
{
    output.clear();
    refOutput.clear();
}
