#include "frffunction.h"
#include "fileformats/filedescriptor.h"
#include "fft.h"
#include "logging.h"

FrfFunction::FrfFunction(QObject *parent, const QString &name) : AbstractFunction(parent, name)
{DDD;

}

QString FrfFunction::name() const
{DDD;
    return "FRF";
}

QString FrfFunction::description() const
{DDD;
    return "Передаточная";
}

QStringList FrfFunction::properties() const
{DDD;
    return QStringList()<<"type"<<"output";
}

QString FrfFunction::propertyDescription(const QString &property) const
{DDD;
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип передаточной функции\","
                                   "  \"values\"      : [\"H1\",\"H2\"]"
                                   "}";
    if (property == "output") return "{"
                                     "  \"name\"        : \"output\"   ,"
                                     "  \"type\"        : \"enum\"   ,"
                                     "  \"displayName\" : \"Значения\"   ,"
                                     "  \"defaultValue\": 0         ,"
                                     "  \"toolTip\"     : \"Тип результата\","
                                     "  \"values\"      : [\"Комплексные\",\"Действительные\",\"Мнимые\",\"Амплитуды\",\"Фазы\"]"
                                     "}";
    return QString();
}

QVariant FrfFunction::m_getProperty(const QString &property) const
{DDD;
    if (property.startsWith("?/")) {
//        if (property == "?/processData") {
//            int type = map.value("type");
//            QStringList list;

//            switch (type) {
//                case 0: list << "PName=Передаточная ф-я H1"; break;
//                case 1: list << "PName=Передаточная ф-я H2"; break;
//                default: break;
//            }
//            int referenceChannel = m_input2->getParameter("?/referenceChannelIndex").toInt();
//            list << QString("pBaseChan=%1,").arg(referenceChannel);
//            list << QString("ProcChansList=%1,%2").arg(1).arg(referenceChannel);
//            list << QString("BlockIn=%1").arg(m_input->getParameter("?/blockSize").toInt());
//            list << QString("Wind=%1").arg(m_input->getParameter("?/windowDescription").toString());
//            list << QString("TypeAver=%1").arg(m_input->getParameter("?/averaging").toString());
//            list << "pTime=(0000000000000000)";
//            return list;
//        }
        if (property == "?/dataType") {
            switch (map.value("output")) {// dfd не различает H1 и H2
                case 0: return 154; //DiNike,
                case 1: return 150; //TrFuncRe
                case 2: return 151; //TrFuncIm
                case 3: return 147; //TransFunc
                case 4: return 147; //dfd не имет типа данных TrFuncPhas
                default: return 147;
            }
        }
        if (property == "?/xName") return "Гц";
//        if (property == "?/xType") return 18; //frequency
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xStep") {
            return m_input->getParameter("?/sampleRate").toDouble() / m_input->getParameter("?/blockSize").toDouble();
        }
        if (property == "?/functionDescription") {
            switch (map.value("type")) {
                case 0: return "H1"; break;
                case 1: return "H2"; break;
                default: return "H1";
            }
        }
//        if (property == "?/functionType") {
//            return 4; //FRF
//        }
//        if (property == "?/normalization") {
//            return 0; //no normalization
//        }
        if (property == "?/dataFormat") {
            switch (map.value("output")) {
                case 0: return "complex"; break;
                case 1: return "real"; break;
                case 2: return "imaginary"; break;
                case 3: return "amplitude"; break;
                case 4: return "phase"; break;
                default: return "unknown";
            }
        }
        if (property == "?/yValuesUnits") {
            switch (map.value("output")) {
                case 0:
                case 1:
                case 2:
                case 3: return DataHolder::UnitsLinear;
                case 4: return DataHolder::UnitsDimensionless;
                default: return "unknown";
            }
        }
        if (property == "?/yName") {
            QString s1 = m_input->getParameter("?/yNameOld").toString();
            QString s2 = "?";
            if (m_file) {
                int refIndex = m_input2->getParameter("?/referenceChannelIndex").toInt()-1;
                if (m_file->channel(refIndex)) {
                    s2 = m_file->channel(refIndex)->yName();
                }
            }
            return QString("%1/%2").arg(s1).arg(s2);
        }

        if (property == "?/referenceChannelIndex" && m_input2)
            return m_input2->getParameter(property);

        // do not know anything about these broadcast properties
        if (m_input) return m_input->getParameter(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();

    QString p = property.section("/",1);
    return map.value(p);
}

void FrfFunction::m_setProperty(const QString &property, const QVariant &val)
{DDD;
    if (!property.startsWith(name()+"/")) return;

    QString p = property.section("/",1);
    map.insert(p, val.toInt());
}

QString FrfFunction::displayName() const
{DDD;
    return "Передаточная";
}

bool FrfFunction::compute(FileDescriptor *file)
{DDD;
    output.clear();

    if (!m_input || !m_input2) return false;

    const int dataFormat = map.value("output");

    switch (map.value("type")) {
        case 0: {//"H1 = Sab/Saa
            m_input->compute(file);

            QVector<double> data = m_input->getData("input");
            if (data.isEmpty()) return false;
            m_input2->resetData();

            if (cashedReferenceOutput.isEmpty()) {
               m_input2->compute(file);

                QVector<double> data2 = m_input2->getData("input");
                if (data2.isEmpty()) return false;
                cashedReferenceOutput = data2;
            }

            //Sab is from data
            //Saa is from data2
            for (int i=0; i<data.size()-1; i+=2) {
                cx_double ab = {data[i], data[i+1]};
                cx_double aa = {cashedReferenceOutput[i], cashedReferenceOutput[i+1]};
                cx_double h1 = ab/aa;
                switch (dataFormat) {
                    case 0: output << h1.real() << h1.imag(); break;
                    case 1: output << h1.real(); break;
                    case 2: output << h1.imag(); break;
                    case 3: output << std::abs(h1); break;
                    case 4: output << std::arg(h1) * 180/M_PI; break;
                    default: output << h1.real() << h1.imag();;
                }

            }
            break;
        }
        case 1: {//"H2" = Sbb/Sba
            if (cashedReferenceOutput.isEmpty()) {
                if (!m_input->compute(file)) return false;
                QVector<double> data = m_input->getData("input");
                if (data.isEmpty()) return false;
                cashedReferenceOutput = data;
            }

            if (!m_input2->compute(file)) return false;
            QVector<double> data2 = m_input2->getData("input");
            if (data2.isEmpty()) return false;



            //Sbb is from data
            //Sba is from data2
            for (int i=0; i<cashedReferenceOutput.size()-1; i+=2) {
                cx_double bb = {cashedReferenceOutput[i], cashedReferenceOutput[i+1]};
                cx_double ba = {data2[i], data2[i+1]};
                cx_double h2 = bb/ba;
                switch (dataFormat) {
                    case 0: output << h2.real() << h2.imag(); break;
                    case 1: output << h2.real(); break;
                    case 2: output << h2.imag(); break;
                    case 3: output << std::abs(h2); break;
                    case 4: output << std::arg(h2) * 180/M_PI; break;
                    default: output << h2.real() << h2.imag();;
                }
            }
            break;
        }
    }
    return true;
}

void FrfFunction::reset()
{DDD;
    output.clear();
}



DataDescription FrfFunction::getFunctionDescription() const
{
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();

    switch (map.value("output")) {
        case 0: result.put("function.format", "complex"); break;
        case 1: result.put("function.format", "real"); break;
        case 2: result.put("function.format", "imaginary"); break;
        case 3: result.put("function.format", "amplitude"); break;
        case 4: result.put("function.format", "phase"); break;
        default: break;
    }
    result.put("function.precision", "float");
    result.put("function.name", "FRF");
    result.put("function.type", 4);
    result.put("function.description", map.value("type")==0?"H1":"H2");
    result.put("function.octaveFormat", 0);
    result.put("function.logscale", "linear");
    result.put("function.logref", 1);

    return result;
}
