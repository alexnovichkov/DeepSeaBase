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
//        if (property == "?/processData") {
//            QStringList list;

//            list << "PName=Автоспектр мощности";
//            list << QString("BlockIn=%1").arg(m_input->getProperty("?/blockSize").toInt());
//            list << QString("Wind=%1").arg(m_input->getProperty("?/windowDescription").toString());
//            list << QString("TypeAver=%1").arg(m_input->getProperty("?/averaging").toString());
//            list << "pTime=(0000000000000000)";
//            return list;
//        }
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
            return "complex";
        }
        if (property == "?/yValuesUnits") {
            return DataHolder::UnitsQuadratic;
        }
        if (property == "?/yName") {
            QString s = m_input->getProperty("?/yName").toString();
            return QString("(%1)^2").arg(s);
        }

        if (property == "?/referenceChannelIndex" && m_input2)
            return m_input2->getProperty(property);

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }

    return QVariant();
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

QVector<double> GxyFunction::getData(const QString &id)
{DD;
    if (id == "input") return output;

    return QVector<double>();
}

bool GxyFunction::compute(FileDescriptor *file)
{DD; //qDebug()<<debugName();
    reset();

    if (!m_input || !m_input2) return false;

    if (!m_input->compute(file)) {
        //qDebug()<<"Gxy can't get data from input1";
        return false;
    }
    if (m_input != m_input2) { //no need to compute the second time, as inputs are the same
        if (!m_input2->compute(file)) {
            //qDebug()<<"Gxy can't get data from input2";
            return false;
        }
    }

    //data should contain complex spectrum
    QVector<double> data1 = m_input->getData("input");
    if (data1.isEmpty()) {
        //qDebug()<<"Data for Gxy from input1 is empty";
        return false;
    }
    QVector<double> data2 = m_input2->getData("input");
    if (data2.isEmpty()) {
        //qDebug()<<"Data for Gxy from input2 is empty";
        return false;
    }
    if (data1.size() != data2.size()) return false;

    int dataSize = data1.size()/2;
    output.resize(dataSize * 2);
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
