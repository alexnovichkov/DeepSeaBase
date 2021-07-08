#include "fftfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

FftFunction::FftFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString FftFunction::name() const
{DD;
    return "Spectrum";
}

QString FftFunction::description() const
{DD;
    return "Спектр";
}

QStringList FftFunction::properties() const
{DD;
    return QStringList()<<"output";
}

QString FftFunction::propertyDescription(const QString &property) const
{DD;
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

QVariant FftFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 128;
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep")
            return m_input->getProperty("?/sampleRate").toDouble() / m_input->getProperty("?/blockSize").toDouble();
        if (property == "?/functionDescription") return "FFT";
        if (property == "?/dataFormat") {
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
            if (map.value("output") == 4) //phase
                return DataHolder::UnitsDimensionless;
            return DataHolder::UnitsLinear;
        }
        if (property == "?/yName") return m_input->getProperty("?/yNameOld").toString();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();

    QString p = property.section("/",1);
    return map.value(p);
}

void FftFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;

    QString p = property.section("/",1);
    map.insert(p, val.toInt());
}

QString FftFunction::displayName() const
{DD;
    return "Значения";
}

bool FftFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input) return false;

    if (!m_input->compute(file)) {
        return false;
    }

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) {
        return false;
    }

    QVector<cx_double> fft = Fft::compute(data);

    int size = int(fft.size()/2.56);
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


    return true;
}

void FftFunction::reset()
{DD;
    output.clear();
}


DataDescription FftFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    switch (map.value("output")) {
        case 0: result.put("function.format", "complex"); break;
        case 1: result.put("function.format", "real"); break;
        case 2: result.put("function.format", "imaginary"); break;
        case 3: result.put("function.format", "amplitude"); break;
        case 4: result.put("function.format", "phase"); break;
        default: break;
    }
    result.put("function.precision", "float");
    result.put("function.name", "FFT");
    result.put("function.type", 12);
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "linear");

    return result;
}
