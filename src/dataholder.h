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

    enum YValuesUnits {
        UnitsUnknown = 0, //dB = 20lg(L/L0)
        UnitsLinear = 1, //dB = 20lg(L/L0)
        UnitsQuadratic = 2, //dB = 10lg(L/L0^2)
        UnitsDimensionless = 3 //dB = L
    };

    DataHolder();
    DataHolder(const DataHolder &other);

    void setSegment(const DataHolder &other, int from, int to);

    void clear();


    void setTemporaryCorrection(double m_correctionValue, int type);
    void removeCorrection();
    bool makeCorrectionConstant();
    QString correctionString() const;
    static QString correctionString(double value, int type);
    double correction() const {return m_correctionValue;}
    bool hasCorrection() const;


    int xValuesFormat() const {return m_xValuesFormat;} // не меняется, так как зависит только от формата данных в файле
    int zValuesFormat() const {return m_zValuesFormat;}
    YValuesFormat yValuesFormat() const {return m_yValuesFormat;} // не меняется, так как зависит только от формата данных в файле
    void setYValuesFormat(YValuesFormat format) {m_yValuesFormat = format;}
    QString yValuesFormatString() const;

    int yValuesPresentation() const {return m_yValuesPresentation;}
    void setYValuesPresentation(int presentation);

    void setYValues(const QVector<double> &values, YValuesFormat initially, int block = 0);
    bool setYValue(int index, double value, int block = 0);
    void setYValues(const QVector<cx_double> &values, int block = 0);

    void setXValues(const QVector<double> &values);
    void setXValues(double xBegin, double xStep, int count);

    void setZValues(const QVector<double> &values);
    void setZValues(double zBegin, double zStep, int count);

    void setXStep(const double xStep);
    void setSamplesCount(const int samplesCount);

    void setThreshold(double threshold) {m_threshold = threshold;}
    double threshold() const {return m_threshold;}

    void setYValuesUnits(int yValuesUnits) {m_yValuesUnits = YValuesUnits(yValuesUnits);}
    int yValuesUnits() const {return m_yValuesUnits;}

    const double* rawXValues() const {return m_xValues.data();}
    QVector<double> rawYValues(int block = 0) const {return m_yValues;}
    QVector<double> yValues(int block=0) const;
    QVector<cx_double> yValuesComplex(int block=0) const {return m_yValuesComplex;}
    QVector<double> xValues() const;
    QVector<double> zValues() const;

    double xValue(int i) const;
    double yValue(int i, int block = 0) const;
    cx_double yValueComplex(int i, int block = 0) const;
    double zValue(int i) const;



    double xMin() const;
    double xMax() const;
    double xStep() const;

    double zMin() const;
    double zMax() const;
    double zStep() const;

    double yMin(int block = 0) const;
    double yMax(int block = 0) const;

    int samplesCount() const;
    int blocksCount() const {return m_zCount;}
    void setBlocksCount(int count) {m_zCount = count;}

    QVector<double> linears(int block = 0) const;
    QVector<double> decibels(int block = 0) const;

    static QVector<double> toLog(const QVector<double> &values, double threshold, int units);
    static QVector<double> fromLog(const QVector<double> &values, double threshold, int units);
    static double toLog(double value, double threshold, int units);
    static double fromLog(double value, double threshold, int units);
private:
    void recalculateMinMax();
    void recalculateYValues();
    double corrected(double val) const;


    QVector<double> m_yValues;
    QVector<cx_double> m_yValuesComplex;
    QVector<double> m_xValues;
    QVector<double> m_yValuesTemporal;
    QVector<double> m_zValues;

    double m_xBegin = 0.0;
    double m_xStep = 0.0;
    double m_zBegin = 0.0;
    double m_zStep = 0.0;
    int m_xCount = 0.0;
    int m_zCount = 1;

    double m_threshold;

    XValuesFormat m_xValuesFormat;
    XValuesFormat m_zValuesFormat;
    YValuesFormat m_yValuesFormat;
    YValuesUnits m_yValuesUnits;


    YValuesPresentation m_yValuesPresentation;

    //some statistics
    double m_yMin, m_yMax;

    double m_correctionValue;
    int m_correctionType;
    YValuesPresentation m_PresentationWhenCorrecting;
    bool m_correction;
};

#endif // DATAHOLDER_H
