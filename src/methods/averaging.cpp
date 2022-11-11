#include "averaging.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

Averaging::Averaging() :
    averagingType(Linear), maximumAverages(0)
{DD;
    averagesMade = 0;
    rho = 1.0;
}

Averaging::Averaging(int averagingType, int maximumAverages) :
    averagingType(averagingType), maximumAverages(maximumAverages)
{DD;
    averagesMade = 0;
    rho = maximumAverages != 0 ? 1.0 / maximumAverages : 1.0;
}

void Averaging::average(const QVector<cx_double> &input)
{DD;
    if (averagingType != NoAveraging)
        averaged.resize(input.size());

    switch (averagingType) {
        case NoAveraging:
            averaged.append(input);
            break;
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
    if (maximumAverages > 0)
        averagingCompleted = averagesMade>=maximumAverages;
}

void Averaging::average(const QVector<double> &input)
{DD;
    if (averagingType != NoAveraging)
        averaged_.resize(input.size());

    switch (averagingType) {
        case NoAveraging:
            averaged_.append(input);
            break;
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
    if (maximumAverages > 0)
        averagingCompleted = averagesMade>=maximumAverages;
}

bool Averaging::averagingDone() const
{DD;
//    // усредняем пока поступают данные если количество усреднений равно нулю или -1
//    if (maximumAverages <= 0) return false;

//    // усредняем до нужного количества если количество усреднений > 0
//    return averagesMade >= maximumAverages;
    return averagingCompleted;
}

QVector<double> Averaging::get()
{DD;
    if (averagingType == Energetic) {
        for (double &i: averaged_)
            i = sqrt(std::abs(i));
    }
    return averaged_;
}

QVector<cx_double> Averaging::getComplex()
{DD;
    if (averagingType == Energetic) {
        for (cx_double &i : averaged) {
            i = sqrt(i);
        }
    }
    return averaged;
}

void Averaging::reset()
{DD;
    averaged.clear();
    averaged_.clear();
    if (averagesMade > 0) averagesReallyMade = averagesMade;
    averagesMade = 0;
    averagingCompleted = false;
}

void Averaging::averageLinear(const QVector<cx_double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (std::operator *(double(averagesMade), averaged[i])+input[i])/double(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<cx_double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged[i] =  (1.0-rho)*averaged[i] +rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<cx_double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        if (std::norm(input[i]) > std::norm(averaged[i]))
            averaged[i] = input[i];
    }
}

void Averaging::averageEnergetic(const QVector<cx_double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged[i] = (averaged[i]*double(averagesMade)+pow(input[i], 2.0))/double(averagesMade+1);
    }
}

void Averaging::averageLinear(const QVector<double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] = (double(averagesMade) * averaged_[i]+input[i])/double(averagesMade+1);
    }
}

void Averaging::averageExponential(const QVector<double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] =  (1.0-rho)*averaged_[i] +rho*input[i];
    }
}

void Averaging::averagePeak(const QVector<double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        if (std::abs(input[i]) > std::abs(averaged_[i]))
            averaged_[i] = input[i];
    }
}

void Averaging::averageEnergetic(const QVector<double> &input)
{DD;
    for (int i=0; i<input.size(); ++i) {
        averaged_[i] = (averaged_[i]*double(averagesMade)+pow(input[i], 2.0))/double(averagesMade+1);
    }
}

int Averaging::getAveragesMade() const
{DD;
    return averagesMade;
}

int Averaging::size() const
{DD;
    if (!averaged.isEmpty()) return averaged.size();
    if (!averaged_.isEmpty()) return averaged_.size();
    return 0;
}

int Averaging::getMaximumAverages() const
{DD;
    return maximumAverages;
}

void Averaging::setMaximumAverages(int value)
{DD;
    maximumAverages = value;
    rho = maximumAverages != 0 ? 1.0 / maximumAverages : 1.0;
}

int Averaging::getAveragingType() const
{DD;
    return averagingType;
}

void Averaging::setAveragingType(int value)
{DD;
    averagingType = value;
}
