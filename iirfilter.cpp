#include "iirfilter.h"

#include "methods/abstractmethod.h"

#include <complex>
#include <iostream>

template<typename T>
QVector<T> fliplr(const QVector<T> &x)
{
    const int n = x.size();
    QVector<T> result(n);
    for (int i=0; i<n; ++i)
        result[n-1-i] = x[i];
    return result;
}

template<typename T>
QDebug operator <<(QDebug debug, const std::complex<T> &val)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << val.real() << ", " << val.imag() << ')';
    return debug;
}

IIRFilter::IIRFilter(const int bandStrip, const double sampleRate)
{
    if (bandStrip==0) {
        return;
    }

    double cutoff = sampleRate;
    int band = bandStrip;
    while (band>1) {
        cutoff /= 2.0;
        band--;
    }
    cutoff /= 2.56;

    computeCoefficients(sampleRate, cutoff);
}

void updateStateLine(QVector<double>& state, int stateSize,
                     const QVector<double>& a, const QVector<double>& b,
                     const double& x, const double& y) {
    for (int k=1; k<stateSize; ++k) {
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

void sftrans(QVector<std::complex<double> > &zero,
             QVector<std::complex<double> > &pole,
             std::complex<double> &Sg,
             double Fl, double Fh,
             bool stop)
{
    double C = 1.0;
    const int p = pole.size();
    const int z = zero.size();

    if (Fh > 0.0) {
        if (stop) {
            // ----------------  -------------------------  ------------------------
            // Band Stop         zero: b ± sqrt(b^2-FhFl)   pole: b ± sqrt(b^2-FhFl)
            //        S(Fh-Fl)   pole: ±sqrt(-FhFl)         zero: ±sqrt(-FhFl)
            // S -> C --------   gain: -x                   gain: -1/x
            //        S^2+FhFl   b=C/x (Fh-Fl)/2            b=C/x (Fh-Fl)/2
            // ----------------  -------------------------  ------------------------

            std::complex<double> prod(1.0, 0.0);
            if (zero.isEmpty()) {
                for (int i=0; i<p; ++i) {
                    prod *= pole[i]*(-1.0);
                }
                prod = std::pow(prod, -1.0);
            }
            else if (pole.isEmpty()) {
                for (int i=0; i<z; ++i) {
                    prod *= zero[i]*(-1.0);
                }
            }
            else {
                for (int i=0; i<z; ++i) {
                    prod *= zero[i] / pole[i];
                }
            }
            Sg *= prod.real();

            const double coef = C*(Fh-Fl)/2.0;
            QVector<std::complex<double> > b(p);
            std::complex<double> one(1,0);
            for (int i=0; i<p; ++i) {
                b[i] = one * coef / pole[i];
            }

            pole.resize(p*2);
            for (int i = 0; i < p; ++i) {
                std::complex<double> sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                pole[i] = b[i] + sq;
                pole[i+p] = b[i] - sq;
            }

            QVector<std::complex<double> > extend(2);
            extend[0] = one * std::pow(one*(-1.0)*Fh*Fl, 0.5);
            extend[1] = one * (-1.0)*std::pow(one*(-1.0)*Fh*Fl, 0.5);

            zero.resize(2*p);
            for (int i=0; i<zero.size(); ++i) {
                zero[i] = extend[1 - (i % 2)];
            }
        }
        else {
            // ----------------  -------------------------  ------------------------
            // Band Pass         zero: b ± sqrt(b^2-FhFl)   pole: b ± sqrt(b^2-FhFl)
            //        S^2+FhFl   pole: 0                    zero: 0
            // S -> C --------   gain: C/(Fh-Fl)            gain: (Fh-Fl)/C
            //        S(Fh-Fl)   b=x/C (Fh-Fl)/2            b=x/C (Fh-Fl)/2
            // ----------------  -------------------------  ------------------------
            Sg *= pow(C/(Fh-Fl) , z-p);
            QVector<std::complex<double> > b(p);
            for (int i=0; i<p; ++i) {
                b[i] = pole[i] * ((Fh-Fl)/(2.0*C));
            }

            pole.resize(p*2);
            for (int i = 0; i < p; ++i) {
                std::complex<double> sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                pole[i]   = b[i] + sq;
                pole[i+p] = b[i] - sq;
            }
            if (zero.isEmpty()) {
                zero = QVector<std::complex<double> >(p);
            }
            else {
                b.resize(z);
                for (int i=0; i<z; ++i) {
                    b[i] = zero[i] * ((Fh-Fl)/(2.0*C));
                }
                zero.resize(2*z);
                for (int i = 0; i < z; ++i) {
                    std::complex<double> sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                    zero[i]   = b[i] + sq;
                    zero[i+z] = b[i] - sq;
                }
                if (p>z) zero.append(QVector<std::complex<double> >(p-z));
            }
        }
    }
    else {
        double Fc = Fl;
        if (stop) {
            // ----------------  -------------------------  ------------------------
            // High Pass         zero: Fc C/x               pole: Fc C/x
            // S -> C Fc/S       pole: 0                    zero: 0
            //                   gain: -x                   gain: -1/x
            // ----------------  -------------------------  ------------------------
            std::complex<double> prod(1.0, 0.0);
            if (zero.isEmpty()) {
                for (int i=0; i<p; ++i) {
                    prod *= pole[i]*(-1.0);
                }
                prod = std::pow(prod, -1.0);
            }
            else if (pole.isEmpty()) {
                for (int i=0; i<z; ++i) {
                    prod *= zero[i]*(-1.0);
                }
            }
            else {
                for (int i=0; i<z; ++i) {
                    prod *= zero[i] / pole[i];
                }
            }
            Sg *= prod.real();

            std::complex<double> one(1,0);
            for(int i=0; i<p; ++i) {
                pole[i] = one * C * Fc / pole[i];
            }

            if (zero.isEmpty()) {
                zero = QVector<std::complex<double> >(p);
            }
            else {
                for(int i=0; i<z; ++i) {
                    zero[i] = one * C * Fc / zero[i];
                }
                if (p>z) zero.append(QVector<std::complex<double> >(p-z));
            }
        }
        else {
            // ----------------  -------------------------  ------------------------
            // Low Pass          zero: Fc x/C               pole: Fc x/C
            // S -> C S/Fc       gain: C/Fc                 gain: Fc/C
            // ----------------  -------------------------  ------------------------
            Sg *= std::pow(C/Fc, z-p);
            for (int i=0; i<p; ++i) {
                pole[i] *= Fc/C;
            }
            for (int i=0; i<zero.size(); ++i)
                zero[i] *= Fc / C;
        }
    }
}

void bilinear(QVector<std::complex<double> > &Sz,
              QVector<std::complex<double> > &Sp,
              std::complex<double> &Sg)
{
    int p = Sp.size();
    int z = Sz.size();

    std::complex<double> one(1.0, 0.0);

    std::complex<double> prodSz = 1.0;
    foreach (std::complex<double> val, Sz) {
        prodSz *= (one - val);
    }

    std::complex<double> prod(1.0, 0.0);
    for (int i=0; i<p; ++i) {
        prod *= (one - Sp[i]);
    }
    prod = one * Sg * prodSz / prod;
    Sg = prod;

    QVector<std::complex<double> > Zp(Sp.size(), one);
    for (int i=0; i<p; ++i) {
        Zp[i] = one*(one + Sp[i])/(one - Sp[i]);
    }

    QVector<std::complex<double> > Zz(p,{-1.0, 0.0});

    if (z>0) {
        for (int i=0; i<z; ++i) {
            Zz[i] = (1.0+Sz[i])/(1.0-Sz[i]);
        }
    }

    Sz = Zz;
    Sp = Zp;
}

template<typename T>
QVector<T> poly(const QVector<T> &x)
{
    QVector<T> v = x;
    QVector<T> y(x.size()+1);
    y[0] = T(1.0);

    for (int j=0; j<x.size(); ++j) {
        QVector<T> t = y;
        for (int k=1; k<=j+1; ++k) {
            y[k] = t[k] - v[j] * t[k-1];
        }
    }

    return y;
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
//    bilinear(zero, real, imag, gain);

//    QVector<double> p = poly(zero);
//    for (int i=0; i<p.size(); ++i) p[i] *= gain;

//    b = p;
//    a = poly(real,imag);

//    state.resize(b.size());
//    reset();
}

void IIRFilter::reset()
{
    for (int i=0; i<state.size(); ++i)
        state[i] = 0.0;
}

ButterworthFilter::ButterworthFilter(const double sampleRate) :
    sampleRate(sampleRate)
{

}

void ButterworthFilter::computeCoefficients(const int order, const double cutoffFreq, bool highPass)
{
    this->order = order;
    this->cutoff = cutoffFreq;
    this->highPass = highPass;
    this->highCutOff = 0.0;

    double Wl = cutoff / sampleRate * 2.0;
    double Wh = highCutOff / sampleRate * 2.0;

    butter(b,a,order,Wl, Wh);

     // Prewarp to the band edges to s plane
    double T = 2;       // sampling frequency of 2 Hz
    Wl = 2.0 / T * tan(M_PI*Wl/T);
    Wh = 2.0 / T * tan(M_PI*Wh/T);

    QVector<double> real(order), imag(order);

    for (int i=0; i<order; ++i) {
        real[i] = cos(M_PI*(2.0*(i+1)+order-1.0)/2.0/order);
        imag[i] = sin(M_PI*(2.0*(i+1)+order-1.0)/2.0/order);
    }
    if ((order % 2) != 0) { // pure real value at exp(i*pi)
        real[int((order+1)/2)] = -1.0;
        imag[int((order+1)/2)] = 0.0;
    }

//    QVector<double> zero;
//    double gain = 1.0;

//    sftrans(zero, real, imag, gain, Wl, Wh, highPass);
//    bilinear(zero, real, imag, gain);

//    QVector<double> p = poly(zero);
//    for (int i=0; i<p.size(); ++i) p[i] *= gain;

//    b = p;
//    a = poly(real,imag);

//    state.resize(b.size());
//    reset();
}

void ButterworthFilter::computeCoefficients(const int order, const double lowCutoffFreq, double highCutoffFreq)
{
    this->order = order;
    this->cutoff = lowCutoffFreq;
    this->highPass = false;
    this->highCutOff = highCutoffFreq;


}

void ButterworthFilter::filter(QVector<double> &input)
{
    QVector<double> x = input;
    QVector<double> y(x.size());
    reset();

    for (int n=0; n < y.size(); ++n) {
        y[n] = b[0]*x[n] + state[0];
        updateStateLine(state, state.size(), a, b, x[n], y[n]);
    }
}

void ButterworthFilter::reset()
{
    for (int i=0; i<state.size(); ++i)
        state[i] = 0.0;
}

void butter(QVector<double> &B, QVector<double> &A, int N, double W1, double W2)
{
    // Prewarp to the band edges to s plane
    double T = 2.0;       // sampling frequency of 2 Hz
    W1 = tan(M_PI*W1/T);
    W2 = tan(M_PI*W2/T);

    QVector<std::complex<double> > pole(N);
    std::complex<double> _i(0,1);
    double coef = 0.5/N;
    for (int i=0; i<N; ++i) {
        pole[i] = exp(_i*M_PI*(2.0*(i+1)+N-1.0)*coef);
    }

    if ((N % 2) != 0) { // pure real value at exp(i*pi)
        pole[int((N+1)/2)-1] = -1.0;
    }

    QVector<std::complex<double> > zero;
    std::complex<double> gain(1.0, 0);

    sftrans(zero, pole, gain, W1, W2, false);
    bilinear(zero, pole, gain);

    QVector<std::complex<double> > p = poly<std::complex<double> >(zero);
    B.resize(p.size());
    for (int i=0; i<p.size(); ++i)  {
        std::complex<double> val = p[i] * gain;
        B[i] = val.real();
    }

    QVector<std::complex<double> > AA = poly<std::complex<double> >(pole);
    A.resize(AA.size());
    for (int i=0; i<AA.size(); ++i) {
        A[i] = AA[i].real();
    }

}

void cheby1(QVector<double> &B, QVector<double> &A, int n, double Rp, double W)
{
    // Prewarp to the band edges to s plane
    double T = 2.0;       // sampling frequency of 2 Hz
    W = 2.0/T*tan(M_PI*W/T);

    // Generate splane poles and zeros for the chebyshev type 1 filter
    //double C = 1.0; // default cutoff frequency
    double epsilon = sqrt(pow(10.0,Rp/10.0) - 1);
    double v0 = asinh(1.0/epsilon)/n;
    QVector<std::complex<double> > pole;
    std::complex<double> I(0.0, 1.0);
    for (int i=-n+1; i<=n-1; i+=2) {
        double coef = double(i)/n;
        std::complex<double> val = std::exp(I*M_PI_2*coef);
        pole.append(std::complex<double>(-1.0*sinh(v0)*val.real(), cosh(v0)*val.imag()));
    }
    Q_ASSERT(pole.size()==n);

    QVector<std::complex<double> > zero;

    // compensate for amplitude at s=0
    std::complex<double> gain(1.0, 0.0);
    for (int i=0; i<pole.size(); ++i) {
        gain = gain * (-1.0)*pole[i];
    }

    // if n is even, the ripple starts low, but if n is odd the ripple
    // starts high. We must adjust the s=0 amplitude to compensate.
    if (n % 2 == 0) {
        gain /= pow(10.0,Rp/20.0);
    }

//    // splane frequency transform
    sftrans(zero, pole, gain, W, 0.0, false);
    // Use bilinear transform to convert poles to the z plane
    bilinear(zero, pole, gain);

    QVector<std::complex<double> > p = poly<std::complex<double> >(zero);
    B.resize(p.size());
    for (int i=0; i<p.size(); ++i)  {
        std::complex<double> val = p[i] * gain;
        B[i] = val.real();
    }

    QVector<std::complex<double> > AA = poly<std::complex<double> >(pole);
    A.resize(AA.size());
    for (int i=0; i<AA.size(); ++i) {
        A[i] = AA[i].real();
    }
}

QVector<double> filter(const QVector<double> &B, const QVector<double> &A, const QVector<double> &x, const QVector<double> &si)
{
    QVector<double> y(x.size(), 0.0);
    QVector<double> z=si;
    if (z.isEmpty()) z.resize(B.size());

    double norm = A.first();

    for (int m=0; m < x.size(); ++m) {
        y[m] = B[0]*x[m]/norm + z[0];
        for (int i = 1; i < B.size(); ++i)
            z[i - 1] = (B[i] * x[m] - A[i] * y[m])/norm + z[i] ;
    }

    return y;
}

/** ## Forward and reverse filter the signal. This corrects for phase
## distortion introduced by a one-pass filter, though it does square the
## magnitude response in the process. That''s the theory at least.  In
## practice the phase correction is not perfect, and magnitude response
## is distorted, particularly in the stop band.
##
## Example
## @example
## @group
## [b, a]=butter(3, 0.1);                  # 10 Hz low-pass filter
## t = 0:0.01:1.0;                         # 1 second sample
## x=sin(2*pi*t*2.3)+0.25*randn(size(t));  # 2.3 Hz sinusoid+noise
## y = filtfilt(b,a,x); z = filter(b,a,x); # apply filter
## plot(t,x,';data;',t,y,';filtfilt;',t,z,';filter;')
## @end group
## @end example
## @end deftypefn*/
QVector<double> filtfilt(const QVector<double> &B, const QVector<double> &A, const QVector<double> &x)
{
    QVector<double> b = B;
    QVector<double> a = A;

    int lx = x.size();
    int lb = b.size();
    int la = a.size();
    int n = std::max(lb, la);
    int lrefl = 3 * (n - 1);
    if (la < n) a.resize(n);
    if (lb < n) b.resize(n);

    // Compute a the initial state taking inspiration from
    // Likhterov & Kopeika, 2003. "Hardware-efficient technique for
    //     minimizing startup transients in Direct Form II digital filters"

    double kdc = b.first();
    for (int i=1; i<b.size(); ++i) kdc+=b[i];
    double sum = a.first();
    for (int i=1; i<a.size(); ++i) sum+=a[i];
    kdc /= sum;

    QVector<double> si;
    if (std::isinf(kdc) || std::isnan(kdc)) {
        si.resize(a.size());
    }
    else {
        const int n = b.size();
        QVector<double> sum(n);
        for (int i=0; i<n; ++i)
            sum[i] = b[i] - kdc * a[i];

        si.resize(n);
        si[0] = sum.last();
        for (int i=1; i<n; ++i) {
            si[i] = si[i-1]+sum[n-1-i]; // si = fliplr(cumsum(fliplr(b - kdc * a)));
        }
        si = fliplr(si);
    }
    si.removeFirst();

    // filter all columns, one by one
    QVector<double> v;
    for (int i=lrefl; i>=1; --i) {
        v << 2*x[0] - x[i];
    }
    v.append(x);
    for (int i=x.size()-2; i>=x.size()-lrefl-1; --i) {
        v << 2*x.last() -x[i];
    }

    QVector<double> SI = si;
    for (int i=0; i<SI.size(); ++i)
        SI[i] *= v.first();

    // Do forward and reverse filtering
    // forward filter
    v = filter(b,a,v,SI);
    // reverse filter
    SI = si;
    for (int i=0; i<SI.size(); ++i)
        SI[i] *= v.last();
    v = fliplr(filter(b,a,fliplr(v),SI));

    return v.mid(lrefl, lx+lrefl);
}

QVector<double> sinetone(double freq, double rate, double sec, double ampl)
{
    double ns = rate * sec;
    QVector<double> result(ns);

    for (int i=1; i<=ns; ++i) {
        result[i-1] = ampl * sin(2*M_PI*i / rate * freq);
    }

    return result;
}
