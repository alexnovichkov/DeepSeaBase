#ifndef AVERAGING_H
#define AVERAGING_H

#include <QtCore>

#include "algorithms.h"

class FileDescriptor;

class Averaging
{
public:
    enum AveragingType
    {
        NoAveraging = 0,
        Linear = 1,
        Exponential = 2,
        PeakHold = 3,
        Energetic = 4
    };

    static QString averagingDescription(int avgType) {
        switch (avgType) {
            case NoAveraging: return "без усреднения";
            case Linear: return "линейное";
            case Exponential: return "экспоненциальное";
            case PeakHold: return "хранение максимума";
            case Energetic: return "энергетическое";
        }
        return "";
    }
    static QString averagingDescriptionEng(int avgType) {
        switch (avgType) {
            case NoAveraging: return "no";
            case Linear: return "linear";
            case Exponential: return "exponential";
            case PeakHold: return "peak hold";
            case Energetic: return "energetic";
        }
        return "";
    }

    explicit Averaging();
    explicit Averaging(int averagingType, int maximumAverages);

    void average(const QVector<cx_double> &input);
    void average(const QVector<double> &input);

    bool averagingDone() const;
    QVector<double> get();
    QVector<cx_double> getComplex();

    void reset();

    int getAveragingType() const;
    void setAveragingType(int value);

    int getMaximumAverages() const;
    void setMaximumAverages(int value);

    int getAveragesMade() const;
    int averagesReallyMade = 0;

private:
    void averageLinear(const QVector<cx_double> &input);
    void averageExponential(const QVector<cx_double> &input);
    void averagePeak(const QVector<cx_double> &input);
    void averageEnergetic(const QVector<cx_double> &input);

    void averageLinear(const QVector<double> &input);
    void averageExponential(const QVector<double> &input);
    void averagePeak(const QVector<double> &input);
    void averageEnergetic(const QVector<double> &input);

    QVector<cx_double> averaged;
    QVector<double> averaged_;
    int averagingType;
    int maximumAverages;
    int averagesMade;
    bool averagingCompleted = false;


    double rho;
};

Averaging *averageChannels(const QList<QPair<FileDescriptor *, int> > &toMean);

#endif // AVERAGING_H
