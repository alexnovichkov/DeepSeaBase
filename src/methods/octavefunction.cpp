#include "octavefunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

OctaveFunction::OctaveFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString OctaveFunction::name() const
{DD;
    return "OCTF";
}

QString OctaveFunction::description() const
{DD;
    return "Октавный спектр";
}

QStringList OctaveFunction::properties() const
{DD;
    return {"type"};
}

QString OctaveFunction::propertyDescription(const QString &property) const
{DD;
    if (property == "type") return "{"
                                      "  \"name\"        : \"type\"   ,"
                                      "  \"type\"        : \"enum\"   ,"
                                      "  \"displayName\" : \"Тип\"   ,"
                                      "  \"defaultValue\": 0         ,"
                                      "  \"toolTip\"     : \"Тип спектра\","
                                      "  \"values\"      : [\"1/3-октавный\",\"октавный\"]"
                                      "}";
    return QString();
}

QVariant OctaveFunction::m_getProperty(const QString &property) const
{DD;
    if (m_input && property.startsWith("?/")) {
        if (property == "?/octaveFormat") return static_cast<int>(bank.getType());
        if (property == "?/abscissaData") {
            const auto data = bank.getFrequencies(true);
            QVariantList l;
            for (double x: data) l << x;
            return l;
        }
        if (property == "?/abscissaEven") return false;
        if (property == "?/portionsCount") return portionsCount;

        if (property == "?/dataType") {
            switch (bank.getType()) {
                case OctaveType::Octave1: return 156;
                case OctaveType::Octave3: return 157;
                case OctaveType::Octave2: return 158;
                case OctaveType::Octave6: return 159;
                case OctaveType::Octave12: return 160;
                case OctaveType::Octave24: return 161;
                default: break;
            }
        }
        if (property == "?/xName") return "Гц";
        if (property == "?/xBegin") {
            auto data = bank.getFrequencies(true);
            return data.value(0);
        }
        if (property == "?/xStep") return 0.0;
        if (property == "?/functionDescription") return "OCTF";
        if (property == "?/dataFormat") return "amplitude";
        if (property == "?/yValuesUnits") return DataHolder::UnitsQuadratic;

        return m_input->getParameter(property);
    }

    if (property == "OCTF/type") return static_cast<int>(bank.getType());

    return QVariant();
}

void OctaveFunction::m_setProperty(const QString &property, const QVariant &val)
{DD;
    if (property == "OCTF/type") {
        switch (val.toInt()) {
            case 0: bank.setType(OctaveType::Octave3); break;
            case 1: bank.setType(OctaveType::Octave1); break;
        }
    }
}

QString OctaveFunction::displayName() const
{DD;
    return description();
}

bool OctaveFunction::compute(FileDescriptor *file)
{DD;
    output.clear();

    if (!m_input) return false;
    LOG(INFO) << QString("Запуск расчета для функции октавного спектра");
    if (!m_input->compute(file)) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    //данные приходят сразу для всего канала, поэтому мы должны разбить их по блокам
//    const int blockSize = m_input->getParameter("?/blockSize").toInt();
    constexpr const int blockSize = 65536;
    portionsCount = data.size()/blockSize;

    const  double sr = m_input->getParameter("?/sampleRate").toDouble();
    const double logref = m_input->getParameter("?/logref").toDouble();

    for (int block = 0; block < portionsCount; ++block) {
        auto d = data.mid(block*blockSize, blockSize);
        //d.resize(blockSize);
        auto res = bank.compute(d, sr, logref);
        output.append(res.last());
    }
    return true;
}

void OctaveFunction::reset()
{DD;
 //no-op
}

DataDescription OctaveFunction::getFunctionDescription() const
{DD;
    DataDescription d = AbstractFunction::getFunctionDescription();
    d.put("function.format", "amplitude");
    d.put("function.octaveFormat", static_cast<int>(bank.getType()));
    d.put("function.name", "OCTF");
    d.put("function.type", 9);
    d.put("function.logscale", "quadratic");
    //d.put("function.description", map.value("type")==0?"H1":"H2");
    return d;


}



void OctaveFunction::updateProperty(const QString &property, const QVariant &val)
{DD;
    if (property == "?/blockSize") bank.setBlockSize(val.toInt());
}
