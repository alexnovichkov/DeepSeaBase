#include "filteringfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

FilteringFunction::FilteringFunction(QObject *parent) :
    AbstractFunction(parent)
{DD;
    // default values
    map.insert("type", Filtering::NoFiltering);
    map.insert("approximation", Filtering::Butterworth);
    map.insert("order", 8);
    map.insert("frequency", 1000.0);
    map.insert("Q", 1.0);
    map.insert("bandwidth", 1.0);
    map.insert("bandwidthHz", 0.0);
    map.insert("gain", -6.0);
    map.insert("slope", 1.0);
    map.insert("rippleDb", 0.01);
    map.insert("stopDb", 48.0);
    map.insert("rolloff", 0.0);
}


QString FilteringFunction::name() const
{DD;
    return "Filtering";
}

QString FilteringFunction::description() const
{DD;
    return "Фильтрация временных данных";
}

QStringList FilteringFunction::properties() const
{DD;
    return QStringList()<<"type"<<"approximation"<<"order"<<"frequency"<<"Q"<<"bandwidth"<<"bandwidthHz"<<"gain"<<"slope"
                       <<"rippleDb"<<"stopDb"<<"rolloff";
}

QString FilteringFunction::propertyDescription(const QString &property) const
{DD;
//    LOG(DEBUG)<<"property description"<<property;
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип фильтрации\","
                                   "  \"values\"      : [\"Без фильтрации\", \"LowPass\",\"HighPass\",\"BandPass\","
                                   "  \"BandStop\",\"LowShelf\",\"HighShelf\",\"BandShelf\"]" //для enum
                                   "}";
    if (property == "approximation") {
        QStringList values;
        values << "\"Butterworth\""<<"\"ChebyshevI\""<<"\"ChebyshevII\"";
        const int type = map.value("type", Filtering::LowPass).toInt();
        if (type<=Filtering::BandStop && type>=Filtering::LowPass)
            values << "\"Bessel\""<<"\"Elliptic\""<<"\"Legendre\"";
        values << "\"RBJ\"";
        return QString("{"
                       "  \"name\"        : \"approximation\"   ,"
                       "  \"type\"        : \"enum\"   ,"
                       "  \"displayName\" : \"Функция\"   ,"
                       "  \"defaultValue\": 0         ,"
                       "  \"toolTip\"     : \"Тип аппроксимирующей функции\","
                       "  \"values\"      : [%1],"
                       "  \"minimum\"     : -2.4,"
                       "  \"maximum\"     : 30.5"
                       "}").arg(values.join(","));
    }
    if (property == "order") return "{"
                                    "  \"name\"        : \"order\"   ,"
                                    "  \"type\"        : \"int\"   ,"
                                    "  \"displayName\" : \"Порядок\"   ,"
                                    "  \"defaultValue\": 8,"
                                    "  \"toolTip\"     : \"Порядок фильтра (2-25)\","
                                    "  \"values\"      : [],"
                                    "  \"minimum\"     : 2,"
                                    "  \"maximum\"     : 25"
                                    "}";
    if (property == "frequency") return "{"
                                        "  \"name\"        : \"frequency\"   ,"
                                        "  \"type\"        : \"double\"   ,"
                                        "  \"displayName\" : \"Частота [Гц]\"   ,"
                                        "  \"defaultValue\": 0.0,"
                                        "  \"toolTip\"     : \"Частота (среза или центральная)\","
                                        "  \"values\"      : [],"
                                        "  \"minimum\"     : 1.0"
                                        "}";
    if (property == "Q") return "{"
                                "  \"name\"        : \"Q\"   ,"
                                "  \"type\"        : \"double\"   ,"
                                "  \"displayName\" : \"Resonance\"   ,"
                                "  \"defaultValue\": 1.0,"
                                "  \"toolTip\"     : \"Resonance\","
                                "  \"values\"      : [],"
                                "  \"minimum\"     : 0.0"
                                "}";
    if (property == "bandwidth") return "{"
                                        "  \"name\"        : \"bandwidth\"   ,"
                                        "  \"type\"        : \"double\"   ,"
                                        "  \"displayName\" : \"Отн. ширина полосы\"   ,"
                                        "  \"defaultValue\": 1.0,"
                                        "  \"toolTip\"     : \"Относительная ширина полосы\","
                                        "  \"values\"      : [],"
                                        "  \"minimum\"     : 0.0"
                                        "}";
    if (property == "bandwidthHz") return "{"
                                        "  \"name\"        : \"bandwidthHz\"   ,"
                                        "  \"type\"        : \"double\"   ,"
                                        "  \"displayName\" : \"Ширина полосы [Гц]\"   ,"
                                        "  \"defaultValue\": 0.0,"
                                        "  \"toolTip\"     : \"Ширина полосы [Гц]\","
                                        "  \"values\"      : [],"
                                        "  \"minimum\"     : 0.0"
                                        "}";
    if (property == "gain") return "{"
                                   "  \"name\"        : \"gain\"   ,"
                                   "  \"type\"        : \"double\"   ,"
                                   "  \"displayName\" : \"Gain\"   ,"
                                   "  \"defaultValue\": -6.0,"
                                   "  \"toolTip\"     : \"Коэффициент усиления [дБ]\","
                                   "  \"values\"      : []"
                                   "}";
    if (property == "slope") return "{"
                                    "  \"name\"        : \"slope\"   ,"
                                    "  \"type\"        : \"double\"   ,"
                                    "  \"displayName\" : \"Slope\"   ,"
                                    "  \"defaultValue\": 1.0,"
                                    "  \"toolTip\"     : \"Slope [?]\","
                                    "  \"values\"      : []"
                                    "}";
    if (property == "rippleDb") return "{"
                                     "  \"name\"        : \"rippleDb\"   ,"
                                     "  \"type\"        : \"double\"   ,"
                                     "  \"displayName\" : \"Неравномерность\"   ,"
                                     "  \"defaultValue\": 0.01,"
                                     "  \"toolTip\"     : \"Неравномерность затухания\","
                                     "  \"minimum\"     : 0.001,"
                                     "  \"maximum\"     : 12.0"
                                     "}";
    if (property == "stopDb") return "{"
                                     "  \"name\"        : \"stopDb\"   ,"
                                     "  \"type\"        : \"double\"   ,"
                                     "  \"displayName\" : \"Полоса заграждения\"   ,"
                                     "  \"defaultValue\": 48.0,"
                                     "  \"toolTip\"     : \"Полоса заграждения [дБ]\","
                                     "  \"minimum\"     : 3.0,"
                                     "  \"maximum\"     : 60.0"
                                     "}";
    if (property == "rolloff") return "{"
                                    "  \"name\"        : \"rolloff\"   ,"
                                    "  \"type\"        : \"double\"   ,"
                                    "  \"displayName\" : \"Transition width\"   ,"
                                    "  \"defaultValue\": 0.0,"
                                    "  \"toolTip\"     : \"Transition width\","
                                    "  \"minimum\"     : -16.0,"
                                    "  \"maximum\"     : 4.0"
                                    "}";
    return "";
}

