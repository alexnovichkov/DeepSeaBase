#include "dataholder.h"
#include "logging.h"
#include <QMessageBox>
#include "settings.h"

DataHolder::YValuesFormat DataHolder::formatFromString(const QString &format)
{DD;
    if (format == "complex") return YValuesComplex;
    else if (format == "amplitude") return YValuesAmplitudes;
    else if (format.toLower() == "amplitudedb") return YValuesAmplitudesInDB;
    else if (format == "phase") return YValuesPhases;
    else if (format == "real") return YValuesReals;
    else if (format == "imaginary") return YValuesImags;

    return YValuesUnknown;
}

QString DataHolder::formatToString(DataHolder::YValuesFormat format)
{DD;
    switch (format) {
    case YValuesComplex: return "complex";
    case YValuesAmplitudes: return "amplitude";
    case YValuesAmplitudesInDB: return "amplitudeDb";
    case YValuesPhases: return "phase";
    case YValuesReals: return "real";
    case YValuesImags: return "imaginary";
    default: break;
    }
    return "unknown";
}

DataHolder::YValuesUnits DataHolder::unitsFromString(const QString &format)
{DD;
    if (format == "unknown") return DataHolder::UnitsUnknown;
    if (format == "linear") return DataHolder::UnitsLinear;
    if (format == "quadratic") return DataHolder::UnitsQuadratic;
    if (format == "dimensionless") return DataHolder::UnitsDimensionless;
    return DataHolder::UnitsUnknown;
}

QString DataHolder::unitsToString(DataHolder::YValuesUnits units)
{DD;
    switch (units) {
        case DataHolder::UnitsUnknown: return "unknown";
        case DataHolder::UnitsLinear: return "linear";
        case DataHolder::UnitsQuadratic: return "quadratic";
        case DataHolder::UnitsDimensionless: return "dimensionless";
        default: break;
    }
    return "unknown";
}

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

