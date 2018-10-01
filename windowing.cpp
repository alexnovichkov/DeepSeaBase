#include "windowing.h"

Windowing::Windowing(int bufferSize, int windowType)
{
    w = QVector<double>(bufferSize, 0.0);

    switch (windowType) {
        case Unknown: break;
        case Square: square(); break;
        case Triangular: triangular(); break;
        case Hann: hann(); break;
        case Hamming: hamming(); break;
        case Natoll: natoll(); break;
        case Gauss: gauss(); break;
        default: break;
    }
}

void Windowing::square()
{
    for (int i=0; i<w.size(); ++i)
        w[i] = 1.0;
}

void Windowing::hann()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 1.0 + cos(2.0*M_PI*t);
    }
}

void Windowing::triangular()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 2.0 - 4.0*qAbs(t);
    }
}

void Windowing::hamming()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 1.85*(0.54+0.46*cos(2.0*M_PI*t));
    }
}

void Windowing::natoll()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 2.75*(0.3635819
                     + 0.4891775*cos(2.0*M_PI*t)
                     + 0.1365995*cos(2.0*M_PI*t*2.0)
                     + 0.0106411*cos(2.0*M_PI*t*3.0));
    }
}

void Windowing::gauss()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 2.02*exp(2.0*2.5*2.5*t*t);
    }
}


void Windowing::applyTo(QVector<float> &values)
{
    int i=0;
    while (1) {
        for (int j=0; j<w.size(); j++) {
            if (i >= values.size()) return;
            values[i] = values[i] * w[j];
            i++;
        }
    }
}

