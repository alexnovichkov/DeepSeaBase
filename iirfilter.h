#ifndef IIRFILTER_H
#define IIRFILTER_H

#include <QtCore>

class Parameters;

class IIRFilter
{
public:
    IIRFilter(const Parameters &p);

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

#endif // IIRFILTER_H
