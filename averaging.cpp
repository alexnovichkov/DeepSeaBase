#include "averaging.h"

Averaging::Averaging(int averagingType, int maximumAverages) :
    averagingType(averagingType), maximumAverages(maximumAverages)
{
    averagesMade = 0;
    rho = maximumAverages != 0.0 ? 1.0 / maximumAverages : 1.0;
}

void Averaging::average(const QVector<double> &input)
{
    averaged.resize(input.size());

    switch (averagingType) {
        case Linear:
            averageLinear(input);
            averagesMade++;
            break;
        case Exponential:
            averageExponential(input);
            averagesMade++;
            break;
        case PeakHold:
            averagePeak(input);
            averagesMade++;
            break;
        default: break;
    }
}

void Averaging::average(const QVector<QPair<double, double> > &input)
{
    averaged1.resize(input.size());

    switch (averagingType) {
        case Linear:
            averageLinear(input);
            averagesMade++;
            break;
        case Exponential:
            averageExponential(input);
            averagesMade++;
            break;
        case PeakHold:
            averagePeak(input);
            averagesMade++;
            break;
        default: break;
    }
}

bool Averaging::averagingDone() const
{
    return averagesMade >= maximumAverages;
}

QVector<double> Averaging::get()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged.size(); ++i) {
            averaged[i] = sqrt(averaged[i]);
        }
    }
    return averaged;
}

QVector<QPair<double, double> > Averaging::getComplex()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged1.size(); ++i) {
            averaged1[i].first = sqrt(averaged1[i].first);
            averaged1[i].second = sqrt(averaged1[i].second);
        }
    }
    return averaged1;
}

void Averaging::averageLinear(const QVector<QPair<double, double> > &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged1[i].first = (averagesMade*averaged1[i].first+input[i].first)/(averagesMade+1);
        averaged1[i].second = (averagesMade*averaged1[i].second+input[i].second)/(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<QPair<double, double> > &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged1[i].first =  (1.0-rho)*averaged1[i].first +rho*input[i].first;
        averaged1[i].second = (1.0-rho)*averaged1[i].second+rho*input[i].second;
    }
}

void Averaging::averagePeak(const QVector<QPair<double, double> > &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged1[i].first =  qMax(input[i].first, averaged1[i].first);
        averaged1[i].second = qMax(input[i].second, averaged1[i].second);
    }
}

void Averaging::averageEnergetic(const QVector<QPair<double, double> > &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged1[i].first = (averagesMade*averaged1[i].first+pow(input[i].first,2.0))/(averagesMade+1);
        averaged1[i].second = (averagesMade*averaged1[i].second+pow(input[i].second,2.0))/(averagesMade+1);
    }
}

void Averaging::averageLinear(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (averagesMade * averaged[i]+input[i])/(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (1.0-rho)*averaged[i]+rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = qMax(input[i], averaged[i]);
    }
}

void Averaging::averageEnergetic(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (averagesMade * averaged[i]+pow(input[i], 2.0))/(averagesMade+1);
    }
}
