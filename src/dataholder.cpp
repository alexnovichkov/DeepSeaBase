#include "dataholder.h"
#include "logging.h"

DataHolder::DataHolder()
{DD;
    m_threshold = 0.0;
    m_xValuesFormat = XValuesUnknown;
    m_zValuesFormat = XValuesUnknown;
    m_yValuesFormat = YValuesUnknown;
    m_yValuesUnits = UnitsUnknown;
    m_yValuesPresentation = ShowAsDefault;

    m_correctionValue = 0.0;
    m_correctionType = 0; //adding
    m_PresentationWhenCorrecting = ShowAsDefault;
    m_correction = false;
}

DataHolder::DataHolder(const DataHolder &other)
{DD;
    m_xBegin = other.m_xBegin;
    m_xStep = other.m_xStep;
    m_zBegin = other.m_zBegin;
    m_zStep = other.m_zStep;
    m_xCount = other.m_xCount;
    m_zCount = other.m_zCount;
    m_threshold = other.m_threshold;
    m_xValues = other.m_xValues;
    m_yValues = other.m_yValues;
    m_zValues = other.m_zValues;
    m_yValuesTemporal = other.m_yValuesTemporal;
    m_yValuesComplex = other.m_yValuesComplex;
    m_xValuesFormat = other.m_xValuesFormat;
    m_zValuesFormat = other.m_zValuesFormat;
    m_yValuesFormat = other.m_yValuesFormat;
    m_yValuesUnits = other.m_yValuesUnits;

    m_yMin = other.m_yMin;
    m_yMax = other.m_yMax;
    m_yValuesPresentation = other.m_yValuesPresentation;

    m_correctionValue = other.m_correctionValue;
    m_correctionType = other.m_correctionType;
    m_PresentationWhenCorrecting = other.m_PresentationWhenCorrecting;
    m_correction = other.m_correction;
}

template<typename T>
QVector<T> segment(const QVector<T> &values, int from, int to, int blockSize, int blocks)
{
    QVector<T> result;
    result.reserve((to - from + 1)*blocks);

    for (int i=0; i<blocks; ++i) {
        result.append(values.mid(i*blockSize + from, to - from + 1));
    }
    return result;
}

void DataHolder::setSegment(const DataHolder &other, int from, int to)
{DD;
    m_xBegin = other.m_xBegin + other.m_xStep * from;
    m_xStep = other.m_xStep;
    m_zBegin = other.m_zBegin;
    m_zStep = other.m_zStep;
    m_xCount = to - from + 1;
    m_zCount = other.m_zCount;
    m_threshold = other.m_threshold;
    m_xValues = segment(other.m_xValues, from, to, m_xCount, m_zCount);
    m_yValues = segment(other.m_yValues, from, to, m_xCount, m_zCount);
    m_zValues = other.m_zValues;
    m_yValuesTemporal = segment(other.m_yValuesTemporal, from, to, m_xCount, m_zCount);
    m_yValuesComplex  = segment(other.m_yValuesComplex, from, to, m_xCount, m_zCount);
    m_xValuesFormat = other.m_xValuesFormat;
    m_zValuesFormat = other.m_zValuesFormat;
    m_yValuesFormat = other.m_yValuesFormat;
    m_yValuesUnits = other.m_yValuesUnits;

    m_yValuesPresentation = other.m_yValuesPresentation;

    m_correctionValue = other.m_correctionValue;
    m_correctionType = other.m_correctionType;
    m_PresentationWhenCorrecting = other.m_PresentationWhenCorrecting;
    m_correction = other.m_correction;
    recalculateMinMax();
}

void DataHolder::clear()
{DD;
    m_yValues.clear();         m_yValues.squeeze();
    m_yValuesComplex.clear();  m_yValuesComplex.squeeze();
    //m_xValues.clear();         m_xValues.squeeze();
    m_yValuesTemporal.clear(); m_yValuesTemporal.squeeze();

    //some statistics
    m_yMin.clear(); m_yMax.clear();

    m_correctionValue = 0.0;
    m_correctionType = 0;
    m_PresentationWhenCorrecting = ShowAsDefault;
    m_correction = false;
}

void DataHolder::setTemporaryCorrection(double correctionValue, int type)
{DD;
    this->m_correctionValue = correctionValue;
    this->m_correctionType = type;
    m_PresentationWhenCorrecting = m_yValuesPresentation;
    m_correction = true;
}

