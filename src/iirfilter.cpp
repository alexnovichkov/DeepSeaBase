#include "iirfilter.h"

#include "methods/abstractmethod.h"

#include <complex>
#include <iostream>
#include "algorithms.h"
#include "logging.h"

template<typename T>
void fliplr(QVector<T> &x)
{DD;
    std::reverse(x.begin(), x.end());
}

void sftrans(QVector<cx_double > &zero,
             QVector<cx_double > &pole,
             cx_double &Sg,
             const QVector<double> &W,
             bool stop)
{
    double C = 1.0;
    const int p = pole.size();
    const int z = zero.size();

    if (W.size()==2) {
        double Fl = W.first();
        double Fh = W.last();
        if (stop) {
            // ----------------  -------------------------  ------------------------
            // Band Stop         zero: b ± sqrt(b^2-FhFl)   pole: b ± sqrt(b^2-FhFl)
            //        S(Fh-Fl)   pole: ±sqrt(-FhFl)         zero: ±sqrt(-FhFl)
            // S -> C --------   gain: -x                   gain: -1/x
            //        S^2+FhFl   b=C/x (Fh-Fl)/2            b=C/x (Fh-Fl)/2
            // ----------------  -------------------------  ------------------------

            cx_double prod(1.0, 0.0);
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
            QVector<cx_double > b(p);
            cx_double one(1,0);
            for (int i=0; i<p; ++i) {
                b[i] = one * coef / pole[i];
            }

            pole.resize(p*2);
            for (int i = 0; i < p; ++i) {
                cx_double sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                pole[i] = b[i] + sq;
                pole[i+p] = b[i] - sq;
            }

            QVector<cx_double > extend(2);
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
            QVector<cx_double > b(p);
            for (int i=0; i<p; ++i) {
                b[i] = pole[i] * ((Fh-Fl)/(2.0*C));
            }

            pole.resize(p*2);
            for (int i = 0; i < p; ++i) {
                cx_double sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                pole[i]   = b[i] + sq;
                pole[i+p] = b[i] - sq;
            }
            if (zero.isEmpty()) {
                zero = QVector<cx_double >(p);
            }
            else {
                b.resize(z);
                for (int i=0; i<z; ++i) {
                    b[i] = zero[i] * ((Fh-Fl)/(2.0*C));
                }
                zero.resize(2*z);
                for (int i = 0; i < z; ++i) {
                    cx_double sq = pow(b[i]*b[i]-Fh*Fl, 0.5);

                    zero[i]   = b[i] + sq;
                    zero[i+z] = b[i] - sq;
                }
                if (p>z) zero.append(QVector<cx_double >(p-z));
            }
        }
    }
    else if (W.size()==1) {
        double Fc = W.first();
        if (stop) {
            // ----------------  -------------------------  ------------------------
            // High Pass         zero: Fc C/x               pole: Fc C/x
            // S -> C Fc/S       pole: 0                    zero: 0
            //                   gain: -x                   gain: -1/x
            // ----------------  -------------------------  ------------------------
            cx_double prod(1.0, 0.0);
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

            cx_double one(1,0);
            for(int i=0; i<p; ++i) {
                pole[i] = one * C * Fc / pole[i];
            }

            if (zero.isEmpty()) {
                zero = QVector<cx_double >(p);
            }
            else {
                for(int i=0; i<z; ++i) {
                    zero[i] = one * C * Fc / zero[i];
                }
                if (p>z) zero.append(QVector<cx_double >(p-z));
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

void bilinear(QVector<cx_double > &Sz,
              QVector<cx_double > &Sp,
              cx_double &Sg)
{
    int p = Sp.size();
    int z = Sz.size();

    cx_double one(1.0, 0.0);

    cx_double prodSz = 1.0;
    foreach (cx_double val, Sz) {
        prodSz *= (one - val);
    }

    cx_double prod(1.0, 0.0);
    for (int i=0; i<p; ++i) {
        prod *= (one - Sp[i]);
    }
    prod = one * Sg * prodSz / prod;
    Sg = prod;

    QVector<cx_double > Zp(Sp.size(), one);
    for (int i=0; i<p; ++i) {
        Zp[i] = one*(one + Sp[i])/(one - Sp[i]);
    }

    QVector<cx_double > Zz(p,{-1.0, 0.0});

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

void butter(QVector<double> &B, QVector<double> &A, int N, double W1, double W2)
{
    // Prewarp to the band edges to s plane
    double T = 2.0;       // sampling frequency of 2 Hz
    W1 = tan(M_PI*W1/T);
    W2 = tan(M_PI*W2/T);

    QVector<cx_double> pole(N);
    cx_double _i(0,1);
    double coef = 0.5/N;
    for (int i=0; i<N; ++i) {
        pole[i] = exp(_i*M_PI*(2.0*(i+1)+N-1.0)*coef);
    }

    if ((N % 2) != 0) { // pure real value at exp(i*pi)
        pole[int((N+1)/2)-1] = -1.0;
    }

    QVector<cx_double > zero;
    cx_double gain(1.0, 0);

    sftrans(zero, pole, gain, QVector<double>()<< W1 << W2, false);
    bilinear(zero, pole, gain);

    QVector<cx_double > p = poly<cx_double >(zero);
    B.resize(p.size());
    for (int i=0; i<p.size(); ++i)  {
        cx_double val = p[i] * gain;
        B[i] = val.real();
    }

    QVector<cx_double > AA = poly<cx_double >(pole);
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
    QVector<cx_double > pole;
    cx_double I(0.0, 1.0);
    for (int i=-n+1; i<=n-1; i+=2) {
        double coef = double(i)/n;
        cx_double val = std::exp(I*M_PI_2*coef);
        pole.append(cx_double(-1.0*sinh(v0)*val.real(), cosh(v0)*val.imag()));
    }
    Q_ASSERT(pole.size()==n);

    QVector<cx_double > zero;

    // compensate for amplitude at s=0
    cx_double gain(1.0, 0.0);
    for (int i=0; i<pole.size(); ++i) {
        gain = gain * (-1.0)*pole[i];
    }

    // if n is even, the ripple starts low, but if n is odd the ripple
    // starts high. We must adjust the s=0 amplitude to compensate.
    if (n % 2 == 0) {
        gain /= pow(10.0,Rp/20.0);
    }

//    // splane frequency transform
    sftrans(zero, pole, gain, QVector<double>()<< W, false);
    // Use bilinear transform to convert poles to the z plane
    bilinear(zero, pole, gain);

    QVector<cx_double > p = poly<cx_double >(zero);
    B.resize(p.size());
    for (int i=0; i<p.size(); ++i)  {
        cx_double val = p[i] * gain;
        B[i] = val.real();
    }

    QVector<cx_double > AA = poly<cx_double >(pole);
    A.resize(AA.size());
    for (int i=0; i<AA.size(); ++i) {
        A[i] = AA[i].real();
    }
}

QVector<double> filter(const QVector<double> &B, const QVector<double> &A, const QVector<double> &x, const QVector<double> &si)
{DD;
    const int xSize = x.size();

    QVector<double> y(xSize, 0.0);
    QVector<double> z=si;
    z.resize(B.size()-1);
    const int si_len = z.size();
    bool flag = false;
    for (int m = 0; m < xSize; m++) {

        y[m] = z[0] + B[0] * x[m];
        if (!qIsFinite(y[m])) {
            if (!flag)
            //qDebug()<<m<<y[m];
            flag = true;
        }

        for (int j = 0; j < si_len-1; j++) {
            z[j] = z[j+1] - A[j+1] * y[m] + B[j+1] * x[m];
        }

        //z[si_len-1] = B[si_len] * x[m] - A[si_len] * y[m];
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

    double kdc = std::accumulate(b.begin(), b.end(), 0.0);
    double sum = std::accumulate(a.begin(), a.end(), 0.0);
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
        fliplr(si);
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

//    auto mm = std::minmax_element(v.begin(), v.end());
//    double min = *mm.first;
//    int minpos = mm.first - v.begin();
//    double max = *mm.second;
//    int maxpos = mm.second - v.begin();

    // reverse filter
    SI = si;
    for (int i=0; i<SI.size(); ++i)
        SI[i] *= v.last();
    fliplr(v);
    v = filter(b,a,v,SI);
    fliplr(v);

//    mm = std::minmax_element(v.begin(), v.end());
//    min = *mm.first;
//    minpos = mm.first - v.begin();
//    max = *mm.second;
//    maxpos = mm.second - v.begin();

    return v.mid(lrefl, lx);
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
