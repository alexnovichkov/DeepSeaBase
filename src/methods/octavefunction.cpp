#include "octavefunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

OctaveFunction::OctaveFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString OctaveFunction::name() const
{
    return "OCTF";
}

QString OctaveFunction::description() const
{
    return "Октавный спектр";
}

QStringList OctaveFunction::properties() const
{
    return {"type"};
}

QString OctaveFunction::propertyDescription(const QString &property) const
{
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
{
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
            return 155+static_cast<int>(bank.getType());
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
{
    if (property == "OCTF/type") {
        switch (val.toInt()) {
            case 0: bank.setType(OctaveType::Octave3); break;
            case 1: bank.setType(OctaveType::Octave1); break;
        }
    }
}

QString OctaveFunction::displayName() const
{
    return description();
}

bool OctaveFunction::compute(FileDescriptor *file)
{
    output.clear();

    if (!m_input) return false;

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
{
 //no-op
}

DataDescription OctaveFunction::getFunctionDescription() const
{
    DataDescription d;
    if (m_input) d = m_input->getFunctionDescription();
    d.put("function.format", "amplitude");
    d.put("function.octaveFormat", static_cast<int>(bank.getType()));
    d.put("function.precision", "float");
    d.put("function.name", "OCTF");
    d.put("function.type", 9);
    d.put("function.logscale", "quadratic");
    //d.put("function.description", map.value("type")==0?"H1":"H2");
    return d;


}



void OctaveFunction::updateProperty(const QString &property, const QVariant &val)
{
    if (property == "?/blockSize") bank.setBlockSize(val.toInt());
}
