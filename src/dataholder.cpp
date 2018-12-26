#include "dataholder.h"
#include "logging.h"

DataHolder::DataHolder()
{DD;
    m_xBegin = 0.0;
    m_xStep = 0.0;
    m_count = 0;
    m_threshold = 0.0;
    m_xValuesFormat = XValuesUnknown;
    m_yValuesFormat = YValuesUnknown;
    m_yValuesUnits = UnitsUnknown;
    m_yValuesPresentation = ShowAsDefault;

    correctionValue = 0.0;
    correctionType = 0; //adding
}

DataHolder::DataHolder(const DataHolder &other)
{DD;
    m_xBegin = other.m_xBegin;
    m_xStep = other.m_xStep;
    m_count = other.m_count;
    m_threshold = other.m_threshold;
    m_xValues = other.m_xValues;
    m_yValues = other.m_yValues;
    m_yValuesTemporal = other.m_yValuesTemporal;
    m_yValuesComplex = other.m_yValuesComplex;
    m_xValuesFormat = other.m_xValuesFormat;
    m_yValuesFormat = other.m_yValuesFormat;
    m_yValuesUnits = other.m_yValuesUnits;

    m_yMin = other.m_yMin;
    m_yMax = other.m_yMax;
    m_yValuesPresentation = other.m_yValuesPresentation;

    correctionValue = other.correctionValue;
    correctionType = other.correctionType;
}

void DataHolder::setSegment(const DataHolder &other, int from, int to)
{DD;
    m_xBegin = other.m_xBegin + other.m_xStep * from;
    m_xStep = other.m_xStep;
    m_count = to - from + 1;
    m_threshold = other.m_threshold;
    m_xValues = other.m_xValues.mid(from, to - from + 1);
    m_yValues = other.m_yValues.mid(from, to - from + 1);
    m_yValuesTemporal = other.m_yValuesTemporal.mid(from, to - from + 1);
    m_yValuesComplex = other.m_yValuesComplex.mid(from, to - from + 1);
    m_xValuesFormat = other.m_xValuesFormat;
    m_yValuesFormat = other.m_yValuesFormat;
    m_yValuesUnits = other.m_yValuesUnits;

    m_yValuesPresentation = other.m_yValuesPresentation;

    correctionValue = other.correctionValue;
    correctionType = other.correctionType;
    recalculateMinMax();
}

void DataHolder::clear()
{DD;
    m_yValues.clear();         m_yValues.squeeze();
    m_yValuesComplex.clear();  m_yValuesComplex.squeeze();
    m_xValues.clear();         m_xValues.squeeze();
    m_yValuesTemporal.clear(); m_yValuesTemporal.squeeze();

    //some statistics
    m_yMin = 0; m_yMax = 0;

    correctionValue = 0.0;
    correctionType = 0;
}

void DataHolder::setCorrection(double correctionValue, int type)
{DD;
    this->correctionValue = correctionValue;
    this->correctionType = type;
}

void DataHolder::removeCorrection()
{
    if (correctionType == 0) correctionValue = 0.0;
    if (correctionType == 1) correctionValue = 1.0;
}

QString DataHolder::yValuesFormatString() const
{
    switch (m_yValuesFormat) {
        case YValuesUnknown: return "Неизв."; break;
        case YValuesComplex: return "[Cmplx]"; break;
        case YValuesReals: return "[Re]"; break;
        case YValuesImags: return "[Im]"; break;
        case YValuesAmplitudes: return "[Abs]"; break;
        case YValuesAmplitudesInDB: return "[dB]"; break;
        case YValuesPhases: return "[Phase]"; break;
    }
}

void DataHolder::setYValuesPresentation(int presentation)
{DD;
    if (m_yValuesPresentation == DataHolder::YValuesPresentation(presentation)) return;

    m_yValuesPresentation = DataHolder::YValuesPresentation(presentation);
    recalculateYValues();
    recalculateMinMax();
}

void DataHolder::setYValues(const QVector<double> &values, YValuesFormat initially)
{DD;
    m_yValuesFormat = initially; // амплитуды (чаще всего), фазы, мнимые значения и т.д.
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = YValuesPresentation(initially); // используем тот же формат, что и исходные данные
    }

    m_yValues = values;

    m_yValuesComplex.clear(); // комплексные значения не нужны, очищаем
    m_yValuesComplex.squeeze();

    recalculateYValues(); // нужно, чтобы подцепился указатель на данные для построения кривых
    recalculateMinMax();
}

