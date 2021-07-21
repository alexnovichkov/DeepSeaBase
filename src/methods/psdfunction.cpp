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
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") return "PSD";
        if (property == "?/dataFormat") return "amplitude";
        if (property == "?/yValuesUnits") return DataHolder::UnitsQuadratic;
        if (property == "?/yName") {
            QString s = m_input->getParameter("?/yNameOld").toString();
            if (s.toLower() == "m/s^2")
                return QString("(%1)^2/Hz").arg(s);
            return QString("(%1)^2/Гц").arg(s);
        }
        if (property == "?/portionsCount") return portionsCount;

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }
    return QVariant();
}

void PsdFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString PsdFunction::displayName() const
{DD;
    return "PSD";
}

bool PsdFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input) return false;
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
    const int blockSize = m_input->getParameter("?/blockSize").toInt();
    portionsCount = data.size()/blockSize;

    double sampleRate = m_input->getParameter("?/sampleRate").toDouble();

    for (int block = 0; block < portionsCount; ++block) {
        QVector<cx_double> fft = Fft::compute(data.mid(block*blockSize, blockSize));

        int size = fft.size()*100/256;
        const int Nvl = fft.size();
        const double factor = 2.0 / Nvl / sampleRate;
        QVector<double> data1(size);
        for (int i = 0; i < size; i++) {
            data1[i] = factor * std::norm(fft[i]);
        }
        output.append(data1);
    }

    return true;
}

void PsdFunction::reset()
{DD;
    output.clear();
    portionsCount = 0;
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
