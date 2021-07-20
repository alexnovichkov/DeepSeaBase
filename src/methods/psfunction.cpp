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
        if (property == "?/portionsCount") return portionsCount;

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    return QVariant();
}

void PsFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString PsFunction::displayName() const
{DD;
    return "Спектр мощности";
}

bool PsFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input) return false;
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
    const int blockSize = m_input->getProperty("?/blockSize").toInt();
    portionsCount = data.size()/blockSize;

    for (int block = 0; block < portionsCount; ++block) {
        QVector<cx_double> fft = Fft::compute(data.mid(block*blockSize, blockSize));

        int size = fft.size() * 100 / 256;
        const int Nvl = fft.size();
        const double factor = 2.0 / Nvl/Nvl;
        QVector<double> data1(size);

        for (int i = 0; i < size; i++) {
            data1[i] = factor * std::norm(fft[i]);
        }
        output.append(data1);
    }

    return true;
}

void PsFunction::reset()
{DD;
    output.clear();
    portionsCount = 0;
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