void DataHolder::setSegment(const DataHolder &other, int from, int to)
{DD;
    m_xBegin = other.m_xBegin + other.m_xStep * from;
    m_xStep = other.m_xStep;
    m_zBegin = other.m_zBegin;
    m_zStep = other.m_zStep;
    m_xCount = to - from + 1;
    m_zCount = other.m_zCount;
    m_threshold = other.m_threshold;
    m_xValues = segment(other.m_xValues, from, to, other.m_xCount, m_zCount);
    m_yValues = segment(other.m_yValues, from, to, other.m_xCount, m_zCount);
    m_zValues = other.m_zValues;
    m_yValuesTemporal = segment(other.m_yValuesTemporal, from, to, other.m_xCount, m_zCount);
    m_yValuesComplex  = segment(other.m_yValuesComplex, from, to, other.m_xCount, m_zCount);
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
{DD;
    if (m_correctionType == 0) m_correctionValue = 0.0;
    if (m_correctionType == 1) m_correctionValue = 1.0;
    m_PresentationWhenCorrecting = ShowAsDefault;
    m_correction = false;
}

bool DataHolder::makeCorrectionConstant()
{DD;
    if (!hasCorrection()) return true; // есть временная коррекция
    if (!correctionIsValid()) return true; // и эта коррекция не тривиальная

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
                            double abs_ = std::abs(m_yValuesComplex.at(i));
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
                                m_yValues[i] = thr * pow(m_yValues.at(i), m_correctionValue);
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
                            double abs_ = std::abs(m_yValuesComplex.at(i));
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
                            if (abs(m_yValues.at(i)) + m_correctionValue < 0.0)
                                m_yValues[i] = 0.0;
                            else if (m_yValues.at(i) >= 0.0)
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
                                m_yValues[i] = 20.0 * log10((pow(10.0, m_yValues.at(i)/20.0)*m_threshold+m_correctionValue)/m_threshold);
                            else if (m_yValuesUnits == UnitsQuadratic)
                                m_yValues[i] = 10.0 * log10((pow(10.0, m_yValues.at(i)/10.0)*m_threshold*m_threshold
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
                            m_yValuesComplex[i] = {m_yValuesComplex.at(i).real() + m_correctionValue,
                                                   m_yValuesComplex.at(i).imag()};
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex.at(i).real() * m_correctionValue,
                                                   m_yValuesComplex.at(i).imag()};
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
                            m_yValuesComplex[i] = {m_yValuesComplex.at(i).real(),
                                                   m_yValuesComplex.at(i).imag() + m_correctionValue};
                        }
                    }
                    else if (m_correctionType == 1) {// множитель
                        for (int i=0; i<m_yValuesComplex.size(); ++i) {
                            m_yValuesComplex[i] = {m_yValuesComplex.at(i).real(),
                                                   m_yValuesComplex.at(i).imag() * m_correctionValue};
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
    m_correction = false; //временная коррекция заменена постоянной
    return true;
}



QString DataHolder::correctionString() const
{DD;
    return correctionString(m_correctionValue, m_correctionType);
}

QString DataHolder::correctionString(double value, int type) /*static*/
{DD;
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
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex: return "[Cmplx]"; break;
        case YValuesReals: return "[Re]"; break;
        case YValuesImags: return "[Im]"; break;
        case YValuesAmplitudes: return "[Abs]"; break;
        case YValuesAmplitudesInDB: return "[dB]"; break;
        case YValuesPhases: return "[Phase]"; break;
        default: break;
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
        try {
            m_yValues = values;
        }
        catch (const std::bad_alloc& bad) {
            LOG(DEBUG)<<"Exception at setting YValues: " << bad.what();
            QMessageBox::critical(nullptr, "Чтение данных", "Не удалось выделить память под данные канала");
            return;
        }

    }
    else {
        try {
            if (m_yValues.size() < m_zCount*m_xCount) m_yValues.resize(m_zCount*m_xCount);
            for (int i=0; i<values.size(); ++i)
                m_yValues[block*m_xCount + i] = values.at(i);
        }
        catch (const std::bad_alloc& bad) {
            LOG(DEBUG)<<"Exception at setting YValues block: " << bad.what();
            QMessageBox::critical(nullptr, "Чтение данных", "Не удалось выделить память под данные канала");
            return;
        }
    }

    m_yValuesComplex.clear(); // комплексные значения не нужны, очищаем
    m_yValuesComplex.squeeze();

    try {
        recalculateYValues(); // нужно, чтобы подцепился указатель на данные для построения кривых
    }
    catch (const std::bad_alloc& bad) {
        LOG(DEBUG)<<"Exception at recalculating YValues: " << bad.what();
        QMessageBox::critical(nullptr, "Чтение данных", "Не удалось выделить память под данные канала");
        return;
    }

    try {
        recalculateMinMax();
    }
    catch (const std::bad_alloc& bad) {
        LOG(DEBUG)<<"Exception at recalculating min & max: " << bad.what();
        QMessageBox::critical(nullptr, "Чтение данных", "Не удалось выделить память под данные канала");
    }
}

bool DataHolder::setYValue(int index, double value, int block)
{DD;
    if (block < 0) return false;
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
                m_yValuesComplex[index] = std::polar(value, std::arg(m_yValuesComplex.at(index)));
                return true;
            case ShowAsAmplitudes:
                m_yValuesComplex[index] = std::polar(value, std::arg(m_yValuesComplex.at(index)));
                return true;
            case ShowAsImags:
                m_yValuesComplex[index] = {m_yValuesComplex.at(index).real(), value};
                return true;
            case ShowAsPhases: {
                bool radians = se->getSetting("phaseType").toInt() == 0;
                m_yValuesComplex[index] = std::polar(std::abs(m_yValuesComplex.at(index)), radians ? value : value * M_PI / 180.0);
                return true;
            }
            case ShowAsReals:
                m_yValuesComplex[index] = {value, m_yValuesComplex.at(index).imag()};
                return true;
            default: return false;
        };
    }
    else {
        switch (m_yValuesFormat) {
            case YValuesReals: {
                if (m_yValuesPresentation == ShowAsAmplitudes) {
                    if (value < 0) return false;
                    if (m_yValues.at(index) >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else if (m_yValuesPresentation == ShowAsAmplitudesInDB) {
                    value = fromLog(value, m_threshold, m_yValuesUnits);
                    if (m_yValues.at(index) >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else return false;
            }
            case YValuesImags: {
                if (m_yValuesPresentation == ShowAsAmplitudes) {
                    if (value < 0) return false;
                    if (m_yValues.at(index) >= 0) m_yValues[index] = value;
                    else m_yValues[index] = -1.0 * value;
                    m_yValuesTemporal[index] = value;
                    return true;
                }
                else if (m_yValuesPresentation == ShowAsAmplitudesInDB) {
                    value = fromLog(value, m_threshold, m_yValuesUnits);
                    if (m_yValues.at(index) >= 0) m_yValues[index] = value;
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
{DD;
    m_zValues = values;
    m_zCount = values.size();
    m_zBegin = values.isEmpty() ? 0.0 : values.constFirst();
    m_zStep = 0.0;
    m_zValuesFormat = XValuesNonUniform;
}

void DataHolder::setZValues(double zBegin, double zStep, int count)
{DD;
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
{DD;
    m_xCount = samplesCount;
}

QVector<double> DataHolder::rawYValues(int block) const
{DD;
    if (block == -1) return m_yValues;
    return m_yValues.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::yValues(int block) const
{DD;
    if (block == -1) return m_yValuesTemporal;
    return m_yValuesTemporal.mid(block*m_xCount, m_xCount);
}

QVector<cx_double> DataHolder::yValuesComplex(int block) const
{DD;
    if (block == -1) return m_yValuesComplex;
    return m_yValuesComplex.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::xValues() const
{DD;
    if (xValuesFormat() == XValuesUniform) return linspace(m_xBegin, m_xStep, m_xCount);

    return m_xValues;
}

QVector<double> DataHolder::zValues() const
{DD;
    if (m_zValuesFormat == XValuesUniform) return linspace(m_zBegin, m_zStep, m_zCount);

    return m_zValues;
}

QPair<double, double> DataHolder::yMinMax(int begin, int end)
{DD;
    begin = std::clamp(begin, 0, samplesCount()-1);
    end = std::clamp(end, 0, samplesCount()-1);

    if (begin == 0 && end == samplesCount()-1) return {yMin(0), yMax(0)};

    double min = yValue(begin, 0);
    double max = yValue(begin, 0);
    for (int i=begin+1; i<=end; ++i) {
        if (auto y = yValue(i); y < min) min = y;
        if (auto y = yValue(i); y > max) max = y;
    }
    return {min, max};
}

double DataHolder::xValue(int i) const
{DD;
    if (i<0) return 0.0;
    if (xValuesFormat() != XValuesNonUniform) return m_xBegin + m_xStep * i;

    if (i<m_xValues.size()) return m_xValues.at(i);
    return 0.0;
}

double DataHolder::yValue(int i, int block) const
{DD;
    if (i<0 || block < 0) return 0.0;
    i += block*m_xCount;

    if (i < m_yValuesTemporal.size())
        return corrected(m_yValuesTemporal.at(i));
    return 0.0;
}

double DataHolder::yValueRaw(int i, int block) const
{DD;
    if (i<0 || block < 0) return 0.0;
    i += block*m_xCount;

    if (i < m_yValues.size())
        return corrected(m_yValues.at(i));
    return 0.0;
}

int DataHolder::floor(double x) const
{DD;
    int index=-1;
    if (m_xValuesFormat == XValuesNonUniform) {
        for (int i = 0; i < m_xCount; ++i) {
            if (m_xValues[i]<=x) index=i;
            else break;
        }
    }
    else {
        index = qFuzzyIsNull(m_xStep) ? -1 : qFloor((x-m_xBegin)/m_xStep);
        if (index >= m_xCount) index = -1;
    }
    return index;
}

int DataHolder::ceil(double x) const
{DD;
    int index=-1;
    if (m_xValuesFormat == XValuesNonUniform) {
        for (int i = 0; i < m_xCount; ++i) {
            if (m_xValues[i]<=x) index=i;
            else {
                index = i+1;
                break;
            }
        }
    }
    else {
        index = qFuzzyIsNull(m_xStep) ? -1 : qCeil((x-m_xBegin)/m_xStep);
        if (index >= m_xCount) index = -1;
    }
    return index;
}

int DataHolder::nearest(double x) const
{DD;
    int index=-1;
    if (m_xValuesFormat == XValuesNonUniform) {

        auto idx = std::min_element(m_xValues.constBegin(), m_xValues.constEnd(),
                                    [x](double a, double b){
            return std::abs(a-x) < std::abs(b-x);
        });
        if (idx == m_xValues.constEnd()) return -1;
        return std::distance(m_xValues.constBegin(), idx);
//        auto upper = std::upper_bound(m_xValues.constBegin(), m_xValues.constEnd(), x);
//        if (upper == m_xValues.constEnd()) return -1; //larger than xmax
//        if (upper == m_xValues.constBegin()) return -1; //smaller than xmin
//        index = std::distance(m_xValues.constBegin(), upper)-1;
//        if (x-m_xValues[index] > m_xValues[index+1]-x) index++;
    }
    else {
        index = qFuzzyIsNull(m_xStep) ? -1 : qRound((x-m_xBegin)/m_xStep);
        if (index >= m_xCount) index = -1;
    }
    return index;
}

int DataHolder::floorZ(double z) const
{DD;
    int index=-1;
    if (m_zValuesFormat == XValuesNonUniform) {
        for (int i = 0; i < m_zCount; ++i) {
            if (m_zValues[i]<=z) index=i;
            else break;
        }
    }
    else {
        index = qFuzzyIsNull(m_zStep) ? -1 : qFloor((z-m_zBegin)/m_zStep);
        if (index >= m_zCount) index = -1;
    }
    return index;
}

int DataHolder::ceilZ(double z) const
{DD;
    int index=-1;
    if (m_zValuesFormat == XValuesNonUniform) {
        for (int i = 0; i < m_zCount; ++i) {
            if (m_zValues[i]<=z) index=i;
            else {
                index = i+1;
                break;
            }
        }
    }
    else {
        index = qFuzzyIsNull(m_zStep) ? -1 : qCeil((z-m_zBegin)/m_zStep);
        if (index >= m_zCount) index = -1;
    }
    return index;
}

int DataHolder::nearestZ(double z) const
{DD;
    int index=-1;
    if (m_zValuesFormat == XValuesNonUniform) {
        auto upper = std::upper_bound(m_zValues.constBegin(), m_zValues.constEnd(), z);
        if (upper == m_zValues.constEnd()) return -1; //larger than zmax
        if (upper == m_zValues.constBegin()) return -1; //smaller than zmin
        index = std::distance(m_zValues.constBegin(), upper)-1;
        if (z-m_zValues[index] > m_zValues[index+1]-z) index++;
    }
    else {
        index = qFuzzyIsNull(m_zStep) ? -1 : qRound((z-m_zBegin)/m_zStep);
        if (index >= m_zCount) index = -1;
    }
    return index;
}

cx_double DataHolder::yValueComplex(int i, int block) const
{DD;
    if (i<0 || block < 0) return {0.0,0.0};
    i += block*m_xCount;

    if (i<m_yValuesComplex.size())  return m_yValuesComplex.at(i);
    return {0.0,0.0};
}

double DataHolder::zValue(int i) const
{DD;
    if (i<0) return 0.0;
    if (m_zValuesFormat != XValuesNonUniform) return m_zBegin + m_zStep * i;

    if (i<m_zValues.size()) return m_zValues.at(i);
    return 0.0;
}

double DataHolder::YforXandZ(double x, double z, bool &success) const
{DD;
    int xIndex = nearest(x);
    if (xIndex < 0 || xIndex >= m_xCount) {
        success = false;
        return 0;
    }

    if (m_zCount >= 2) {
        int zIndex = nearestZ(z);
        if (zIndex < 0 || zIndex >= m_zCount) {
            success = false;
            return 0;
        }
        success = true;
        return yValue(xIndex, zIndex);
    }

    success = true;
    return yValue(xIndex);
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
{DD;
    if (m_zValuesFormat == XValuesUniform) return m_zBegin;
    if (m_zValuesFormat == XValuesNonUniform && !m_zValues.isEmpty()) return m_zValues.constFirst();
    return 0.0;
}

double DataHolder::zMax() const
{DD;
    if (m_zValuesFormat == XValuesUniform) return m_zBegin + m_zStep*(m_zCount-1);
    if (m_zValuesFormat == XValuesNonUniform && !m_zValues.isEmpty()) return m_zValues.last();
    return 0.0;
}

double DataHolder::zStep() const
{DD;
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

QVector<double> DataHolder::linears(int block) const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex:
            return block==-1 ? absolutes(m_yValuesComplex)
                             : absolutes(m_yValuesComplex.mid(block*m_xCount, m_xCount));
        case YValuesAmplitudesInDB:
            return block==-1 ? fromLog(m_yValues, m_threshold, m_yValuesUnits)
                             : fromLog(m_yValues.mid(block*m_xCount, m_xCount), m_threshold, m_yValuesUnits);
        default: break;
    }
    return block==-1 ? m_yValues : m_yValues.mid(block*m_xCount, m_xCount);
}

QVector<double> DataHolder::decibels(int block) const
{DD;
    switch (m_yValuesFormat) {
        case YValuesComplex:
            return block == -1 ? toLog(absolutes(m_yValuesComplex), m_threshold, m_yValuesUnits)
                               : toLog(absolutes(m_yValuesComplex.mid(block*m_xCount, m_xCount)),
                                       m_threshold, m_yValuesUnits);
        case YValuesAmplitudesInDB:
            return block == -1 ? m_yValues : m_yValues.mid(block*m_xCount, m_xCount);
        default: break;
    }
    return block == -1 ? toLog(m_yValues, m_threshold, m_yValuesUnits)
                       : toLog(m_yValues.mid(block*m_xCount, m_xCount), m_threshold, m_yValuesUnits);
}

QByteArray DataHolder::wavData(qint64 pos, qint64 samples, int format)
{
    QByteArray b;

    QDataStream s(&b, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QVector<double> values = rawYValues(0).mid(pos,samples);

    //формат данных PCM - [-32768..32767]

    const double max = qMax(qAbs(yMax()), qAbs(yMin()));
    const double coef = 32768.0 / max;
    for (qint64 i=0; i<values.size(); ++i) {
        if (format == 1) {//format == 1 - PCM
            qint16 v = qint16(coef * values[i]);
            s << v;
        }
        else if (format == 2) {//format = 2 - float
            s << static_cast<float>(values[i]);
        }
    }
    return b;
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
        if (values.at(i) <= 0.0) a[i] = 0.0;
        else {
            a[i] = factor*log10(values.at(i)/thr);
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
        a[i] = thr*exp(values.at(i)*factor);
    }
    return a;
}

double DataHolder::toLog(double value, double threshold, int units)
{DD;
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
{DD;
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
            int index = block*m_xCount;
            if (index >= m_yValuesTemporal.size()) break;

            double min = m_yValuesTemporal.at(index);
            double max = m_yValuesTemporal.at(index);
            for (int i=1; i<m_xCount; ++i) {
                index = i+block*m_xCount;
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
    const bool deg = se->getSetting("phaseType").toInt() == 1;
    m_yValuesTemporal.clear();

    switch (m_yValuesFormat) {
        case YValuesComplex: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(absolutes(m_yValuesComplex), m_threshold, m_yValuesUnits); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = absolutes(m_yValuesComplex); break;
                case ShowAsImags:          m_yValuesTemporal = ::imags(m_yValuesComplex); break;
                case ShowAsPhases:         m_yValuesTemporal = ::phases(m_yValuesComplex, deg); break;
                case ShowAsReals:          m_yValuesTemporal = ::reals(m_yValuesComplex); break;
            };
            break;
        }
        case YValuesReals: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:        m_yValuesTemporal = m_yValues; break;
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold, m_yValuesUnits); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = absolutes(m_yValues); break;
                case ShowAsImags:          m_yValuesTemporal.clear(); break;
                case ShowAsPhases:         m_yValuesTemporal = ::phases(complexesFromReals(m_yValues), deg); break;
                case ShowAsReals:          m_yValuesTemporal = m_yValues; break;
            };
            break;
        }
        case YValuesImags: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:        m_yValuesTemporal = m_yValues; break;
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(absolutes(m_yValues), m_threshold, m_yValuesUnits); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = absolutes(m_yValues); break;
                case ShowAsImags:          m_yValuesTemporal = m_yValues; break;
                case ShowAsPhases:         m_yValuesTemporal = ::phases(complexesFromImags(m_yValues), deg); break;
                case ShowAsReals:          m_yValuesTemporal.clear(); break;
            };
            break;
        }
        case YValuesAmplitudes: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:        m_yValuesTemporal = m_yValues; break;
                case ShowAsAmplitudesInDB: m_yValuesTemporal = toLog(m_yValues, m_threshold, m_yValuesUnits); break;
                case ShowAsAmplitudes:     m_yValuesTemporal = m_yValues; break;
                case ShowAsImags:          m_yValuesTemporal.clear(); break;
                case ShowAsPhases:         m_yValuesTemporal.clear(); break;
                case ShowAsReals:          m_yValuesTemporal.clear(); break;
            };
            break;
        }
        case YValuesAmplitudesInDB: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:        m_yValuesTemporal = m_yValues; break;
                case ShowAsAmplitudesInDB: m_yValuesTemporal = m_yValues; break;
                case ShowAsAmplitudes:     m_yValuesTemporal = fromLog(m_yValues, m_threshold, m_yValuesUnits); break;
                case ShowAsImags:          m_yValuesTemporal.clear(); break;
                case ShowAsPhases:         m_yValuesTemporal.clear(); break;
                case ShowAsReals:          m_yValuesTemporal.clear(); break;
            };
            break;
        }
        case YValuesPhases: {
            switch (m_yValuesPresentation) {
                case ShowAsDefault:
                case ShowAsPhases:         m_yValuesTemporal = phasesFromPhases(m_yValues, deg); break;
                case ShowAsAmplitudesInDB: m_yValuesTemporal.clear(); break;
                case ShowAsAmplitudes:     m_yValuesTemporal.clear(); break;
                case ShowAsImags:          m_yValuesTemporal.clear(); break;
                case ShowAsReals:          m_yValuesTemporal.clear(); break;
            };
        }
        default:
            break;
    }

    if (m_xCount > 0 && m_yValuesTemporal.isEmpty())
        m_yValuesTemporal = QVector<double>(m_xCount*m_zCount, 0.0);
//    m_yValuesTemporal.squeeze();
}

double correctedByType(double val, int type, double correction)
{DD;
    if (type == 0) return val + correction;
    if (type == 1) return val * correction;
    return val;
}

double DataHolder::corrected(double val) const
{DD;
    if (m_correction)
        return correctedByType(val, m_correctionType, m_correctionValue);
    return val;
}

bool DataHolder::correctionIsValid() const
{DD;
    if (m_correctionType == 0 && !qFuzzyIsNull(m_correctionValue) ) return true;
    if (m_correctionType == 1 && !qFuzzyIsNull(m_correctionValue - 1.0) ) return true;
    return false;
}
