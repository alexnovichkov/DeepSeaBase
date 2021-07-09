#include "psfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

PsFunction::PsFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString PsFunction::name() const
{DD;
    return "PS";
}

QString PsFunction::description() const
{DD;
    return "Спектр мощности";
}

QStringList PsFunction::properties() const
{DD;
    return QStringList();
}

QString PsFunction::propertyDescription(const QString &property) const
{DD;
    Q_UNUSED(property);
    return QString();
}

QVariant PsFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 128;
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getProperty("?/sampleRate").toDouble() / m_input->getProperty("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") return "PS";
        if (property == "?/dataFormat") return "amplitude";
        if (property == "?/yValuesUnits") return DataHolder::UnitsQuadratic;
        if (property == "?/yName") {
            QString s = m_input->getProperty("?/yNameOld").toString();
            return QString("(%1)^2").arg(s);
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    return QVariant();
}

void PsFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;

}

QString PsFunction::displayName() const
{DD;
    return "Спектр мощности";
}

bool PsFunction::compute(FileDescriptor *file)
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
    const int Nvl = fft.size();
    const double factor = 2.0 / Nvl/Nvl;
    output.resize(size);
    for (int i = 0; i < size; i++) {
        output[i] = factor * std::norm(fft[i]);
    }
    return true;
}

void PsFunction::reset()
{DD;
    output.clear();
}


DataDescription PsFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    result.put("function.format", "amplitude");
    result.put("function.precision", "float");
    result.put("function.name", "PS");
    result.put("function.type", 12);
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "quadratic");  //units quadratic

    return result;
}
