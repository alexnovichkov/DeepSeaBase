#include "fftfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"

FftFunction::FftFunction(QObject *parent) :
    AbstractFunction(parent)
{

}

QString FftFunction::name() const
{
    return "Spectrum";
}

QString FftFunction::description() const
{
    return "Спектр";
}

QStringList FftFunction::properties() const
{
    return QStringList()<<"type"<<"output";
}

QString FftFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип спектральной характеристики\","
                                   "  \"values\"      : [\"FFT\",\"Power spectrum\",\"Power spectrum density\"]"
                                   "}";
    if (property == "output") return "{"
                                     "  \"name\"        : \"output\"   ,"
                                     "  \"type\"        : \"enum\"   ,"
                                     "  \"displayName\" : \"Значения\"   ,"
                                     "  \"defaultValue\": 0         ,"
                                     "  \"toolTip\"     : \"Тип результата\","
                                     "  \"values\"      : [\"Комплексные\",\"Действительные\",\"Мнимые\",\"Амплитуды\",\"Фазы\"]"
                                     "}";
    return QString();
}

QVariant FftFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        if (property == "?/processData") {
            int type = map.value("type");
            QStringList list;

            switch (type) {
                case 0: list << "PName=Спектроанализатор"; break;
                case 1: list << "PName=Спектр мощности"; break;
                case 2: list << "PName=Плотн.спектра мощности"; break;
                default: break;
            }

            list << QString("BlockIn=%1").arg(getProperty("?/blockSize").toInt());
            list << QString("Wind=%1").arg(getProperty("?/windowDescription").toString());
            list << QString("TypeAver=%1").arg(getProperty("?/averaging").toString());
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
            return getProperty("?/sampleRate").toDouble() / getProperty("?/blockSize").toDouble();
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
            QString s = getProperty("?/yName").toString();
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

void FftFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;

    QString p = property.section("/",1);
    map.insert(p, val.toInt());
}

QString FftFunction::displayName() const
{
    return "Спектр";
}

bool FftFunction::propertyShowsFor(const QString &property) const
{
    if (!property.startsWith(name()+"/")) return false;

    QString p = property.section("/",1);
    if (p == "output") return (map.value("type") == 0);

    return true;
}

QVector<double> FftFunction::getData(const QString &id)
{
    if (id == "input")
        return output;

    return QVector<double>();
}

bool FftFunction::compute(FileDescriptor *file)
{
    output.clear();

    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    double sampleRate = m_input->getProperty("?/sampleRate").toDouble();

    QVector<cx_double> fft = Fft::compute(data);

    int size = int(fft.size()/2.56);
    switch (map.value("type")) {
        case 0: //FFT
            switch (map.value("output")) {
                case 0: {//complex
                    output.resize(size*2);
                    for (int i=0; i<size; ++i) {
                        output[i*2] = fft[i].real();
                        output[i*2+1] = fft[i].imag();
                    }

                    break;
                }
                case 1: {//real
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = fft[i].real();
                    }
                    break;
                }
                case 2: {//imag
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = fft[i].imag();
                    }
                    break;
                }
                case 3: {//ampl
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = std::abs(fft[i]);
                    }
                    break;
                }
                case 4: //phase
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = std::arg(fft[i])*180.0/M_PI;
                    }
            }
            break;
        case 1: {//Спектр мощности
            const int Nvl = fft.size();
            const double factor = 2.0 / Nvl/Nvl;
            output.resize(size);
            for (int i = 0; i < size; i++) {
                output[i] = factor * std::norm(fft[i]);
            }
            break;
        }
        case 2: {//Спектральная плотность мощности
            const int Nvl = fft.size();
            const double factor = 2.0 / Nvl / sampleRate;
            output.resize(size);
            for (int i = 0; i < size; i++) {
                output[i] = factor * std::norm(fft[i]);
            }
            break;
        }
        default:
            break;
    }
    return true;
}

void FftFunction::reset()
{
    output.clear();
}
