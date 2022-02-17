#include "gxyfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

GxyFunction::GxyFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString GxyFunction::name() const
{DD;
    return "GXY";
}

QString GxyFunction::description() const
{DD;
    return "Взаимный спектр";
}

QStringList GxyFunction::properties() const
{DD;
    return QStringList();
}

QString GxyFunction::propertyDescription(const QString &property) const
{DD;
    Q_UNUSED(property);
    return QString();
}

QVariant GxyFunction::m_getProperty(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        if (property == "?/dataType") return 131; // отсутствует в DeepSea, APS
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription")
            return "GXY";

        if (property == "?/dataFormat")
            return "complex";

        if (property == "?/yValuesUnits")
            return DataHolder::UnitsQuadratic;

        if (property == "?/yName")
            return QString("(%1)^2").arg(m_input->getParameter("?/yNameOld").toString());

        if (property == "?/referenceChannelIndex" && m_input2)
            return m_input2->getParameter(property);

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }

    return QVariant();
}

DataDescription GxyFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    result.put("function.name", "GXY");

    return result;
}

void GxyFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString GxyFunction::displayName() const
{DD;
    return "Автоспектр";
}

bool GxyFunction::compute(FileDescriptor *file)
{DD;
    reset();

    if (!m_input || !m_input2) return false;

    if (!m_input->compute(file)) {
        qDebug()<<"Gxy can't get data from input1";
        return false;
    }
    if (m_input != m_input2) { //no need to compute the second time, as inputs are the same
        if (!m_input2->compute(file)) {
            qDebug()<<"Gxy can't get data from input2";
            return false;
        }
    }

    //data should contain complex spectrum
    QVector<double> data1 = m_input->getData("input");
    if (data1.isEmpty()) {
        qDebug()<<"Data for Gxy from input1 is empty";
        return false;
    }
    QVector<double> data2 = m_input2->getData("input");
    if (data2.isEmpty()) {
        qDebug()<<"Data for Gxy from input2 is empty";
        return false;
    }
    if (data1.size() != data2.size()) return false;

    int dataSize = data1.size()/2;
    output.resize(data1.size());
    for (int i=0; i<dataSize; ++i) {
        cx_double cd1={data1[i*2], data1[i*2+1]};
        cx_double cd2={data2[i*2], data2[i*2+1]};
        cx_double cd = std::conj(cd1) * cd2;
        output[i*2] = cd.real();
        output[i*2+1] = cd.imag();
    }

    return true;
}

void GxyFunction::reset()
{DD;
    output.clear();
}
