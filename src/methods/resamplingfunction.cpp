#include "resamplingfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

ResamplingFunction::ResamplingFunction(QObject *parent) :
    AbstractFunction(parent)
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
               "  \"toolTip\"     : \"Множитель, применяемый к частоте дискретизации\","
               "  \"values\"      : []"
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

QVariant ResamplingFunction::getProperty(const QString &property) const
{DD;
    double sR = 1.0 / xStep;
    if (property.startsWith("?/")) {
        if (!m_input) return QVariant();

        // we know about ?/sampleRate
        // recalculate sample rate to new val
        if (property == "?/sampleRate") {
            sR = sR / factor ;
            return sR;
        }
        if (property == "?/xStep") {
            return xStep * factor;
        }
        if (property == "?/functionDescription") {
            return "RSMPL";
        }
        if (property == "?/functionType") {
            return 1; //"Time Response";
        }
        if (property == "?/dataType") {// dfd DataType
            return 1; //CuttedData
        }
        if (property == "?/processData") {
            QStringList list;
            list << "pName=Осциллограф";
            list << "pTime=(0000000000000000)";
            QString chanIndexes = m_input->getProperty("?/channels").toString();
            list << "ProcChansList="+chanIndexes;
            //TODO: размер блока должен быть равен числу отсчетов в канале
            list << "BlockIn=1024";
            list << "TypeProc=0";
            list << "NAver=1";
            list << "Values=измеряемые";
            return list;
        }
        if (property == "?/xName") {
            return "с";
        }
        if (property == "?/xType") {
            return 17; //Time
        }
        if (property == "?/abscissaEven")
            return true;

        return m_input->getProperty(property);
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

void ResamplingFunction::setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "resampleType") {
        currentResamplingType = val.toInt();
    }
    else if (p == "factor") {
        double f = val.toDouble();
        if (factor != f) {
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            //обновляем два других параметра, зависящие от этого
            emit attributeChanged(name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(name()+"/sampleRate",QVariant(),"");
        }
    }
    else if (p == "frequencyRange") {
        if (getProperty(name()+"/frequencyRange").toInt() != val.toInt()) {
            double f = 1.0 / val.toDouble() / 2.56 / xStep;
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            emit attributeChanged(name()+"/factor",QVariant(),"");
            emit attributeChanged(name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(name()+"/sampleRate",QVariant(),"");
        }
    }
    else if (p == "sampleRate") {
        int sR = val.toInt();
        if (getProperty(name()+"/sampleRate").toInt() != sR) {
            double f = 1.0 / xStep / sR;
            factor = f;
            emit propertyChanged("?/xStep", xStep * factor);
            emit attributeChanged(name()+"/factor",QVariant(),"");
            emit attributeChanged(name()+"/frequencyRange",QVariant(),"");
            emit attributeChanged(name()+"/sampleRate",QVariant(),"");
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
}


QVector<double> ResamplingFunction::getData(const QString &id)
{DD;
    if (id == "input") return output;

    return QVector<double>();
}

bool ResamplingFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;

    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    if (qFuzzyCompare(factor, 1.0)) output = data;
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
    }
    return true;
}


void ResamplingFunction::updateProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    setProperty(property, val);
}
