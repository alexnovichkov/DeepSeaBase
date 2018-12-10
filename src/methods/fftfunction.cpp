#include "fftfunction.h"

#include "filedescriptor.h"
#include "fft.h"

FftFunction::FftFunction(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractFunction(dataBase, parent)
{

}

QString FftFunction::name() const
{
    return "Spectrum";
}

QString FftFunction::description() const
{
    return "Спектр";
}

QStringList FftFunction::properties() const
{
    return QStringList()<<"type"<<"output";
}

QString FftFunction::propertyDescription(const QString &property) const
{
    if (property == "type") return "{"
                                   "  \"name\"        : \"type\"   ,"
                                   "  \"type\"        : \"enum\"   ,"
                                   "  \"displayName\" : \"Тип\"   ,"
                                   "  \"defaultValue\": 0         ,"
                                   "  \"toolTip\"     : \"Тип спектральной характеристики\","
                                   "  \"values\"      : [\"FFT\",\"Спектр мощности\",\"Спектральная плотность мощности\"]"
                                   "}";
    if (property == "output") {
        QStringList values;
        if (map.value("type") == 0) values << "\"Комплексные\"" << "\"Действительные\"" << "\"Мнимые\"";
        values << "\"Амплитуды\"";
        if (map.value("type") == 0) values << "\"Фазы\"";
        return QString("{"
               "  \"name\"        : \"output\"   ,"
               "  \"type\"        : \"enum\"   ,"
               "  \"displayName\" : \"Значения\"   ,"
               "  \"defaultValue\": 0         ,"
               "  \"toolTip\"     : \"Тип результата\","
               "  \"values\"      : [%1]"
               "}").arg(values.join(","));
    }

    return QString();
}

QVariant FftFunction::getProperty(const QString &property) const
{
    if (property.startsWith("?/")) {
        // do not know anything about these broadcast properties
        if (m_input) return m_input->getProperty(property);
    }
    if (!property.startsWith(name()+"/")) return QVariant();

    QString p = property.section("/",1);
    return map.value(p);
}

void FftFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;

    QString p = property.section("/",1);
    map.insert(p, val.toInt());
}

QString FftFunction::displayName() const
{
    return "Спектр";
}

//returns fft of data
QVector<double> FftFunction::get(FileDescriptor *file, const QVector<double> &data)
{

}


bool FftFunction::propertyShowsFor(const QString &property) const
{
    if (!property.startsWith(name()+"/")) return false;

    QString p = property.section("/",1);
    if (p == "output") return (map.value("type") == 0);

    return true;
}

QVector<double> FftFunction::getData(const QString &id)
{
    if (id == "input")
        return output;

    return QVector<double>();
}

bool FftFunction::compute()
{
    output.clear();

    if (!m_input) return false;

    if (!m_input->compute()) return false;

    QVector<double> data = m_input->getData("input");
    if (data.isEmpty()) return false;

    double sampleRate = m_input->getProperty("?/sampleRate").toDouble();

    QVector<cx_double> fft = Fft::compute(data);
//    for (int i=0; i<fft.size(); ++i) {
//        qDebug()<<fft[i].real()<<fft[i].imag();
//    }
    int size = int(fft.size()/2.56);
    switch (map.value("type")) {
        case 0: //FFT
            switch (map.value("output")) {
                case 0: {//complex
                    output.resize(size*2);
                    for (int i=0; i<size; ++i) {
                        output[i*2] = fft[i].real();
                        output[i*2+1] = fft[i].imag();
                    }

                    break;
                }
                case 1: {//real
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = fft[i].real();
                    }
                    break;
                }
                case 2: {//imag
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = fft[i].imag();
                    }
                    break;
                }
                case 3: {//ampl
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = std::abs(fft[i]);
                    }
                    break;
                }
                case 4: //phase
                    output.resize(size);
                    for (int i=0; i<size; ++i) {
                        output[i] = std::arg(fft[i])*180.0/M_PI;
                    }
            }
            break;
        case 1: {//Спектр мощности
            const int Nvl = fft.size();
            const double factor = 2.0 / Nvl/Nvl;
            output.resize(size);
            for (int i = 0; i < size; i++) {
                output[i] = factor * std::norm(fft[i]);
            }
            break;
        }
        case 2: {//Спектральная плотность мощности
            const int Nvl = fft.size();
            const double factor = 2.0 / Nvl / sampleRate;
            output.resize(size);
            for (int i = 0; i < size; i++) {
                output[i] = factor * std::arg(fft[i]);
            }
            break;
        }
        default:
            break;
    }
    return true;
}

void FftFunction::reset()
{
    output.clear();
}
