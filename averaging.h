#ifndef AVERAGING_H
#define AVERAGING_H

#include <QtCore>

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
    void average(const QVector<double> &input);
    void average(const QVector<QPair<double,double> > &input);
    bool averagingDone() const;
    QVector<double> get();
    QVector<QPair<double,double> > getComplex();
private:
    void averageLinear(const QVector<QPair<double,double> > &input);
    void averageExponential(const QVector<QPair<double,double> > &input);
    void averagePeak(const QVector<QPair<double,double> > &input);
    void averageEnergetic(const QVector<QPair<double,double> > &input);

    void averageLinear(const QVector<double> &input);
    void averageExponential(const QVector<double> &input);
    void averagePeak(const QVector<double> &input);
    void averageEnergetic(const QVector<double> &input);

    QVector<double> averaged;
    QVector<QPair<double,double> > averaged1;
    int averagingType;
    int maximumAverages;
    int averagesMade;

    double rho;
};

#endif // AVERAGING_H
