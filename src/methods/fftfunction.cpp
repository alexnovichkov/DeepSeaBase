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

QStringList FftFunction::parameters() const
{DD;
    return {"output"};
}

QString FftFunction::m_parameterDescription(const QString &property) const
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

QVariant FftFunction::m_getParameter(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 128;
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep")
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
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
        if (property == "?/portionsCount") return portionsCount;
        if (property == "?/yValuesUnits") {
            if (map.value("output") == 4) //phase
                return DataHolder::UnitsDimensionless;
            return DataHolder::UnitsLinear;
        }
        if (property == "?/yName") return m_input->getParameter("?/yNameOld").toString();

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();

    QString p = property.section("/",1);
    return map.value(p);
}

void FftFunction::m_setParameter(const QString &property, const QVariant &val)
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
    LOG(INFO) << QString("Запуск расчета преобразования Фурье");
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
    const int blockSize = m_input->getParameter("?/blockSize").toInt();
    portionsCount = data.size()/blockSize;

    for (int block = 0; block < portionsCount; ++block) {
        QVector<cx_double> fft = Fft::compute(data.mid(block*blockSize, blockSize));
        if (fft.isEmpty()) return false;

        int size = fft.size()*100/256;
        const int Nvl = fft.size();
        const double factor = sqrt(2.0) / Nvl;

        QVector<double> data1;

        switch (map.value("output")) {
            case 0: {//complex
                data1.resize(size*2);
                for (int i=0; i<size; ++i) {
                    data1[i*2] = fft[i].real() * factor;
                    data1[i*2+1] = fft[i].imag() * factor;
                }

                break;
            }
            case 1: {//real
                data1.resize(size);
                for (int i=0; i<size; ++i) {
                    data1[i] = fft[i].real() * factor;
                }
                break;
            }
            case 2: {//imag
                data1.resize(size);
                for (int i=0; i<size; ++i) {
                    data1[i] = fft[i].imag() * factor;
                }
                break;
            }
            case 3: {//ampl
                data1.resize(size);
                for (int i=0; i<size; ++i) {
                    data1[i] = std::abs(fft[i]) * factor;
                }
                break;
            }
            case 4: //phase
                data1.resize(size);
                for (int i=0; i<size; ++i) {
                    data1[i] = std::arg(fft[i])*180.0/M_PI;
                }
        }
        output.append(data1);
    }

    return true;
}

void FftFunction::reset()
{DD;
    AbstractFunction::reset();
    portionsCount = 0;
}


DataDescription FftFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    switch (map.value("output")) {
        case 0: result.put("function.format", "complex"); break;
        case 1: result.put("function.format", "real"); break;
        case 2: result.put("function.format", "imaginary"); break;
        case 3: result.put("function.format", "amplitude"); break;
        case 4: result.put("function.format", "phase"); break;
        default: break;
    }
    result.put("function.name", "FFT");
    result.put("function.type", 12);
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "linear");

    return result;
}
