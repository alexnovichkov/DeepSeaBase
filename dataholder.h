#ifndef DATAHOLDER_H
#define DATAHOLDER_H

#include <QtCore>

#include "algorithms.h"

/**
 * @brief The DataHolder class
 *
 * Использование:
 *   1. Объявление экземпляра класса
 *   2. Задание значений по осям X и Y
 *   3. Изменение формата отображения значений по оси Y, если необходимо
 */

class DataHolder
{
public:
    enum XValuesFormat {
        XValuesUnknown = -1,
        XValuesUniform = 0,
        XValuesNonUniform = 1
    };

    enum YValuesFormat {
        YValuesUnknown = -1,
        YValuesComplex = 0,
        YValuesReals = 1,
        YValuesImags = 2,
        YValuesAmplitudes = 3,
        YValuesAmplitudesInDB = 4,
        YValuesPhases = 5
    };

    enum YValuesPresentation {
        ShowAsDefault = -1,
        ShowAsReals = 1,
        ShowAsImags = 2,
        ShowAsAmplitudes = 3,
        ShowAsAmplitudesInDB = 4,
        ShowAsPhases = 5
    };

    DataHolder();
    DataHolder(const DataHolder &other);

    void setSegment(const DataHolder &other, int from, int to);

    void clear();

    void setCorrection(double correctionValue);

    int xValuesFormat() const {return m_xValuesFormat;} // не меняется, так как зависит только от формата данных в файле
    int yValuesFormat() const {return m_initially;} // не меняется, так как зависит только от формата данных в файле

    int yValuesPresentation() const {return m_yValuesPresentation;}
    void setYValuesPresentation(YValuesPresentation presentation);

    void setYValues(const QVector<double> &values, YValuesFormat initially);
    void setYValues(const QVector<cx_double> &values);
    void setXValues(const QVector<double> &values);
    void setXValues(double xBegin, double xStep, int count);
    void setXStep(const double xStep);
    void setSamplesCount(const int samplesCount);

    void setThreshold(double threshold) {m_threshold = threshold;}

    const double* rawXValues() const {return m_xValues.data();}
    const double *rawYValues() const {return m_yValuesTemporal.data();}
    QVector<double> yValues() const {return m_yValuesTemporal;}
    QVector<cx_double> yValuesComplex() const {return m_yValuesComplex;}
    QVector<double> xValues() const;

    double xValue(int i) const;
    double yValue(int i) const;
    cx_double yValueComplex(int i) const;

    double xMin() const;
    double xMax() const;
    double xStep() const;
    double yMin() const;
    double yMax() const;
    int samplesCount() const;

    QVector<double> linears() const;
    QVector<double> decibels() const;

    static QVector<double> toLog(const QVector<double> &values, double threshold);
    static QVector<double> fromLog(const QVector<double> &values, double threshold);
private:
    void recalculateMinMax();
    void recalculateYValues();

    QVector<double> m_yValues;
    QVector<cx_double> m_yValuesComplex;
    QVector<double> m_xValues;
    QVector<double> m_yValuesTemporal;

    double m_xBegin;
    double m_xStep;
    int m_count;

    double m_threshold;

    XValuesFormat m_xValuesFormat;
    YValuesFormat m_initially;

    YValuesPresentation m_yValuesPresentation;

    //some statistics
    double m_yMin, m_yMax;

    double correctionValue;
//    double oldCorrectionValue;
};

#endif // DATAHOLDER_H