bool DataHolder::setYValue(int index, double value)
{
    if (index <0 || index >= m_count) return false;
    if (m_yValuesTemporal[index] == value) return false;

    if (int(m_yValuesFormat) == int(m_yValuesPresentation)) {
        m_yValues[index] = value;
        m_yValuesTemporal = m_yValues;
        return true;
    }

//    m_yValuesTemporal[index] = value;
    if (m_yValuesFormat == YValuesComplex) {
        m_yValuesTemporal[index] = value;
        switch (m_yValuesPresentation) {
            case ShowAsDefault:
            case ShowAsAmplitudesInDB:
                value = fromLog(value, m_threshold, m_yValuesUnits);
                m_yValuesComplex[index] = std::polar(value, std::arg(m_yValuesComplex[index]));
                return true;
            case ShowAsAmplitudes:
                m_yValuesComplex[index] = std::polar(value, std::arg(m_yValuesComplex[index]));
                return true;
            case ShowAsImags:
                m_yValuesComplex[index] = {m_yValuesComplex[index].real(), value};
                return true;
            case ShowAsPhases:
                m_yValuesComplex[index] = std::polar(std::abs(m_yValuesComplex[index]), value);
                return true;
            case ShowAsReals:
                m_yValuesComplex[index] = {value, m_yValuesComplex[index].imag()};
                return true;
            default: return false;
        };
    }
    else {
        switch (m_yValuesFormat) {
            case YValuesReals: {
                if (m_yValuesPresentation == ShowAsAmplitudes) {
                    if (value < 0) return false;
                    if (m_yValues[index] >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else if (m_yValuesPresentation == ShowAsAmplitudesInDB) {
                    value = fromLog(value, m_threshold, m_yValuesUnits);
                    if (m_yValues[index] >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else return false;
            }
            case YValuesImags: {
                if (m_yValuesPresentation == ShowAsAmplitudes) {
                    if (value < 0) return false;
                    if (m_yValues[index] >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else if (m_yValuesPresentation == ShowAsAmplitudesInDB) {
                    value = fromLog(value, m_threshold, m_yValuesUnits);
                    if (m_yValues[index] >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else return false;
            }
            case YValuesAmplitudes: {
                if (m_yValuesPresentation == ShowAsAmplitudesInDB) {
                    m_yValues[index] = fromLog(value, m_threshold, m_yValuesUnits);
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                return false;
            }
            case YValuesAmplitudesInDB: {
                if (m_yValuesPresentation == ShowAsAmplitudes) {
                     m_yValues[index] = toLog(value, m_threshold, m_yValuesUnits);
                     m_yValuesTemporal[index] = value;
                     return true;
                 }
                 return false;
            }
            default:
                return false;
        }
    }
    return false;
}

void DataHolder::setYValues(const QVector<cx_double> &values)
{DD;
    m_yValuesFormat = YValuesComplex;
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = ShowAsAmplitudesInDB;
    }

    m_yValuesComplex = values;

    m_yValues.clear(); // действительные значения не нужны, очищаем
    m_yValues.squeeze();
//    m_yValuesTemporal.clear();

    recalculateYValues(); // заполняем вектор действительных значений,
                          // который будет использоваться для графиков
    recalculateMinMax();
}

void DataHolder::setXValues(const QVector<double> &values)
{DD;
    m_xValues = values;
    m_count = values.size();
    m_xBegin = values.isEmpty() ? 0.0 : values.first();
    m_xStep = 0.0;
    m_xValuesFormat = XValuesNonUniform;
}

void DataHolder::setXValues(double xBegin, double xStep, int count)
{DD;
    m_xBegin = xBegin;
    m_xStep = xStep;
    m_count = count;
    m_xValuesFormat = XValuesUniform;
    m_xValues.clear();
    m_xValues.squeeze();
}

void DataHolder::setXStep(const double xStep)
{DD;
    if (m_xValuesFormat == XValuesUniform) {
        m_xStep = xStep;
    }
}

void DataHolder::setSamplesCount(const int samplesCount)
{
    m_count = samplesCount;
}

QVector<double> DataHolder::yValues() const
{
//    if (m_yValuesTemporal.isEmpty()) {
//        recalculateYValues();
//        recalculateMinMax();
//    }
    return m_yValuesTemporal;
}

QVector<double> DataHolder::xValues() const
{DD;
    if (xValuesFormat() == XValuesUniform) return linspace(m_xBegin, m_xStep, m_count);

    return m_xValues;
}

double DataHolder::xValue(int i) const
{
    if (xValuesFormat() != XValuesNonUniform) return m_xBegin + m_xStep * i;

    if (i<m_count) return m_xValues[i];
    return 0.0;
}

double DataHolder::yValue(int i) const
{
    return corrected(m_yValuesTemporal[i]);
}

cx_double DataHolder::yValueComplex(int i) const
{
    return m_yValuesComplex[i];
}

double DataHolder::xMin() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xBegin;
    if (xValuesFormat() == XValuesNonUniform && !m_xValues.isEmpty()) return m_xValues.first();
    return 0;
}

double DataHolder::xMax() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xBegin + m_xStep*(m_count-1);
    if (xValuesFormat() == XValuesNonUniform && !m_xValues.isEmpty()) return m_xValues.last();
    return 0;
}

double DataHolder::xStep() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xStep;

    return 0;
}

double DataHolder::yMin() const
{DD;
    return corrected(m_yMin);
}

double DataHolder::yMax() const
{DD;
    return corrected(m_yMax);
}

int DataHolder::samplesCount() const
{DD;
    return m_count;
}

QVector<double> DataHolder::linears() const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex: return absolutes(m_yValuesComplex);
        case YValuesAmplitudesInDB: return fromLog(m_yValues, m_threshold, m_yValuesUnits);
        default: break;
    }
    return m_yValues;
}

QVector<double> DataHolder::decibels() const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex: return toLog(absolutes(m_yValuesComplex), m_threshold, m_yValuesUnits);
        case YValuesAmplitudesInDB: return m_yValues;
        default: break;
    }
    return toLog(m_yValues, m_threshold, m_yValuesUnits);
}

QVector<double> DataHolder::toLog(const QVector<double> &values, double threshold, int units)
{DD;
    if (threshold == 0 || units == UnitsDimentionless) return values;
    if (values.isEmpty()) return QVector<double>();

    QVector<double> a(values.size());
    double factor = 20.0;
    double thr = threshold;
    if (units == UnitsQuadratic) {
        factor = 10.0;
        thr *= thr;
    }
    for (int i=0; i<values.size(); ++i) {
        if (values[i] <= 0.0) a[i] = 0.0;
        else {
            a[i] = factor*log10(values[i]/thr);
        }
    }
    return a;
}

QVector<double> DataHolder::fromLog(const QVector<double> &values, double threshold, int units)
{DD;
    if (threshold == 0 || units == UnitsDimentionless) return values;
    if (values.isEmpty()) return QVector<double>();

    QVector<double> a(values.size());
    double factor = log(10) / 20.0;
    double thr = threshold;
    if (units == UnitsQuadratic) {
        factor = log(10) / 10.0;
        thr *= thr;
    }

    for (int i=0; i<values.size(); ++i) {
        a[i] = thr*exp(values[i]*factor);
    }
    return a;
}

double DataHolder::toLog(double value, double threshold, int units)
{
    if (threshold == 0 || units == UnitsDimentionless) return value;

    if (value <= 0.0) return 0.0;
    double factor = 20.0;
    double thr = threshold;
    if (units == UnitsQuadratic) {
        factor = 10.0;
        thr *= thr;
    }
    return factor*log10(value/thr);
}

double DataHolder::fromLog(double value, double threshold, int units)
{
    if (threshold == 0 || units == UnitsDimentionless) return value;

    double factor = log(10) / 20.0;
    double thr = threshold;
    if (units == UnitsQuadratic) {
        factor = log(10) / 10.0;
        thr *= thr;
    }

    return thr*exp(value*factor);
}

void DataHolder::recalculateMinMax()
{DD;
    if (!m_yValuesTemporal.isEmpty()) {
        auto minmax = std::minmax_element(m_yValuesTemporal.begin(), m_yValuesTemporal.end());
        m_yMin = *(minmax.first);
        m_yMax = *(minmax.second);
    }
}

void DataHolder::recalculateYValues()
{DD;
    m_yValuesTemporal.clear();

    if (int(m_yValuesFormat) == int(m_yValuesPresentation)) {
        m_yValuesTemporal = m_yValues;
    }
    else {
        if (m_yValuesFormat == YValuesComplex) {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(absolutes(m_yValuesComplex), m_threshold, m_yValuesUnits); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = absolutes(m_yValuesComplex); break;
                case ShowAsImags:          m_yValuesTemporal = ::imags(m_yValuesComplex); break;
                case ShowAsPhases:         m_yValuesTemporal = ::phases(m_yValuesComplex); break;
                case ShowAsReals:          m_yValuesTemporal = ::reals(m_yValuesComplex); break;
            };
        }
        else {
            switch (m_yValuesFormat) {
                case YValuesReals: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = absolutes(m_yValues);
                    else if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold, m_yValuesUnits);
                    else if (m_yValuesPresentation == ShowAsPhases)
                        m_yValuesTemporal = ::phases(complexes(m_yValues));
                    break;
                }
                case YValuesImags: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = absolutes(m_yValues);
                    else if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold, m_yValuesUnits);
                    else if (m_yValuesPresentation == ShowAsPhases)
                        m_yValuesTemporal = ::phases(complexes(m_yValues, false));
                    break;
                }
                case YValuesAmplitudes: {
                    if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(m_yValues, m_threshold, m_yValuesUnits);
                    break;
                }
                case YValuesAmplitudesInDB: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = fromLog(m_yValues, m_threshold, m_yValuesUnits);
                    break;
                }
                default:
                    break;
            }
        }
    }

    if (m_count > 0 && m_yValuesTemporal.isEmpty())
        m_yValuesTemporal = QVector<double>(m_count, 0.0);
    m_yValuesTemporal.squeeze();
}

double DataHolder::corrected(double val) const
{
    if (correctionType == 0) return val + correctionValue;
    if (correctionType == 1) return val * correctionValue;
    return 0.0;
}
