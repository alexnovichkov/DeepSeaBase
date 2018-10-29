#ifndef AVERAGING_H
#define AVERAGING_H

#include <QtCore>

#include "algorithms.h"

class Averaging
{
public:
    enum AveragingType
    {
        Unknown = -1,
        Linear = 0,
        Exponential = 1,
        PeakHold = 2,
        Energetic = 3
    };

    static QString averagingDescription(int avgType) {
        switch (avgType) {
            case 0: return "линейное";
            case 1: return "экспоненциальное";
            case 2: return "хранение максимума";
            case 3: return "энергетическое";
        }
        return "";
    }

    Averaging(int averagingType, int maximumAverages);

    void average(const QVector<cx_double> &input);
    void average(const QVector<double> &input);

    bool averagingDone() const;
    QVector<double> get();
    QVector<cx_double> getComplex();
private:
    void averageLinear(const QVector<cx_double> &input);
    void averageExponential(const QVector<cx_double> &input);
    void averagePeak(const QVector<cx_double> &input);
    void averageEnergetic(const QVector<cx_double> &input);

    QVector<cx_double> averaged;
    int averagingType;
    int maximumAverages;
    int averagesMade;

    double rho;
};

#endif // AVERAGING_H
