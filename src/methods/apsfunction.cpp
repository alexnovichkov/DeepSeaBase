#include "apsfunction.h"

#include "fileformats/filedescriptor.h"
#include "fft.h"

ApsFunction::ApsFunction(QObject *parent) :
    AbstractFunction(parent)
{

}

QString ApsFunction::name() const
{
    return "APS";
}

QString ApsFunction::description() const
{
    return "Автоспектр";
}

QStringList ApsFunction::properties() const
{
    return QStringList();
}

QString ApsFunction::propertyDescription(const QString &property) const
{
    return QString();
}

QVariant ApsFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
//        if (property == "?/processData") {
//            int type = map.value("type");
//            QStringList list;

//            switch (type) {
//                case 0: list << "PName=Спектроанализатор"; break;
//                case 1: list << "PName=Спектр мощности"; break;
//                case 2: list << "PName=Плотн.спектра мощности"; break;
//                default: break;
//            }

//            list << QString("BlockIn=%1").arg(m_input->getProperty("?/blockSize").toInt());
//            list << QString("Wind=%1").arg(m_input->getProperty("?/windowDescription").toString());
//            list << QString("TypeAver=%1").arg(m_input->getProperty("?/averaging").toString());
//            list << "pTime=(0000000000000000)";
//            return list;
//        }
        if (property == "?/dataType") {
//            if (typeCombo->currentText()=="спектр СКЗ") return 130;
//            if (typeCombo->currentText()=="мощности") return 128;
//            if (typeCombo->currentText()=="плотности мощн.") return 129;
//            return 128;
            switch (map.value("type")) {
                case 2: return 129; break; // "плотности мощн."
                default: return 128;
            }
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
            if (s.toLower() == "m/s^2")
                return QString("(%1)^2/Hz").arg(s);
            return QString("(%1)^2/Гц").arg(s);
        }

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }

    return QVariant();
}

void ApsFunction::setProperty(const QString &property, const QVariant &val)
{
    Q_UNUSED(property);
    Q_UNUSED(val);
}

QString ApsFunction::displayName() const
{
    return "Автоспектр";
}

QVector<double> ApsFunction::getData(const QString &id)
{
    if (id == "input") return output;
    //if (id == "referenceInput") return refOutput;

    return QVector<double>();
}

bool ApsFunction::compute(FileDescriptor *file)
{
    reset();

    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    //data should contain complex spectrum
    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    double sampleRate = m_input->getProperty("?/sampleRate").toDouble();

    int dataSize = data.size()/2;
    output.resize(dataSize);
    for (int i=0; i<dataSize; ++i) {
        cx_double cd={data[i*2], data[i*2+1]};
        output[i] = std::norm(cd);
    }

    return true;
}

void ApsFunction::reset()
{
    output.clear();
}
