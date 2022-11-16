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

QStringList ApsFunction::parameters() const
{DD;
    return QStringList();
}

QString ApsFunction::parameterDescription(const QString &parameter) const
{DD;
    Q_UNUSED(parameter);
    return QString();
}

QVariant ApsFunction::m_getParameter(const QString &parameter) const
{DD;
    if (parameter.startsWith("?/")) {
        if (parameter == "?/processData") {
            QStringList list;

            list << "PName=Автоспектр мощности";
            list << QString("BlockIn=%1").arg(m_input->getParameter("?/blockSize").toInt());
            list << QString("Wind=%1").arg(m_input->getParameter("?/windowDescription").toString());
            list << QString("TypeAver=%1").arg(m_input->getParameter("?/averaging").toString());
            list << "pTime=(0000000000000000)";
            return list;
        }
        if (parameter == "?/dataType") {
            return 131; // отсутствует в DeepSea, APS
        }
        if (parameter == "?/xName") return "Гц";
        if (parameter == "?/xType") return 18; //frequency
        if (parameter == "?/xBegin") return 0.0;
        if (parameter == "?/xStep") {
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
        }
        if (parameter == "?/functionDescription") {
            return "APS";
        }
        if (parameter == "?/functionType") {
            return 2;
        }
        if (parameter == "?/normalization") {
            return 1; //units squared
        }
        if (parameter == "?/dataFormat") {
            return "real";
        }
        if (parameter == "?/yValuesUnits") {
            return DataHolder::UnitsQuadratic;
        }
        if (parameter == "?/yName") {
            QString s = m_input->getParameter("?/yName").toString();
            return QString("(%1)^2").arg(s);
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(parameter);
    }

    return QVariant();
}

void ApsFunction::m_setParameter(const QString &parameter, const QVariant &val)
{DD;
    Q_UNUSED(parameter);
    Q_UNUSED(val);
}

QString ApsFunction::displayName() const
{DD;
    return "Автоспектр";
}

DataDescription ApsFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("xname", "Гц");
    QString s = m_input->getParameter("?/yName").toString();
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
    LOG(INFO) << QString("Запуск расчета для функции автоспектра");

    if (!m_input->compute(file)) return false;

    //data should contain complex spectrum
    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //double sampleRate = m_input->getParameter("?/sampleRate").toDouble();

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
