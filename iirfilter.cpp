#include "iirfilter.h"

IIRFilter::IIRFilter(double sampleRate, double cutoffFrequency, QVector<double> &input)
    : y(input)
{
    //inputSize = input.size();
    state = QVector<double>(2, 0.0);

    double c = (tan(M_PI*cutoffFrequency/sampleRate) - 1) /
               (tan(M_PI*cutoffFrequency/sampleRate) + 1);

    b = QVector<double>(2, 0.0);
    b[0] = (1.0+c)/2.0;
    b[1] = (1.0+c)/2.0;

    a = QVector<double>(2, 0.0);
    a[0] = 1.0;
    a[1] = c;
}

QVector<double> &IIRFilter::output()
{
   // QVector<double> y(inputSize, 0.0);

    for (quint32 n=0; n < y.size(); ++n) {
        double x = y[n];
        y[n] = b[0] * x + state[0];

        state[0] = b[1]*x - a[1]*y[n]+state[1];

        // make sure there are no denormal numbers in the state line
        if (std::fpclassify(state[0]) == FP_SUBNORMAL) state[0] = 0.0;
    }
    return y;
}
