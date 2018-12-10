#include "filteringfunction.h"

#include "filedescriptor.h"

FilteringFunction::FilteringFunction(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractFunction(dataBase, parent)
{
    //filtering.setBlockSize(dataBase.first());

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
{
    return "Filtering";
}

QString FilteringFunction::description() const
{
    return "Фильтрация временных данных";
}

QStringList FilteringFunction::properties() const
{
    return QStringList()<<"type"<<"approximation"<<"order"<<"frequency"<<"Q"<<"bandwidth"<<"bandwidthHz"<<"gain"<<"slope"
                       <<"rippleDb"<<"stopDb"<<"rolloff";
}

QString FilteringFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип фильтрации\","
                                   "  \"values\"      : [\"Без фильтрации\", \"LowPass\",\"HighPass\",\"BandPass\","
                                   "  \"BandStop\",\"LowShelf\",\"HighShelf\",\"BandShelf\"]," //для enum
                                   "  \"minimum\"     : -2.4," //для int и double
                                   "  \"maximum\"     : 30.5" //для int и double
                                   "}";
    if (property == "approximation") {
        QStringList values;
        values << "\"Butterworth\""<<"\"ChebyshevI\""<<"\"ChebyshevII\"";
        if (map.value("type", Filtering::LowPass).toInt()<=Filtering::BandStop &&
            map.value("type", Filtering::LowPass).toInt()>=Filtering::LowPass)
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

QVariant FilteringFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
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

void FilteringFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    map.insert(p, val);

    if (p == "type") {
        QStringList values;
        values << "Butterworth"<<"ChebyshevI"<<"ChebyshevII";
        if (map.value("type").toInt()<=Filtering::BandStop) values << "Bessel"<<"Elliptic"<<"Legendre";
        values << "RBJ";

        emit updateProperty("Filtering/approximation",values,"enumNames");
    }
}

bool FilteringFunction::propertyShowsFor(const QString &property) const
{
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
{
    return "Фильтрация";
}

QVector<double> FilteringFunction::get(FileDescriptor *file, const QVector<double> &data)
{
    //check for no-op
    if (map.value("type").toInt() == 0) return data;

    //create
    filtering.setBlockSize(data.size());
    filtering.setType(map.value("type").toInt());
    filtering.setApproximation(map.value("approximation").toInt());
    filtering.create();

    //set params
    filtering.setParameter(Filtering::idOrder, map.value("order", 8).toDouble());
    filtering.setParameter(Filtering::idFrequency, map.value("frequency", 1000.0).toDouble());
    filtering.setParameter(Filtering::idQ, map.value("Q", 1.0).toDouble());
    filtering.setParameter(Filtering::idBandwidth, map.value("bandwidth", 1.0).toDouble());
    filtering.setParameter(Filtering::idBandwidthHz, map.value("bandwidthHz", 0.0).toDouble());
    filtering.setParameter(Filtering::idGain, map.value("gain", -6.0).toDouble());
    filtering.setParameter(Filtering::idSlope, map.value("slope", 1.0).toDouble());
    filtering.setParameter(Filtering::idRippleDb, map.value("rippleDb", 0.01).toDouble());
    filtering.setParameter(Filtering::idStopDb, map.value("stopDb", 48.0).toDouble());
    filtering.setParameter(Filtering::idRolloff, map.value("rolloff", 0.0).toDouble());
    //process
    return filtering.filter(data);
}

void FilteringFunction::reset()
{
    filtering.reset();
}

QVector<double> FilteringFunction::getData(const QString &id)
{
    if (id == "input") return output;

    return QVector<double>();
}

bool FilteringFunction::compute()
{
    if (!m_input) return false;

    if (!m_input->compute()) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    if (map.value("type").toInt() == 0) {
        output = data;
        return true;
    }

    //create
    filtering.setBlockSize(data.size());
    filtering.setType(map.value("type").toInt());
    filtering.setApproximation(map.value("approximation").toInt());
    filtering.create();

    //set params
    filtering.setParameter(Filtering::idOrder, map.value("order", 8).toDouble());
    filtering.setParameter(Filtering::idFrequency, map.value("frequency", 1000.0).toDouble());
    filtering.setParameter(Filtering::idQ, map.value("Q", 1.0).toDouble());
    filtering.setParameter(Filtering::idBandwidth, map.value("bandwidth", 1.0).toDouble());
    filtering.setParameter(Filtering::idBandwidthHz, map.value("bandwidthHz", 0.0).toDouble());
    filtering.setParameter(Filtering::idGain, map.value("gain", -6.0).toDouble());
    filtering.setParameter(Filtering::idSlope, map.value("slope", 1.0).toDouble());
    filtering.setParameter(Filtering::idRippleDb, map.value("rippleDb", 0.01).toDouble());
    filtering.setParameter(Filtering::idStopDb, map.value("stopDb", 48.0).toDouble());
    filtering.setParameter(Filtering::idRolloff, map.value("rolloff", 0.0).toDouble());
    //process
    output = filtering.filter(data);
    return true;
}
