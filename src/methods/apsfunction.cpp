#include "apsfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

ApsFunction::ApsFunction(QObject *parent) :
    AbstractFunction(parent)
{DD;

}

QString ApsFunction::name() const
{DD;
    return "APS";
}

QString ApsFunction::description() const
{DD;
    return "Автоспектр";
}

QStringList ApsFunction::properties() const
{DD;
    return QStringList();
}

QString ApsFunction::propertyDescription(const QString &property) const
{DD;
    Q_UNUSED(property);
    return QString();
}

QVariant ApsFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/processData") {
            QStringList list;

            list << "PName=Автоспектр мощности";
            list << QString("BlockIn=%1").arg(m_input->getProperty("?/blockSize").toInt());
            list << QString("Wind=%1").arg(m_input->getProperty("?/windowDescription").toString());
            list << QString("TypeAver=%1").arg(m_input->getProperty("?/averaging").toString());
            list << "pTime=(0000000000000000)";
            return list;
        }
        if (property == "?/dataType") {
            return 131; // отсутствует в DeepSea, APS
        }
        if (property == "?/xName") return "Гц";
        if (property == "?/xType") return 18; //frequency
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getProperty("?/sampleRate").toDouble() / m_input->getProperty("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") {
            return "APS";
        }
        if (property == "?/functionType") {
            return 2;
        }
        if (property == "?/normalization") {
            return 1; //units squared
        }
        if (property == "?/dataFormat") {
            return "real";
        }
        if (property == "?/yValuesUnits") {
            return DataHolder::UnitsQuadratic;
        }
        if (property == "?/yName") {
            QString s = m_input->getProperty("?/yName").toString();
            return QString("(%1)^2").arg(s);
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }

    return QVariant();
}

void ApsFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString ApsFunction::displayName() const
{DD;
    return "Автоспектр";
}

DataDescription ApsFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    result.put("xname", "Гц");
    QString s = m_input->getProperty("?/yName").toString();
    result.put("yname", QString("(%1)^2").arg(s));
    result.put("function.name", "APS");
    result.put("function.type", 2);
    result.put("function.format", "real");
    result.put("function.logscale", "quadratic");

    return result;
}

bool ApsFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    //data should contain complex spectrum
    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //double sampleRate = m_input->getProperty("?/sampleRate").toDouble();

    int dataSize = data.size()/2;
    output.resize(dataSize);
    for (int i=0; i<dataSize; ++i) {
        cx_double cd={data[i*2], data[i*2+1]};
        output[i] = std::norm(cd);
    }

    return true;
}

void ApsFunction::reset()
{DD;
    output.clear();
}
