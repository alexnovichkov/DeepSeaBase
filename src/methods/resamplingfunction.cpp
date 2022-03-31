#include "resamplingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

ResamplingFunction::ResamplingFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;
}


QString ResamplingFunction::name() const
{DD;
    return "Resampling";
}

QString ResamplingFunction::description() const
{DD;
    return "Выбор частотного диапазона";
}

QStringList ResamplingFunction::properties() const
{DD;
    return QStringList()<<"resampleType"<<"factor"<<"frequencyRange"<<"sampleRate";
}

QString ResamplingFunction::propertyDescription(const QString &property) const
{DD;
    if (property == "resampleType") {
        return QString("{"
               "  \"name\"        : \"resampleType\"   ,"
               "  \"type\"        : \"enum\"   ,"
               "  \"displayName\" : \"Способ передискретизации\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"Коэффициент | Диапазон | Частота дискретизации\","
               "  \"values\"      : [\"Коэффициент\", \"Частотный диапазон\", \"Частота дискретизации\"]"
                "}");
    }
    if (property == "factor") {
        return QString("{"
               "  \"name\"        : \"factor\"   ,"
               "  \"type\"        : \"double\"   ,"
               "  \"displayName\" : \"Коэффициент\"   ,"
               "  \"defaultValue\": 1.0         ,"
               "  \"toolTip\"     : \"Частота дискретизации будет уменьшена в k раз\","
               "  \"values\"      : [],"
               "  \"minimum\"     : 0.0"
                "}");
    }
    if (property == "frequencyRange") {
        return QString("{"
               "  \"name\"        : \"frequencyRange\"   ,"
               "  \"type\"        : \"int\"   ,"
               "  \"displayName\" : \"Диапазон\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"Зависит от частоты дискретизации\","
               "  \"values\"      : [],"
               "  \"minimum\"     : 2"
                "}");
    }
    if (property == "sampleRate") {
        return QString("{"
                       "  \"name\"        : \"sampleRate\"   ,"
                       "  \"type\"        : \"int\"   ,"
                       "  \"displayName\" : \"Частота дискретизации\"   ,"
                       "  \"defaultValue\": 0         ,"
                       "  \"toolTip\"     : \"Целое число\","
                       "  \"values\"      : [],"
                       "  \"minimum\"     : 1"
                       "}");
    }

    return "";
}

QVariant ResamplingFunction::m_getProperty(const QString &property) const
{DD;
    double sR = 1.0 / xStep;
    if (property.startsWith("?/")) {
        // recalculate sample rate to new val
        if (property == "?/sampleRate")
            return sR / factor;
        if (property == "?/functionDescription")
            return "RSMPL";
        if (property == "?/dataType") // dfd DataType
            return 1; //CuttedData
        if (property == "?/xStep")
            return xStep * factor;
        if (m_input)
            return m_input->getParameter(property);
    }

    if (property == name()+"/resampleType")
        return currentResamplingType;
    if (property == name()+"/factor")
        return factor;
    if (property == name()+"/frequencyRange")
        return qRound(sR / factor / 2.56);
    if (property == name()+"/sampleRate")
        return qRound(sR / factor);

    return QVariant();
}

DataDescription ResamplingFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    result.put("function.format", "real");
    result.put("function.precision", "float");
    result.put("function.name", "RSMPL");
    result.put("function.type", 12);

    return result;
}

void ResamplingFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "resampleType") {
        currentResamplingType = val.toInt();
    }
    else if (p == "frequencyRange") {
        if (getParameter(name()+"/frequencyRange").toInt() != val.toInt()) {
            double f = 1.0 / val.toDouble() / 2.56 / xStep;
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            emit attributeChanged(this, name()+"/factor",QVariant(),"");
            emit attributeChanged(this, name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(this, name()+"/sampleRate",QVariant(),"");
        }
    }
    else if (p == "sampleRate") {
        int sR = val.toInt();
        if (getParameter(name()+"/sampleRate").toInt() != sR) {
            double f = 1.0 / xStep / sR;
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            emit attributeChanged(this, name()+"/factor",QVariant(),"");
            emit attributeChanged(this, name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(this, name()+"/sampleRate",QVariant(),"");
        }
    }
    else if (p == "factor") {
        double f = val.toDouble();
        if (factor != f) {
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            emit attributeChanged(this, name()+"/factor",QVariant(),"");
            emit attributeChanged(this, name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(this, name()+"/sampleRate",QVariant(),"");
        }
    }
    else if (p == "xStep") {
        xStep = val.toDouble();
    }
}

bool ResamplingFunction::propertyShowsFor(const QString &property) const
{DD;
    if (!property.startsWith(name()+"/")) return false;
    QString p = property.section("/",1);

    if (p == "factor" && currentResamplingType != 0) return false;
    if (p == "frequencyRange" && currentResamplingType != 1) return false;
    if (p == "sampleRate" && currentResamplingType != 2) return false;

    return true;
}


QString ResamplingFunction::displayName() const
{DD;
    return "Передискретизация";
}

void ResamplingFunction::reset()
{DD;
    //resampler.reset();
    output.clear();
    triggerData.clear();
}


bool ResamplingFunction::compute(FileDescriptor *file)
{DD; //qDebug()<<debugName();
    if (!m_input) return false;

    if (!m_input->compute(file))
        return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty())
        return false;

    if (qFuzzyCompare(factor+1.0, 2.0)) {
        output = data;
        triggerData = m_input->getData("triggerInput");
    }
    else {
        int bufferSize = 1024;
        resampler.setBufferSize(bufferSize);
        resampler.setFactor(factor);
        resampler.init();

        int pos = 0;
        while (1) {
            QVector<double> chunk = data.mid(pos, bufferSize);
            if (chunk.size() < bufferSize)
                resampler.setLastChunk();
            QVector<double> filtered = resampler.process(chunk);
            if (filtered.isEmpty()) break;
            output.append(filtered);
            pos += bufferSize;
        }

        //processing trigger data
        auto triggerD = m_input->getData("triggerInput");
        resampler.setBufferSize(bufferSize);
        resampler.setFactor(factor);
        resampler.init();

        pos = 0;
        while (1) {
            QVector<double> chunk = triggerD.mid(pos, bufferSize);
            if (chunk.size() < bufferSize)
                resampler.setLastChunk();
            QVector<double> filtered = resampler.process(chunk);
            if (filtered.isEmpty()) break;
            triggerData.append(filtered);
            pos += bufferSize;
        }
    }

    return true;
}

void ResamplingFunction::updateProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    setParameter(property, val);
}
