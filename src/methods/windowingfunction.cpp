#include "windowingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

WindowingFunction::WindowingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DDD;

}


QString WindowingFunction::name() const
{DDD;
    return "Windowing";
}

QString WindowingFunction::description() const
{DDD;
    return "Применение оконной функции";
}

QStringList WindowingFunction::properties() const
{DDD;
    return {"type","parameter","correction"};
}

QString WindowingFunction::propertyDescription(const QString &property) const
{DDD;
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Функция\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип оконной функции\","
                                   "  \"values\"      : [\"Прямоугольное\",\"Треугольное\",\"Хеннинга\","
                                   "  \"Хемминга\",\"Наттолла\", \"Гаусса\", \"Сила\", \"Экспоненциальное\", \"Тьюки\","
                                   "  \"Бартлетта\", \"Блэкмана\", \"Блэкмана-Натолла\", \"Блэкмана-Харриса\","
                                   "  \"Flat top\", \"Уэлча\"]"
                                   "}";
    if (property == "correction") return "{"
                                   "  \"name\"        : \"correction\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Коррекция\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Коррекция окна\","
                                   "  \"values\"      : [\"амплитудная\",\"энергетическая\",\"без коррекции\"]"
                                   "}";

    if (property == "parameter") return "{"
                                      "  \"name\"        : \"parameter\"   ,"
                                      "  \"type\"        : \"double\"   ,"
                                      "  \"displayName\" : \"%\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"Параметр, зависящий от типа окна\","
                                      "  \"values\"      : [],"
                                      "  \"minimum\"     : 0.0,"
                                      "  \"maximum\"     : 100.0"
                                      "}";
    return "";
}

QVariant WindowingFunction::m_getProperty(const QString &property) const
{DDD;
    if (property.startsWith("?/")) {
        if (property == "?/functionDescription") return "WIN";
//        if (property == "?/windowDescription") return Windowing::windowDescription(windowing.getWindowType());
//        if (property == "?/windowType") return windowing.getWindowType();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return static_cast<int>(windowing.getWindowType());
    if (p == "correction") return static_cast<int>(windowing.getCorrectionType());
    if (p == "parameter") return windowing.getParameter();


    return QVariant();
}

DataDescription WindowingFunction::getFunctionDescription() const
{
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.name", "WIN");
    result.put("function.window", Windowing::windowDescriptionEng(windowing.getWindowType()));
    if (windowing.windowAcceptsParameter(windowing.getWindowType()))
        result.put("function.windowParameter", windowing.getParameter());
    result.put("function.windowCorrection", Windowing::correctionDescription(windowing.getCorrectionType()));

    return result;
}

void WindowingFunction::m_setProperty(const QString &property, const QVariant &val)
{DDD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type")
        windowing.setWindowType(static_cast<Windowing::WindowType>(val.toInt()));
    else if (p == "correction")
        windowing.setCorrectionType(static_cast<Windowing::CorrectionType>(val.toInt()));
    else if (p == "parameter")
        windowing.setParameter(val.toDouble());
}

bool WindowingFunction::propertyShowsFor(const QString &property) const
{DDD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "parameter") return (windowing.windowAcceptsParameter(windowing.getWindowType()));

    return true;
}

QString WindowingFunction::displayName() const
{DDD;
    return "Окно";
}


bool WindowingFunction::compute(FileDescriptor *file)
{DDD;
    reset();

    if (!m_input) return false;
    if (!m_input->compute(file)) return false;

    output = m_input->getData("input");
    if (output.isEmpty()) return false;

    const int bufferSize = m_input->getParameter("?/blockSize").toInt();
    windowing.setBufferSize(bufferSize);
    windowing.applyTo(output, bufferSize);
    return true;
}

void WindowingFunction::reset()
{DDD;
    output.clear();
}

RefWindowingFunction::RefWindowingFunction(QObject *parent, const QString &name)
    : WindowingFunction(parent, name)
{

}

QString RefWindowingFunction::displayName() const
{
    return "Опорное окно";
}

