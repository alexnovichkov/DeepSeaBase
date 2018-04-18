#ifndef IIRFILTER_H
#define IIRFILTER_H

#include <QtCore>

class Parameters;

class IIRFilter
{
public:
    IIRFilter(const int bandStrip, const double sampleRate);

    void filter(QVector<double> &input);
private:
    void computeCoefficients(double sampleRate, double cutoff);
    void reset();
    QVector<double> a;
    QVector<double> b;
    QVector<double> state;

    QVector<double> y;
    QVector<double> x;

};

class ButterworthFilter
{
public:
    ButterworthFilter(const double sampleRate);
    void computeCoefficients(const int order, const double cutoffFreq, bool highPass=false); //creates low-pass filter with cutoff pi*Wc radians
                                                                                           //creates high-pass filter if highPass is false
    void computeCoefficients(const int order, const double lowCutoffFreq, double highCutoffFreq); //creates band-pass filter

    void filter(QVector<double> &input);
private:

    void reset();
    QVector<double> a;
    QVector<double> b;
    QVector<double> state;

    QVector<double> y;
    QVector<double> x;

    int order;
    double cutoff;
    double highCutOff;
    bool highPass;
    double sampleRate;
};

#endif // IIRFILTER_H
