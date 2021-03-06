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
    return QStringList()<<"type"<<"output"/*<<"referenceChannel"*/;
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
//    if (property == "referenceChannel") return "{"
//                                    "  \"name\"        : \"referenceChannel\"   ,"
//                                    "  \"type\"        : \"int\"   ,"
//                                    "  \"displayName\" : \"Опорный канал\"   ,"
//                                    "  \"defaultValue\": 1,"
//                                    "  \"toolTip\"     : \"Номер опорного канала (1-n)\","
//                                    "  \"values\"      : [],"
//                                    "  \"minimum\"     : 1,"
//                                    "  \"maximum\"     : 1000"
//                                    "}";
    return QString();
}

QVariant FrfFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        if (property == "?/processData") {
            int type = map.value("type");
            QStringList list;

            switch (type) {
                case 0: list << "PName=Передаточная ф-я H1"; break;
                case 1: list << "PName=Передаточная ф-я H2"; break;
                default: break;
            }
            int referenceChannel = map.value("referenceChannel");
            list << QString("pBaseChan=%1,").arg(referenceChannel);
            list << QString("ProcChansList=%1,%2").arg(1).arg(referenceChannel);
            list << QString("BlockIn=%1").arg(m_input->getProperty("?/blockSize").toInt());
            list << QString("Wind=%1").arg(m_input->getProperty("?/windowDescription").toString());
            list << QString("TypeAver=%1").arg(m_input->getProperty("?/averaging").toString());
            list << "pTime=(0000000000000000)";
            return list;
        }
        if (property == "?/dataType") {
//            if (typeCombo->currentText()=="спектр СКЗ") return 130;
//            if (typeCombo->currentText()=="мощности") return 128;
//            if (typeCombo->currentText()=="плотности мощн.") return 129;
//            return 128;
            switch (map.value("type")) {
                case 2: return 129; break; // "плотности мощн."
                default: return 128;
            }
        }
        if (property == "?/xName") return "Гц";
        if (property == "?/xType") return 18; //frequency
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getProperty("?/sampleRate").toDouble() / m_input->getProperty("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") {
            switch (map.value("type")) {
                case 0: return "FFT"; break;
                case 1: return "PS"; break;
                case 2: return "PSD"; break;
                default: return "Unknown";
            }
        }
        if (property == "?/functionType") {
            switch (map.value("type")) {
                case 0: return 12;//"FFT";
                case 1: return 12;//"PS";
                case 2: return 9;//"PSD";
                default: return 0;//"Unknown";
            }
        }
        if (property == "?/normalization") {
            switch (map.value("type")) {
                case 0: if (map.value("output") == 3) //amplitude
                        return 1; //units quadratic
                    else
                        return 0; //no normalization
                    break;
                case 1: return 1; break; //units squared
                case 2: return 2; break; //Units squared per Hz
                default: return 0;
            }
        }
        if (property == "?/dataFormat") {
            if (map.value("type") != 0) return "amplitude";
            switch (map.value("output")) {
                case 0: return "complex"; break;
                case 1: return "real"; break;
                case 2: return "imaginary"; break;
                case 3: return "amplitude"; break;
                case 4: return "phase"; break;
                default: return "unknown";
            }
        }
        if (property == "?/yValuesUnits") {
            switch (map.value("type")) {
                case 0: if (map.value("output") == 0) //complex
                        return DataHolder::UnitsLinear;
                    else if (map.value("output") == 4) //phase
                        return DataHolder::UnitsDimensionless;
                    else
                        return DataHolder::UnitsQuadratic;
                    break;
                case 1: return DataHolder::UnitsQuadratic; break;
                case 2: return DataHolder::UnitsQuadratic; break;
                default: return DataHolder::UnitsUnknown;
            }
        }
        if (property == "?/yName") {
            QString s = m_input->getProperty("?/yName").toString();
            switch (map.value("type")) {
                case 0: return s;
                case 1: return QString("(%1)^2").arg(s);
                case 2: {
                    if (s.toLower() == "m/s^2")
                        return QString("(%1)^2/Hz").arg(s);
                    return QString("(%1)^2/Гц").arg(s);
                }
                default: return s;
            }
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();

    QString p = property.section("/",1);
    return map.value(p);
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