void DataHolder::removeCorrection()
{
    if (m_correctionType == 0) m_correctionValue = 0.0;
    if (m_correctionType == 1) m_correctionValue = 1.0;
    m_PresentationWhenCorrecting = ShowAsDefault;
    m_correction = false;
}

bool DataHolder::makeCorrectionConstant()
{
    if (!m_correction) return true; // есть временная коррекция
    if (!hasCorrection()) return true; // и эта коррекция не тривиальная

    if (int(m_PresentationWhenCorrecting) == int(m_yValuesFormat)) {
        // просто применяем поправку ко всем данным
        for (int i=0; i<m_yValues.size(); ++i)
            m_yValues[i] = corrected(m_yValues.at(i));
    }
    else {
        // необходима конвертация данных
        if (m_PresentationWhenCorrecting == ShowAsAmplitudesInDB) {
            // самый распространенный случай, поправки введены в дБ
            switch (m_yValuesFormat) {
                case YValuesComplex: {
                    if (m_correctionType == 0) {// слагаемое
                        double delta = fromLog(m_correctionValue, 1.0, m_yValuesUnits);
                        for (int i=0; i<m_yValuesComplex.size(); ++i)
                            m_yValuesComplex[i].operator *=(delta);
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            double abs_ = std::abs(m_yValuesComplex[i]);
                            if (!qFuzzyIsNull(abs_) && !qFuzzyIsNull(m_threshold)) {
                                double delta = m_threshold * (pow(abs_/m_threshold, m_correctionValue)) / abs_;
                                m_yValuesComplex[i].operator *=(delta);
                            }
                        }
                    }
                    break;
                }
                case YValuesReals:
                case YValuesImags:
                case YValuesAmplitudes: {
                    if (m_correctionType == 0) {// слагаемое
                        double delta = fromLog(m_correctionValue, 1.0, m_yValuesUnits);
                        for (int i=0; i<m_yValues.size(); ++i)
                            m_yValues[i] *= delta;
                    }
                    else if (m_correctionType == 1) {// множитель
                        if ((1.0 - m_correctionValue) >= 0.0) {
                            double thr = pow(m_threshold, 1.0 - m_correctionValue);
                            for (int i=0; i<m_yValues.size(); ++i)
                                m_yValues[i] = thr * pow(m_yValues[i], m_correctionValue);
                        }
                    }
                    break;
                }

                default: break;
            }
        }
        else if (m_PresentationWhenCorrecting == ShowAsAmplitudes) {
            // поправки введены в абсолютных значениях
            switch (m_yValuesFormat) {
                case YValuesComplex: {
                    if (m_correctionType == 0) {// слагаемое
                        // если к амплитуде добавили положительное число, все в порядке.
                        // если к амплитуде добавили отрицательное число, то проверяем, чтобы амплитуда не была меньше нуля.
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            double abs_ = std::abs(m_yValuesComplex[i]);
                            if (abs_ + m_correctionValue < 0.0)
                                m_yValuesComplex[i] = {0.0, 0.0};
                            else if (abs_ > 0.0)
                                m_yValuesComplex[i].operator *=(1.0 + m_correctionValue/abs_);
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i)
                            m_yValuesComplex[i].operator *=(m_correctionValue);
                    }
                    break;
                }
                case YValuesReals:
                case YValuesImags: {
                    if (m_correctionType == 0) {// слагаемое
                        for (int i=0; i<m_yValues.size(); ++i) {
                            if (abs(m_yValues[i]) + m_correctionValue < 0.0)
                                m_yValues[i] = 0.0;
                            else if (m_yValues[i] >= 0.0)
                                m_yValues[i] += m_correctionValue;
                            else
                                m_yValues[i] -= m_correctionValue;
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValues.size(); ++i) {
                            m_yValues[i] *= m_correctionValue;
                        }
                    }
                    break;
                }
                case YValuesAmplitudesInDB: {
                    if (m_correctionType == 0) {// слагаемое
                        for (int i=0; i<m_yValues.size(); ++i) {
                            if (m_yValuesUnits == UnitsLinear)
                                m_yValues[i] = 20.0 * log10((pow(10.0, m_yValues[i]/20.0)*m_threshold+m_correctionValue)/m_threshold);
                            else if (m_yValuesUnits == UnitsQuadratic)
                                m_yValues[i] = 10.0 * log10((pow(10.0, m_yValues[i]/10.0)*m_threshold*m_threshold
                                                             +m_correctionValue)/m_threshold/m_threshold);
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValues.size(); ++i)
                            m_yValues[i] += toLog(m_correctionValue, 1.0, m_yValuesUnits);
                    }
                    break;
                }

                default: break;
            }
        }
        else if (m_PresentationWhenCorrecting == ShowAsReals) {
            switch (m_yValuesFormat) {
                case YValuesComplex: {
                    if (m_correctionType == 0) {// слагаемое
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex[i].real() + m_correctionValue,
                                                   m_yValuesComplex[i].imag()};
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex[i].real() * m_correctionValue,
                                                   m_yValuesComplex[i].imag()};
                        }
                    }
                    break;
                }
                default: break;
            }
        }
        else if (m_PresentationWhenCorrecting == ShowAsImags) {
            switch (m_yValuesFormat) {
                case YValuesComplex: {
                    if (m_correctionType == 0) {// слагаемое
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex[i].real(),
                                                   m_yValuesComplex[i].imag() + m_correctionValue};
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex[i].real(),
                                                   m_yValuesComplex[i].imag() * m_correctionValue};
                        }
                    }
                    break;
                }
                default: break;
            }
        }

        else if (m_PresentationWhenCorrecting == ShowAsPhases) {
            // не шутите с фазой
        }
    }

    recalculateYValues();
    recalculateMinMax();
    return true;
}



