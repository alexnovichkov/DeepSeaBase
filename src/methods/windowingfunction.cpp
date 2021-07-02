#include "windowingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

WindowingFunction::WindowingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}


QString WindowingFunction::name() const
{DD;
    return "Windowing";
}

QString WindowingFunction::description() const
{DD;
    return "Применение оконной функции";
}

QStringList WindowingFunction::properties() const
{DD;
    return QStringList()<<"type"<<"parameter";
}

QString WindowingFunction::propertyDescription(const QString &property) const
{DD;
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
    return "";
}

QVariant WindowingFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/functionDescription") return "WIN";
        if (property == "?/windowDescription") return Windowing::windowDescription(windowing.getWindowType());
        if (property == "?/windowType") return windowing.getWindowType();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return windowing.getWindowType();
    if (p == "parameter") return windowing.getParameter();

    return QVariant();
}

void WindowingFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "type") windowing.setWindowType(val.toInt());
    else if (p == "parameter") windowing.setParameter(val.toDouble());
}

bool WindowingFunction::propertyShowsFor(const QString &property) const
{DD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "parameter") return (windowing.windowAcceptsParameter(windowing.getWindowType()));

    return true;
}


QString WindowingFunction::displayName() const
{DD;
    return "Окно";
}


QVector<double> WindowingFunction::getData(const QString &id)
{DD;
    if (id == "input") return output;

    return QVector<double>();
}

bool WindowingFunction::compute(FileDescriptor *file)
{DD; //qDebug()<<debugName();
    reset();

    if (!m_input) return false;

    if (!m_input->compute(file)) {
        //qDebug()<<"Windowing can't get data";
        return false;
    }

    output = m_input->getData("input");
    if (output.isEmpty()) {
        //qDebug()<<"Data for windowing is empty";
        return false;
    }

    windowing.applyTo(output);

    return true;
}

void WindowingFunction::reset()
{DD;
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

