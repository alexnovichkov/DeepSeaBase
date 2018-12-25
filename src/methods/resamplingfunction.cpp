#include "resamplingfunction.h"

#include "filedescriptor.h"

ResamplingFunction::ResamplingFunction(QObject *parent) :
    AbstractFunction(parent), exponent(0)
{

}


QString ResamplingFunction::name() const
{
    return "Resampling";
}

QString ResamplingFunction::description() const
{
    return "Выбор частотного диапазона";
}

QStringList ResamplingFunction::properties() const
{
    return QStringList()<<"factor";
}

QString ResamplingFunction::propertyDescription(const QString &property) const
{
    if (property == "factor") {
        int sampleRate = int (1.0/xStep);
        QStringList values;
        for (int i=0; i<10; ++i) {
            double p = pow(2.0, i);
            int bS = qRound(sampleRate / p / 2.56);
            values << QString("\"%2 Гц\"").arg(bS);
        }
        return QString("{"
               "  \"name\"        : \"factor\"   ,"
               "  \"type\"        : \"enum\"   ,"
               "  \"displayName\" : \"Диапазон\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"Зависит от частоты дискретизации\","
               "  \"values\"      : [%1]"
                "}").arg(values.join(","));
    }

    return "";
}

QVariant ResamplingFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        if (!m_input) return QVariant();

        // we know about ?/sampleRate
        if (property == "?/sampleRate") {
            double sR = 1.0 / xStep;
            sR = sR / pow(2.0, exponent);
            return sR;
        }
        if (property == "?/xStep") {
            return xStep * pow(2.0, exponent);
        }
        return m_input->getProperty(property);
    }

    if (property == name()+"/factor") return exponent;

    return QVariant();
}

void ResamplingFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "factor") {
        exponent = val.toInt();
        emit propertyChanged("?/xStep", xStep*pow(2.0, exponent));
    }
    else if (p == "xStep") xStep = val.toDouble();
}


QString ResamplingFunction::displayName() const
{
    return "Част. диапазон";
}

void ResamplingFunction::reset()
{
    resampler.reset();
    output.clear();
}


QVector<double> ResamplingFunction::getData(const QString &id)
{
    if (id == "input") return output;

    return QVector<double>();
}

bool ResamplingFunction::compute(FileDescriptor *file)
{
    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    if (exponent == 0) output = data;
    else {
        resampler.setBufferSize(data.size());
        resampler.setFactor(int(pow(2.0, exponent)));
        resampler.init();
        output = resampler.process(data);
    }
    return true;
}
