#include "iirfilter.h"

#include "methods/abstractmethod.h"

//#include <complex>

IIRFilter::IIRFilter(const Parameters &p)
{
    if (p.bandStrip==0) {
        return;
    }

    double cutoff = p.sampleRate;
    int band = p.bandStrip;
    while (band>1) {
        cutoff /= 2.0;
        band--;
    }
    cutoff /= 2.56;

    computeCoefficients(p.sampleRate, cutoff);
}

void updateStateLine(QVector<double>& state, int size,
                     const QVector<double>& a, const QVector<double>& b,
                     const double& x, const double& y) {
    for (int k=1; k<size; ++k) {
        state[k-1] = (b[k]*x - a[k]*y) + state[k];
        if (std::fpclassify(state[k-1]) == FP_SUBNORMAL)
            state[k-1] = 0.0;
    }
}

void IIRFilter::filter(QVector<double> &input)
{
    QVector<double> x = input;
    QVector<double> y(x.size());
    reset();

    for (int n=0; n < y.size(); ++n) {
        y[n] = b[0]*x[n] + state[0];
        updateStateLine(state, state.size(), a, b, x[n], y[n]);
    }
}

void sftrans(QVector<double> &zero,
             QVector<double> &real,
             QVector<double> &imag, double &gain, double W)
{
    double C = 1.0;
    int p = real.size();
    int z = zero.size();

    double Fc = W;

    gain = gain * pow(C/Fc, z-p);

    for (int i=0; i<real.size(); ++i) {
        real[i] = real[i] *W/C;
        imag[i] = imag[i]*W/C;
    }
}

void bilinear(QVector<double> &Sz,
              QVector<double> &Spr,
              QVector<double> &Spi, double &Sg)
{
    double productr = 1.0-Spr[0];
    double producti = -1.0*Spi[0];

    for (int i=1; i<Spr.size(); ++i) {
        double d = productr;
        productr = d*(1.0-Spr[i]) + producti * Spi[i];
        producti = producti*(1.0-Spr[i]) - d*Spi[i];
    }
    double Zg = Sg/productr;

    QVector<double> Zpr(Spr.size()), Zpi(Spi.size());
    for (int i=0; i<Spr.size(); ++i) {
        Zpr[i] = (1.0-Spr[i]*Spr[i]-Spi[i]*Spi[i])/(pow(1.0-Spr[i],2)+Spi[i]*Spi[i]);
        Zpi[i] = 2.0*Spi[i]/(pow(1.0-Spr[i],2)+Spi[i]*Spi[i]);
    }
    QVector<double> Zz(Zpr.size(), -1.0);

    Sz = Zz;
    Spr = Zpr;
    Spi = Zpi;
    Sg = Zg;
}

QVector<double> poly(QVector<double> &v)
{
    QVector<double> y(v.size()+1);
    y[0] = 1.0;

    for (int j=0; j<v.size(); ++j) {
        QVector<double> t = y;
        for (int k=1; k<=j+1; ++k) {
            y[k] = t[k] - v[j] * t[k-1];
        }
    }

    return y;
}

QVector<double> poly(QVector<double> &vr, QVector<double> &vi)
{
    QVector<double> yr(vr.size()+1);
    yr[0] = 1.0;

    QVector<double> yi(vi.size()+1);

    for (int j=0; j<vr.size(); ++j) {
        QVector<double> tr = yr;
        QVector<double> ti = yi;

        for (int k=1; k<=j+1; ++k) {
            yr[k] = tr[k] - (vr[j] * tr[k-1]-vi[j] * ti[k-1]);
            yi[k] = ti[k] - (vi[j] * tr[k-1] + vr[j]*ti[k-1]);
        }
    }

    return yr;
}

void IIRFilter::computeCoefficients(double sampleRate, double cutoff)
{
    // вычисление коэффициентов
    const int order = 20;

    double W = cutoff / sampleRate * 2.0;

    double T = 2;       // sampling frequency of 2 Hz
    W = 2.0 / T * tan(M_PI*W/T);

    QVector<double> real(order), imag(order);

    for (int i=0; i<order; ++i) {
        real[i] = cos(M_PI*(2.0*(i+1)+order-1.0)/2.0/order);
        imag[i] = sin(M_PI*(2.0*(i+1)+order-1.0)/2.0/order);
    }

    QVector<double> zero;
    double gain = 1.0;

    sftrans(zero, real, imag, gain, W);
    bilinear(zero, real, imag, gain);

    QVector<double> p = poly(zero);
    for (int i=0; i<p.size(); ++i) p[i] *= gain;

    b = p;
    a = poly(real,imag);

    state.resize(b.size());
    reset();
}

void IIRFilter::reset()
{
    for (int i=0; i<state.size(); ++i)
        state[i] = 0.0;
}
