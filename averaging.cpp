#include "averaging.h"

Averaging::Averaging(int averagingType, int maximumAverages) :
    averagingType(averagingType), maximumAverages(maximumAverages)
{
    averagesMade = 0;
    rho = maximumAverages != 0.0 ? 1.0 / maximumAverages : 1.0;
}

void Averaging::average(const QVector<cx_double> &input)
{
    averaged2.resize(input.size());

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

void Averaging::average(const QVector<double> &input)
{
    average(complexes(input));
}

bool Averaging::averagingDone() const
{
    return averagesMade >= maximumAverages;
}

QVector<double> Averaging::get()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged2.size(); ++i) {
            averaged2[i] = sqrt(averaged2[i]);
        }
    }
    return absolutes(averaged2);
}

QVector<cx_double> Averaging::getComplex()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged2.size(); ++i) {
            averaged2[i] = sqrt(averaged2[i]);
        }
    }
    return averaged2;
}

void Averaging::averageLinear(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged2[i] = (std::operator *(double(averagesMade), averaged2[i])+input[i])/double(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged2[i] =  (1.0-rho)*averaged2[i] +rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        if (std::abs(input[i]) > std::abs(averaged2[i]))
            averaged2[i] = input[i];
    }
}

void Averaging::averageEnergetic(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged2[i] = (averaged2[i]*double(averagesMade)+pow(input[i], 2.0))/double(averagesMade+1);
    }
}

