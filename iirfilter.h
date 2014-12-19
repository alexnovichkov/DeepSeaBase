#ifndef IIRFILTER_H
#define IIRFILTER_H

#include <QtCore>

class IIRFilter
{
public:
    IIRFilter(double sampleRate, double cutoffFrequency, QVector<double> &input);
    QVector<double> &output();
private:
    QVector<double> a;
    QVector<double> b;
    QVector<double> state;
    QVector<double> &y;
};

#endif // IIRFILTER_H
