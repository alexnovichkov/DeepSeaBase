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
    static YValuesFormat formatFromString(const QString &format);
    static QString formatToString(YValuesFormat format);

    enum YValuesPresentation {
        ShowAsAmplitudes = 3,
        ShowAsAmplitudesInDB = 4,
        ShowAsPhases = 5,
        ShowAsReals = 1,
        ShowAsImags = 2,
        ShowAsDefault = -1,
    };

    enum YValuesUnits {
        UnitsUnknown = 0, //dB = 20lg(L/L0)
        UnitsLinear = 1, //dB = 20lg(L/L0)
        UnitsQuadratic = 2, //dB = 10lg(L/L0^2)
        UnitsDimensionless = 3 //dB = L
    };
    static YValuesUnits unitsFromString(const QString &format);
    static QString unitsToString(YValuesUnits units);

    DataHolder();
    DataHolder(const DataHolder &other);

    void setSegment(const DataHolder &other, int from, int to);

    void clear();

    void setTemporaryCorrection(double m_correctionValue, int type);
    void removeCorrection();
    bool makeCorrectionConstant();
    QString correctionString() const;
    static QString correctionString(double value, int type);
    inline double correction() const {return m_correctionValue;}
    bool correctionIsValid() const;
    inline bool hasCorrection() const {return m_correction;}


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

    void setYValuesUnits(YValuesUnits yValuesUnits) {m_yValuesUnits = yValuesUnits;}
    YValuesUnits yValuesUnits() const {return m_yValuesUnits;}

    QVector<double> rawYValues(int block) const;
    QVector<double> yValues(int block) const;
    QVector<cx_double> yValuesComplex(int block) const;
    QVector<double> xValues() const;
    QVector<double> zValues() const;

    QPair<double,double> yMinMax(int begin, int end);

    double xValue(int i) const;
    double yValue(int i, int block = 0) const;
    double yValueRaw(int i, int block = 0) const;
    //возвращает максимальный индекс i, для которого xValue(i) <= x, или -1
    int floor(double x) const;
    //возвращает минимальный индекс i, для которого xValue(i) >= x, или -1
    int ceil(double x) const;
    //возвращает индекс i, для которого |xValue(i)-x| минимальна, или -1
    int nearest(double x) const;

    //возвращает максимальный индекс i, для которого zValue(i) <= z, или -1
    int floorZ(double z) const;
    //возвращает минимальный индекс i, для которого zValue(i) >= z, или -1
    int ceilZ(double z) const;
    //возвращает индекс i, для которого |zValue(i)-z| минимальна, или -1
    int nearestZ(double z) const;

    cx_double yValueComplex(int i, int block = 0) const;
    double zValue(int i) const;
    double YforXandZ(double x, double z, bool &success) const;



    double xMin() const;
    double xMax() const;
    double xStep() const;

    double zMin() const;
    double zMax() const;
    double zStep() const;

    double yMin(int block = 0) const;
    double yMax(int block = 0) const;

    inline int samplesCount() const {
        return m_xCount;
    }
    inline int blocksCount() const {return m_zCount;}
    inline void setBlocksCount(int count) {m_zCount = count;}

    QVector<double> linears(int block = -1) const;
    QVector<double> decibels(int block = -1) const;

    QByteArray wavData(qint64 pos, qint64 samples, int format);

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
    QVector<double> m_yMin, m_yMax;

    double m_correctionValue;
    int m_correctionType;
    YValuesPresentation m_PresentationWhenCorrecting;
    bool m_correction;
};

#endif // DATAHOLDER_H
