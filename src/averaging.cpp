#include "averaging.h"

Averaging::Averaging() :
    averagingType(Linear), maximumAverages(0)
{
    averagesMade = 0;
    rho = 1.0;
}

Averaging::Averaging(int averagingType, int maximumAverages) :
    averagingType(averagingType), maximumAverages(maximumAverages)
{
    averagesMade = 0;
    rho = maximumAverages != 0 ? 1.0 / maximumAverages : 1.0;
}

void Averaging::average(const QVector<cx_double> &input)
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
        case Energetic:
            averageEnergetic(input);
            averagesMade++;
        default: break;
    }
}

void Averaging::average(const QVector<double> &input)
{
    averaged_.resize(input.size());

    switch (averagingType) {
        case Linear:
            averageLinear(input);
            break;
        case Exponential:
            averageExponential(input);
            break;
        case PeakHold:
            averagePeak(input);
            break;
        case Energetic:
            averageEnergetic(input);
        default: break;
    }
    averagesMade++;
}

bool Averaging::averagingDone() const
{
    // усредняем пока поступают данные если количество усреднений равно нулю или -1
    if (maximumAverages <= 0) return false;

    // усредняем до нужного количества если количество усреднений > 0
    return averagesMade >= maximumAverages;
}

QVector<double> Averaging::get()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged_.size(); ++i) {
            averaged_[i] = sqrt(std::abs(averaged_[i]));
        }
    }
    return averaged_;
}

QVector<cx_double> Averaging::getComplex()
{
    if (averagingType == Energetic) {
        for (int i=0; i<averaged.size(); ++i) {
            averaged[i] = sqrt(averaged[i]);
        }
    }
    return averaged;
}

void Averaging::reset()
{
    averaged.clear();
    averaged_.clear();
    averagesMade = 0;
}

void Averaging::averageLinear(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (std::operator *(double(averagesMade), averaged[i])+input[i])/double(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] =  (1.0-rho)*averaged[i] +rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        if (std::norm(input[i]) > std::norm(averaged[i]))
            averaged[i] = input[i];
    }
}

void Averaging::averageEnergetic(const QVector<cx_double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (averaged[i]*double(averagesMade)+pow(input[i], 2.0))/double(averagesMade+1);
    }
}

void Averaging::averageLinear(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] = (double(averagesMade) * averaged_[i]+input[i])/double(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] =  (1.0-rho)*averaged_[i] +rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        if (std::abs(input[i]) > std::abs(averaged_[i]))
            averaged_[i] = input[i];
    }
}

void Averaging::averageEnergetic(const QVector<double> &input)
{
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] = (averaged_[i]*double(averagesMade)+pow(input[i], 2.0))/double(averagesMade+1);
    }
}

int Averaging::getAveragesMade() const
{
    return averagesMade;
}

int Averaging::getMaximumAverages() const
{
    return maximumAverages;
}

void Averaging::setMaximumAverages(int value)
{
    maximumAverages = value;
}

int Averaging::getAveragingType() const
{
    return averagingType;
}

void Averaging::setAveragingType(int value)
{
    averagingType = value;
}
