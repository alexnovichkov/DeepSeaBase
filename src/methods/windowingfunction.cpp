#include "windowingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

WindowingFunction::WindowingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD0;

}


QString WindowingFunction::name() const
{DD;
    return "Windowing";
}

QString WindowingFunction::description() const
{DD0;
    return "Применение оконной функции";
}

QStringList WindowingFunction::parameters() const
{DD0;
    return {"type","parameter","correction"};
}

QString WindowingFunction::m_parameterDescription(const QString &parameter) const
{DD0;
    if (parameter == "type") return "{"
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
    if (parameter == "correction") return "{"
                                   "  \"name\"        : \"correction\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Коррекция\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Коррекция окна\","
                                   "  \"values\"      : [\"амплитудная\",\"энергетическая\",\"без коррекции\"]"
                                   "}";

    if (parameter == "parameter") return "{"
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

QVariant WindowingFunction::m_getParameter(const QString &parameter) const
{DD0;
    if (parameter.startsWith("?/")) {
        if (parameter == "?/functionDescription") return "WIN";
//        if (property == "?/windowDescription") return Windowing::windowDescription(windowing.getWindowType());
//        if (property == "?/windowType") return windowing.getWindowType();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(parameter);
    }
    if (!parameter.startsWith(name()+"/")) return QVariant();
    QString p = parameter.section("/",1);

    if (p == "type") return static_cast<int>(windowing.getWindowType());
    if (p == "correction") return static_cast<int>(windowing.getCorrectionType());
    if (p == "parameter") return windowing.getParameter();


    return QVariant();
}

DataDescription WindowingFunction::getFunctionDescription() const
{DD0;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.name", "WIN");
    result.put("function.window", Windowing::windowDescriptionEng(windowing.getWindowType()));
    if (windowing.windowAcceptsParameter(windowing.getWindowType()))
        result.put("function.windowParameter", windowing.getParameter());
    result.put("function.windowCorrection", Windowing::correctionDescription(windowing.getCorrectionType()));

    return result;
}

void WindowingFunction::m_setParameter(const QString &parameter, const QVariant &val)
{DD0;
    if (!parameter.startsWith(name()+"/")) return;
    QString p = parameter.section("/",1);

    if (p == "type")
        windowing.setWindowType(static_cast<Windowing::WindowType>(val.toInt()));
    else if (p == "correction")
        windowing.setCorrectionType(static_cast<Windowing::CorrectionType>(val.toInt()));
    else if (p == "parameter")
        windowing.setParameter(val.toDouble());
}

bool WindowingFunction::m_parameterShowsFor(const QString &p) const
{DD0;
    if (p == "parameter") return (windowing.windowAcceptsParameter(windowing.getWindowType()));
    return true;
}

QString WindowingFunction::displayName() const
{DD0;
    return "Окно";
}


bool WindowingFunction::compute(FileDescriptor *file)
{DD0;
    reset();

    LOG(INFO) << QString("Запуск расчета для оконной функции");

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
{DD0;
    output.clear();
}

RefWindowingFunction::RefWindowingFunction(QObject *parent, const QString &name)
    : WindowingFunction(parent, name)
{DD0;

}

QString RefWindowingFunction::displayName() const
{DD0;
    return "Опорное окно";
}

