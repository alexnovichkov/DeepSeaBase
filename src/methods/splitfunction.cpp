#include "splitfunction.h"

SplitFunction::SplitFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;
}

QString SplitFunction::name() const
{DD;
    return "Splitting";
}

QString SplitFunction::description() const
{DD;
    return "Разбиение на блоки";
}

QStringList SplitFunction::parameters() const
{DD;
    return {"blockSize", "sampleRate"};
}

QString SplitFunction::m_parameterDescription(const QString &property) const
{DD;
    if (property == "blockSize") {
        return QString("{"
               "  \"name\"        : \"blockSize\"   ,"
               "  \"type\"        : \"int\"   ,"
               "  \"displayName\" : \"Примерный размер блока\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"Размер блока может меняться\","
               "  \"values\"      : [],"
               "  \"minimum\"     : 1"
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

QVariant SplitFunction::m_getParameter(const QString &property) const
{DD;
    if (property.startsWith("?/")) {
        // recalculate sample rate to new val
        if (property == "?/sampleRate")
            return sampleRate;
        if (property == "?/functionDescription")
            return "SPLIT";
        if (property == "?/functionType")
            return 9; //PSD
        if (property == "?/dataType") // dfd DataType
            return 128; //спектр мощности
        if (property == "?/xStep")
            return 1;
        if (property == "?/zCount")
            return blocksCount;
        if (property == "?/zStep")
            return 1;
        if (property == "?/zName")
            return "s";
        if (property == "?/xName")
            return "Hz";
        if (property == "?/yName")
            return "m/s^2";
        if (m_input)
            return m_input->getParameter(property);
    }

    if (property == name()+"/blockSize")
        return blockSize;
    if (property == name()+"/sampleRate")
        return sampleRate;

    return QVariant();
}

DataDescription SplitFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.format", "real");
    result.put("function.name", "SPLIT");
    result.put("function.type", 9);

    return result;
}

void SplitFunction::m_setParameter(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "blockSize") {
        blockSize = val.toInt();
    }
    else if (p == "sampleRate") {
        int sR = val.toInt();
        if (getParameter(name()+"/sampleRate").toInt() != sR) {
            double xStep = 1.0 / sR;
            emit parameterChanged("?/xStep", xStep);
            emit attributeChanged(this, name()+"/sampleRate",QVariant(),"");
            sampleRate = sR;
        }
    }
}

bool SplitFunction::m_parameterShowsFor(const QString &p) const
{DD;
    Q_UNUSED(p);

    return true;
}


QString SplitFunction::displayName() const
{DD;
    return "Примерный размер блока";
}

bool SplitFunction::compute(FileDescriptor *file)
{DD;
    if (!m_input) return false;
    LOG(INFO) << QString("Запуск расчета для разбиения на блоки");

    if (!m_input->compute(file))
        return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty())
        return false;

    if (blockSize * sampleRate == 1) {
        output = data;
        triggerData = m_input->getData("triggerInput");
    }
    else {
        output.clear();

        QVector<int> peaks;
        //1. Определяем координаты максимумов вблизи границ блоков
        //через каждые blockSize отсчетов в окрестности [-blockSize/2+i*blockSize .. blockSize/2+i*blockSize]

        for (int i=0; blockSize*(i+1) < data.size(); i++) {
            int begin = i*blockSize - blockSize/2; if (begin<0) begin=0;
            int end = i*blockSize + blockSize/2;

            auto max = std::max_element(data.begin()+begin, data.begin()+end);
            peaks << max-data.begin();
        }

        //2. Определяем оптимальный размер блока и количество блоков
        int optimalBlockSize = peaks.at(1)-peaks.at(0);
        for (int i=2; i<peaks.size(); ++i) {
            if (auto x = peaks.at(i)-peaks.at(i-1); x < optimalBlockSize)
                optimalBlockSize = x;
        }

        if (optimalBlockSize < blockSize /2) {
            LOG(ERROR) << "Слишком маленький размер блока";
            return false;
        }
        blockSize = optimalBlockSize;
        blocksCount = peaks.size();

        //3. Составляем output
        for (int peak: peaks) output.append(data.mid(peak, optimalBlockSize));

        if (output.isEmpty()) {
            //LOG(ERROR) << "Resampling error:"<<resampler.error();
        }
    }

    return true;
}