QString DataHolder::correctionString() const
{
    return correctionString(m_correctionValue, m_correctionType);
}

QString DataHolder::correctionString(double value, int type)
{
    if ((type == 0 && qFuzzyIsNull(value)) || (type == 1 && qFuzzyIsNull(value - 1.0)))
        return QString();

    QString suffix;
    if (type == 0) {
        if (value > 0) suffix = "+";
    }
    else if (type == 1) suffix = "*";
    return QString("[%1%2]").arg(suffix).arg(value);
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
    return "Неизв.";
}

void DataHolder::setYValuesPresentation(int presentation)
{DD;
    if (m_yValuesPresentation == DataHolder::YValuesPresentation(presentation)) return;

    m_yValuesPresentation = DataHolder::YValuesPresentation(presentation);
    recalculateYValues();
    recalculateMinMax();
}

void DataHolder::setYValues(const QVector<double> &values, YValuesFormat initially, int block)
{DD;
    m_yValuesFormat = initially; // амплитуды (чаще всего), фазы, мнимые значения и т.д.
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = YValuesPresentation(initially); // используем тот же формат, что и исходные данные
    }

    if (block == -1) {
        //весь канал
        m_yValues = values;
    }
    else {
        if (m_yValues.size() < m_zCount*m_xCount) m_yValues.resize(m_zCount*m_xCount);
        for (int i=0; i<values.size(); ++i)
            m_yValues[block*m_xCount + i] = values.at(i);
    }

    m_yValuesComplex.clear(); // комплексные значения не нужны, очищаем
    m_yValuesComplex.squeeze();

    recalculateYValues(); // нужно, чтобы подцепился указатель на данные для построения кривых
    recalculateMinMax();
}