QVariant FilteringFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 2;//Фильтр. данные
//        if (property == "?/functionType") return 1;//Time response
        if (property == "?/functionDescription") return "FILT";

        if (m_input) return m_input->getParameter(property);
    }

    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "type") return map.value(p, Filtering::LowPass);
    if (p == "approximation") return map.value(p, Filtering::Butterworth);
    if (p == "order") return map.value(p, 8);
    if (p == "frequency") return map.value(p, 1000.0);
    if (p == "Q") return map.value(p, 1.0);
    if (p == "bandwidth") return map.value(p, 1.0);
    if (p == "bandwidthHz") return map.value(p, 0.0);
    if (p == "gain") return map.value(p, -6.0);
    if (p == "slope") return map.value(p, 1.0);
    if (p == "rippleDb") return map.value(p, 0.01);
    if (p == "stopDb") return map.value(p, 48.0);
    if (p == "rolloff") return map.value(p, 0.0);

    return QVariant();
}

DataDescription FilteringFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.name", "FILT");
    switch (filtering.getType()) {
        case Filtering::LowPass: result.put("function.filterType", "low pass"); break;
        case Filtering::HighPass: result.put("function.filterType", "high pass"); break;
        case Filtering::BandPass: result.put("function.filterType", "band pass"); break;
        case Filtering::BandStop: result.put("function.filterType", "band stop"); break;
        case Filtering::LowShelf: result.put("function.filterType", "low shelf"); break;
        case Filtering::HighShelf: result.put("function.filterType", "high shelf"); break;
        case Filtering::BandShelf: result.put("function.filterType", "band shelf"); break;
    }
    if (filtering.getType() != Filtering::NoFiltering) {
        switch (filtering.getApproximation()) {
            case Filtering::Butterworth: result.put("function.filterApproximation", "Butterworth"); break;
            case Filtering::ChebyshevI: result.put("function.filterApproximation", "Chebyshev I"); break;
            case Filtering::ChebyshevII: result.put("function.filterApproximation", "Chebyshev II"); break;
            case Filtering::Bessel: result.put("function.filterApproximation", "Bessel"); break;
            case Filtering::Elliptic: result.put("function.filterApproximation", "elliptic"); break;
            case Filtering::Legendre: result.put("function.filterApproximation", "Legendre"); break;
            case Filtering::RBJ: result.put("function.filterApproximation", "RBJ"); break;
        }
        result.put("function.filterFrequency", filtering.getParameter(Filtering::idFrequency));
    }



    return result;
}

void FilteringFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    map.insert(p, val);

    if (p == "type") {
        QStringList values;
        values << "Butterworth"<<"ChebyshevI"<<"ChebyshevII";
        if (map.value("type").toInt()<=Filtering::BandStop) values << "Bessel"<<"Elliptic"<<"Legendre";
        values << "RBJ";

        emit attributeChanged(this, "Filtering/approximation",values,"enumNames");
    }
}

bool FilteringFunction::propertyShowsFor(const QString &property) const
{DD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    const int approx = map.value("approximation").toInt();
    const int type = map.value("type").toInt();

    if (type == 0 && p != "type") return false;

    if (p == "order") return approx != Filtering::RBJ;
    else if (p == "Q")
        return (approx == Filtering::RBJ && (type==Filtering::LowPass || type==Filtering::HighPass));
    else if (p == "bandwidthHz") {
        if (approx == Filtering::RBJ) return false;
        else if (type==Filtering::BandPass || type==Filtering::BandStop) return true;
        else if (type==Filtering::BandShelf)
            return (approx == Filtering::Butterworth || approx == Filtering::ChebyshevI || approx == Filtering::ChebyshevII);
        else return false;
    }
    else if (p == "bandwidth")
        return (approx == Filtering::RBJ &&
                (type==Filtering::BandPass || type==Filtering::BandStop || type==Filtering::BandShelf));
    else if (p == "gain")
        return (type==Filtering::LowShelf || type==Filtering::HighShelf || type==Filtering::BandShelf);
    else if (p == "slope")
        return (approx == Filtering::RBJ &&
                (type==Filtering::LowShelf || type==Filtering::HighShelf));
    else if (p == "rippleDb") return (approx == Filtering::ChebyshevI || approx == Filtering::Elliptic);
    else if (p == "stopDb") return (approx == Filtering::ChebyshevII);
    else if (p == "rolloff") return (approx == Filtering::Elliptic);

    return true;
}


QString FilteringFunction::displayName() const
{DD;
    return "Фильтрация";
}

void FilteringFunction::reset()
{DD;
    filtering.reset();
    output.clear();
}

bool FilteringFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    const int type = map.value("type").toInt();
    if (type == 0) {
        output = data;
        return true;
    }

    //create
    filtering.setBlockSize(data.size());
    filtering.setType(type);
    filtering.setApproximation(map.value("approximation").toInt());
    filtering.create();
    //set params

    QVector<double> params {
        m_input->getParameter("?/sampleRate").toDouble(),
                map.value("frequency", 1000.0).toDouble(),
                map.value("Q", 1.0).toDouble(),
                map.value("bandwidth", 1.0).toDouble(),
                map.value("bandwidthHz", 0.0).toDouble(),
                map.value("gain", -6.0).toDouble(),
                map.value("slope", 1.0).toDouble(),
                map.value("order", 8).toDouble(),
                map.value("rippleDb", 0.01).toDouble(),
                map.value("stopDb", 48.0).toDouble(),
                map.value("rolloff", 0.0).toDouble()
    };
    filtering.setParameters(params);
//    filtering.setParameter(Filtering::idOrder,       map.value("order", 8).toDouble());
//    filtering.setParameter(Filtering::idSampleRate,  m_input->getParameter("?/sampleRate").toDouble());
//    filtering.setParameter(Filtering::idFrequency,   map.value("frequency", 1000.0).toDouble());
//    filtering.setParameter(Filtering::idQ,           map.value("Q", 1.0).toDouble());
//    filtering.setParameter(Filtering::idBandwidth,   map.value("bandwidth", 1.0).toDouble());
//    filtering.setParameter(Filtering::idBandwidthHz, map.value("bandwidthHz", 0.0).toDouble());
//    filtering.setParameter(Filtering::idGain,        map.value("gain", -6.0).toDouble());
//    filtering.setParameter(Filtering::idSlope,       map.value("slope", 1.0).toDouble());
//    filtering.setParameter(Filtering::idRippleDb,    map.value("rippleDb", 0.01).toDouble());
//    filtering.setParameter(Filtering::idStopDb,      map.value("stopDb", 48.0).toDouble());
//    filtering.setParameter(Filtering::idRolloff,     map.value("rolloff", 0.0).toDouble());
    //process
    output = filtering.filter(data);
    return true;
}
