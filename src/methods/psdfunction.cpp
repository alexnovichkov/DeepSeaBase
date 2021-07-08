#include "psdfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

PsdFunction::PsdFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString PsdFunction::name() const
{DD;
    return "PSD";
}

QString PsdFunction::description() const
{DD;
    return "Плотность спектра мощности";
}

QStringList PsdFunction::properties() const
{DD;
    return QStringList();
}

QString PsdFunction::propertyDescription(const QString &property) const
{DD;
    return QString();
}

QVariant PsdFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 129;
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getProperty("?/sampleRate").toDouble() / m_input->getProperty("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") return "PSD";
        if (property == "?/dataFormat") return "amplitude";
        if (property == "?/yValuesUnits") return DataHolder::UnitsQuadratic;
        if (property == "?/yName") {
            QString s = m_input->getProperty("?/yNameOld").toString();
            if (s.toLower() == "m/s^2")
                return QString("(%1)^2/Hz").arg(s);
            return QString("(%1)^2/Гц").arg(s);
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    return QVariant();
}

void PsdFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;

}

QString PsdFunction::displayName() const
{DD;
    return "PSD";
}

bool PsdFunction::compute(FileDescriptor *file)
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

    double sampleRate = m_input->getProperty("?/sampleRate").toDouble();

    QVector<cx_double> fft = Fft::compute(data);

    int size = int(fft.size()/2.56);
    const int Nvl = fft.size();
    const double factor = 2.0 / Nvl / sampleRate;
    output.resize(size);
    for (int i = 0; i < size; i++) {
        output[i] = factor * std::norm(fft[i]);
    }

    return true;
}

void PsdFunction::reset()
{DD;
    output.clear();
}


DataDescription PsdFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    result.put("function.format", "amplitude");
    result.put("function.precision", "float");
    result.put("function.name", "PSD");
    result.put("function.type", 9);
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "quadratic");  //units quadratic

    return result;
}
