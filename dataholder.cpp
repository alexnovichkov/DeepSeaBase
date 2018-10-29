#include "dataholder.h"
#include "logging.h"

DataHolder::DataHolder()
{DD;
    m_xBegin = 0.0;
    m_xStep = 0.0;
    m_count = 0;
    m_threshold = 0.0;
    m_xValuesFormat = XValuesUnknown;
    m_initially = YValuesUnknown;
    m_yValuesPresentation = ShowAsDefault;

    correctionValue = 0.0;
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
    m_initially = other.m_initially;

    m_yMin = other.m_yMin;
    m_yMax = other.m_yMax;
    m_yValuesPresentation = other.m_yValuesPresentation;

    correctionValue = other.correctionValue;
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
    m_initially = other.m_initially;

    m_yValuesPresentation = other.m_yValuesPresentation;

    correctionValue = other.correctionValue;
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
}

void DataHolder::setCorrection(double correctionValue)
{DD;
    this->correctionValue = correctionValue;
}

void DataHolder::setYValuesPresentation(DataHolder::YValuesPresentation presentation)
{DD;
    if (m_yValuesPresentation == presentation) return;

    m_yValuesPresentation = presentation;
    recalculateYValues();
    recalculateMinMax();
}

void DataHolder::setYValues(const QVector<double> &values, YValuesFormat initially)
{DD;
    m_initially = initially; // амплитуды (чаще всего), фазы, мнимые значения и т.д.
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = YValuesPresentation(initially); // используем тот же формат, что и исходные данные
    }

    m_yValues = values;

    m_yValuesComplex.clear(); // комплексные значения не нужны, очищаем
    m_yValuesComplex.squeeze();

    recalculateYValues(); // нужно, чтобы подцепился указатель на данные для построения кривых
    recalculateMinMax();
}

void DataHolder::setYValues(const QVector<cx_double> &values)
{DD;
    m_initially = YValuesComplex;
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = ShowAsAmplitudesInDB;
    }

    m_yValuesComplex = values;

    m_yValues.clear(); // действительные значения не нужны, очищаем
    m_yValues.squeeze();

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

QVector<double> DataHolder::xValues() const
{DD;
    if (xValuesFormat() == XValuesUniform) return linspace(m_xBegin, m_xStep, m_count);

    return m_xValues;
}

double DataHolder::xValue(int i) const
{
    if (xValuesFormat() == XValuesUniform) return m_xBegin + m_xStep * i;

    return m_xValues[i];
}

double DataHolder::yValue(int i) const
{
    return m_yValuesTemporal[i] + correctionValue;
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
    return m_yMin + correctionValue;
}

double DataHolder::yMax() const
{DD;
    return m_yMax + correctionValue;
}

int DataHolder::samplesCount() const
{DD;
    return m_count;
}

QVector<double> DataHolder::linears() const
{DD;
    switch (m_initially) {
        case YValuesComplex: return absolutes(m_yValuesComplex);
        case YValuesAmplitudesInDB: return fromLog(m_yValues, m_threshold);
        default: break;
    }
    return m_yValues;
}

QVector<double> DataHolder::decibels() const
{DD;
    switch (m_initially) {
        case YValuesComplex: return toLog(absolutes(m_yValuesComplex), m_threshold);
        case YValuesAmplitudesInDB: return m_yValues;
        default: break;
    }
    return toLog(m_yValues, m_threshold);
}

QVector<double> DataHolder::toLog(const QVector<double> &values, double threshold)
{DD;
    if (threshold == 0) return values;
    if (values.isEmpty()) return QVector<double>();

    QVector<double> a(values.size());
    for (int i=0; i<values.size(); ++i) {
        if (values[i] <= 0.0) a[i] = 0.0;
        else a[i] = 20*log10(values[i]/threshold);
    }
    return a;
}

QVector<double> DataHolder::fromLog(const QVector<double> &values, double threshold)
{DD;
    if (threshold == 0) return values;
    if (values.isEmpty()) return QVector<double>();

    QVector<double> a(values.size());
    double factor = log(10.0)/20.0;
    for (int i=0; i<values.size(); ++i) {
        a[i] = threshold*exp(values[i]*factor);
    }
    return a;
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

    if (int(m_initially) == int(m_yValuesPresentation)) {
        m_yValuesTemporal = m_yValues;
    }
    else {
        if (m_initially == YValuesComplex) {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(absolutes(m_yValuesComplex), m_threshold); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = absolutes(m_yValuesComplex); break;
                case ShowAsImags:          m_yValuesTemporal = ::imags(m_yValuesComplex); break;
                case ShowAsPhases:         m_yValuesTemporal = ::phases(m_yValuesComplex); break;
                case ShowAsReals:          m_yValuesTemporal = ::reals(m_yValuesComplex); break;
            };
        }
        else {
            switch (m_initially) {
                case YValuesReals: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = absolutes(m_yValues);
                    else if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold);
                    else if (m_yValuesPresentation == ShowAsPhases)
                        m_yValuesTemporal = ::phases(complexes(m_yValues));
                    break;
                }
                case YValuesImags: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = absolutes(m_yValues);
                    else if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold);
                    else if (m_yValuesPresentation == ShowAsPhases)
                        m_yValuesTemporal = ::phases(complexes(m_yValues, false));
                    break;
                }
                case YValuesAmplitudes: {
                    if (m_yValuesPresentation == ShowAsAmplitudesInDB)
                        m_yValuesTemporal = toLog(m_yValues, m_threshold);
                    break;
                }
                case YValuesAmplitudesInDB: {
                    if (m_yValuesPresentation == ShowAsAmplitudes)
                        m_yValuesTemporal = fromLog(m_yValues, m_threshold);
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