bool DataHolder::setYValue(int index, double value, int block)
{
    Q_UNUSED(block);

    index += block * m_xCount;
    if (index < 0 || index >= m_xCount * m_zCount) return false;

    if (!m_yValuesTemporal.isEmpty() && index >= m_yValuesTemporal.size()) return false;
    if (!m_yValues.isEmpty() && index >= m_yValues.size()) return false;
    if (!m_yValuesComplex.isEmpty() && index >= m_yValuesComplex.size()) return false;

    if (qFuzzyIsNull(m_yValuesTemporal.at(index) - value)) return false;

    if (int(m_yValuesFormat) == int(m_yValuesPresentation)) {
        m_yValues[index] = value;
        m_yValuesTemporal = m_yValues;
        return true;
    }

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

void DataHolder::setYValues(const QVector<cx_double> &values, int block)
{DD;
    m_yValuesFormat = YValuesComplex;
    if (m_yValuesPresentation == ShowAsDefault) { //еще не задан формат отображения данных
        m_yValuesPresentation = ShowAsAmplitudesInDB;
    }

    if (block == -1) {
        //весь канал
        m_yValuesComplex = values;
    }
    else {
        if (m_yValuesComplex.size() < m_zCount*m_xCount) m_yValuesComplex.resize(m_zCount*m_xCount);
        for (int i=0; i<values.size(); ++i)
            m_yValuesComplex[block*m_xCount + i] = values.at(i);
    }
    m_yValues.clear(); // действительные значения не нужны, очищаем
    m_yValues.squeeze();


    recalculateYValues(); // заполняем вектор действительных значений,
                          // который будет использоваться для графиков
    recalculateMinMax();
}

void DataHolder::setXValues(const QVector<double> &values)
{DD;
    m_xValues = values;
    m_xCount = values.size();
    m_xBegin = values.isEmpty() ? 0.0 : values.constFirst();
    m_xStep = 0.0;
    m_xValuesFormat = XValuesNonUniform;
}

void DataHolder::setXValues(double xBegin, double xStep, int count)
{DD;
    m_xBegin = xBegin;
    m_xStep = xStep;
    m_xCount = count;
    m_xValuesFormat = XValuesUniform;
    m_xValues.clear();
    m_xValues.squeeze();
}

void DataHolder::setZValues(const QVector<double> &values)
{
    m_zValues = values;
    m_zCount = values.size();
    m_zBegin = values.isEmpty() ? 0.0 : values.constFirst();
    m_zStep = 0.0;
    m_zValuesFormat = XValuesNonUniform;
}

void DataHolder::setZValues(double zBegin, double zStep, int count)
{
    m_zBegin = zBegin;
    m_zStep = zStep;
    m_zCount = count;
    m_zValuesFormat = XValuesUniform;
    m_zValues.clear();
    m_zValues.squeeze();
}

void DataHolder::setXStep(const double xStep)
{DD;
    if (m_xValuesFormat == XValuesUniform) {
        m_xStep = xStep;
    }
}

void DataHolder::setSamplesCount(const int samplesCount)
{
    m_xCount = samplesCount;
}

QVector<double> DataHolder::rawYValues(int block)
{
    if (block == -1) return m_yValues;
    return m_yValues.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::yValues(int block) const
{
    if (block == -1) return m_yValuesTemporal;
    return m_yValuesTemporal.mid(block*m_xCount, m_xCount);
}

QVector<cx_double> DataHolder::yValuesComplex(int block)
{
    if (block == -1) return m_yValuesComplex;
    return m_yValuesComplex.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::xValues() const
{DD;
    if (xValuesFormat() == XValuesUniform) return linspace(m_xBegin, m_xStep, m_xCount);

    return m_xValues;
}

QVector<double> DataHolder::zValues() const
{
    if (m_zValuesFormat == XValuesUniform) return linspace(m_zBegin, m_zStep, m_zCount);

    return m_zValues;
}

double DataHolder::xValue(int i) const
{
    if (xValuesFormat() != XValuesNonUniform) return m_xBegin + m_xStep * i;

    if (i<m_xValues.size()) return m_xValues[i];
    return 0.0;
}

double DataHolder::yValue(int i, int block) const
{
    i += block*m_xCount;

    if (i < m_yValuesTemporal.size())
        return corrected(m_yValuesTemporal.at(i));
    return 0.0;
}

cx_double DataHolder::yValueComplex(int i, int block) const
{
    i += block*m_xCount;

    if (i<m_yValuesComplex.size())  return m_yValuesComplex.at(i);
    return {0.0,0.0};
}

double DataHolder::zValue(int i) const
{
    if (m_zValuesFormat != XValuesNonUniform) return m_zBegin + m_zStep * i;

    if (i<m_zValues.size()) return m_zValues.at(i);
    return 0.0;
}

double DataHolder::YforXandZ(double x, double z, bool &success) const
{
    int zIndex = -1;
    if (m_zValuesFormat == XValuesUniform) {
        zIndex = qFloor((z-m_zBegin)/m_zStep);
    }
    else for (zIndex = 0; zIndex < m_zValues.size(); ++zIndex) {
        if (m_zValues.at(zIndex) >= z) break;
    }
    zIndex--;
    if (zIndex < 0 || zIndex >=m_zCount) {
        success = false;
        return 0;
    }

    int xIndex = -1;
    if (m_xValuesFormat == XValuesUniform) {
        xIndex = qFloor((x-m_xBegin)/m_xStep);
    }
    else for (xIndex = 0; xIndex < m_xValues.size(); ++xIndex) {
        if (m_xValues.at(xIndex) >= x) break;
    }
    xIndex--;
    if (xIndex < 0 || xIndex >=m_xCount) {
        success = false;
        return 0;
    }

    success = true;
    return yValue(xIndex, zIndex);
}

double DataHolder::xMin() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xBegin;
    if (xValuesFormat() == XValuesNonUniform && !m_xValues.isEmpty()) return m_xValues.constFirst();
    return 0;
}

double DataHolder::xMax() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xBegin + m_xStep*(m_xCount-1);
    if (xValuesFormat() == XValuesNonUniform && !m_xValues.isEmpty()) return m_xValues.last();
    return 0.0;
}

double DataHolder::xStep() const
{DD;
    if (xValuesFormat() == XValuesUniform) return m_xStep;

    return 0.0;
}

double DataHolder::zMin() const
{
    if (m_zValuesFormat == XValuesUniform) return m_zBegin;
    if (m_zValuesFormat == XValuesNonUniform && !m_zValues.isEmpty()) return m_zValues.constFirst();
    return 0.0;
}

double DataHolder::zMax() const
{
    if (m_zValuesFormat == XValuesUniform) return m_zBegin + m_zStep*(m_zCount-1);
    if (m_zValuesFormat == XValuesNonUniform && !m_zValues.isEmpty()) return m_zValues.last();
    return 0.0;
}

double DataHolder::zStep() const
{
    if (m_zValuesFormat == XValuesUniform) return m_zStep;
    return 0.0;
}

double DataHolder::yMin(int block) const
{DD;
    if (block == -1) {
        double min = *std::min_element(m_yMin.constBegin(), m_yMin.constEnd());
        return corrected(min);
    }
    return corrected(m_yMin.value(block, 0.0));
}

double DataHolder::yMax(int block) const
{DD;
    if (block == -1) {
        double max = *std::max_element(m_yMax.constBegin(), m_yMax.constEnd());
        return corrected(max);
    }
    return corrected(m_yMax.value(block, 0.0));
}

int DataHolder::samplesCount() const
{DD;
    return m_xCount;
}

QVector<double> DataHolder::linears(int block) const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex: return absolutes(m_yValuesComplex.mid(block*m_xCount, m_xCount));
        case YValuesAmplitudesInDB: return fromLog(m_yValues.mid(block*m_xCount, m_xCount),
                                                   m_threshold, m_yValuesUnits);
        default: break;
    }
    return m_yValues.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::decibels(int block) const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex: return toLog(absolutes(m_yValuesComplex.mid(block*m_xCount, m_xCount)),
                                          m_threshold, m_yValuesUnits);
        case YValuesAmplitudesInDB: return m_yValues.mid(block*m_xCount, m_xCount);
        default: break;
    }
    return toLog(m_yValues.mid(block*m_xCount, m_xCount), m_threshold, m_yValuesUnits);
}

