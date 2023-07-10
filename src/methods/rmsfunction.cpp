#include "rmsfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

RmsFunction::RmsFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString RmsFunction::name() const
{DD;
    return "RMS";
}

QString RmsFunction::description() const
{DD;
    return "Спектр СКЗ";
}

QStringList RmsFunction::parameters() const
{DD;
    return QStringList();
}

QString RmsFunction::m_parameterDescription(const QString &property) const
{DD;
    Q_UNUSED(property);
    return QString();
}

QVariant RmsFunction::m_getParameter(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 130;
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") return "RMS";
        if (property == "?/dataFormat") return "amplitude";
        if (property == "?/yValuesUnits") return DataHolder::UnitsLinear;
        if (property == "?/yName") {
            return m_input->getParameter("?/yNameOld").toString();
        }
        if (property == "?/portionsCount") return portionsCount;

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }
    return QVariant();
}

void RmsFunction::m_setParameter(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString RmsFunction::displayName() const
{DD;
    return "Спектр СКЗ";
}

bool RmsFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input) return false;
    LOG(INFO) << QString("Запуск расчета для функции спектра СКЗ");
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
    const int blockSize = m_input->getParameter("?/blockSize").toInt();
    portionsCount = data.size()/blockSize;

    for (int block = 0; block < portionsCount; ++block) {
        QVector<cx_double> fft = Fft::compute(data.mid(block*blockSize, blockSize));

        int size = fft.size() * 100 / 256;
        const int Nvl = fft.size();
        const double factor = 2.0 / Nvl/Nvl;
        QVector<double> data1(size);

        for (int i = 0; i < size; i++) {
            data1[i] = qSqrt(factor * std::norm(fft[i]));
        }
        output.append(data1);
    }

    return true;
}

void RmsFunction::reset()
{DD;
    AbstractFunction::reset();
    portionsCount = 0;
}


DataDescription RmsFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.format", "amplitude");
    result.put("function.name", "RMS");
    result.put("function.type", 12); //Spectrum
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "linear");  //units quadratic

    return result;
}