QVector<double> DataHolder::toLog(const QVector<double> &values, double threshold, int units)
{DD;
    if (qFuzzyIsNull(threshold) || units == UnitsDimensionless) return values;
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
    if (threshold == 0 || units == UnitsDimensionless) return values;
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
    if (threshold == 0 || units == UnitsDimensionless) return value;

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
    if (threshold == 0 || units == UnitsDimensionless) return value;

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
    m_yMin.clear();
    m_yMax.clear();

//    if (!m_yValuesTemporal.isEmpty()) {
//        auto begin = m_yValuesTemporal.cbegin();
//        auto end = m_yValuesTemporal.cbegin() + m_xCount;
//        for (int block = 0; block < m_zCount; ++block) {
//            auto minmax = std::minmax_element(begin, end);
//            m_yMin << *(minmax.first);
//            m_yMax << *(minmax.second);
//            begin += m_xCount;
//            end += m_xCount;
//        }
//    }

    if (!m_yValuesTemporal.isEmpty()) {

        for (int block = 0; block < m_zCount; ++block) {
            double min = 0.0; double max = 0.0;
            for (int i=0; i<m_xCount; ++i) {
                int index = i+block*m_xCount;
                if (index >= m_yValuesTemporal.size()) break;

                if (m_yValuesTemporal.at(index) < min) min = m_yValuesTemporal.at(index);
                if (m_yValuesTemporal.at(index) > max) max = m_yValuesTemporal.at(index);
            }

            m_yMin << min;
            m_yMax << max;
        }
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

    if (m_xCount > 0 && m_yValuesTemporal.isEmpty())
        m_yValuesTemporal = QVector<double>(m_xCount*m_zCount, 0.0);
//    m_yValuesTemporal.squeeze();
}

double correctedByType(double val, int type, double correction)
{
    if (type == 0) return val + correction;
    if (type == 1) return val * correction;
    return val;
}

double DataHolder::corrected(double val) const
{
    if (m_correction)
        return correctedByType(val, m_correctionType, m_correctionValue);
    return val;
}

bool DataHolder::hasCorrection() const
{
    if (m_correctionType == 0 && !qFuzzyIsNull(m_correctionValue) ) return true;
    if (m_correctionType == 1 && !qFuzzyIsNull(m_correctionValue - 1.0) ) return true;
    return false;
}
