#include "iirfilter.h"

#include "methods/abstractmethod.h"

//#include <complex>

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

void sftrans(QVector<double> &zero,
             QVector<double> &real, QVector<double> &imag,
             double &gain,
             double Fl, double Fh,
             bool stop)
{
    double C = 1.0;
    const int p = real.size();
    const int z = zero.size();

    if (Fh > 0.0) {
        if (stop) {
            // ----------------  -------------------------  ------------------------
            // Band Stop         zero: b ± sqrt(b^2-FhFl)   pole: b ± sqrt(b^2-FhFl)
            //        S(Fh-Fl)   pole: ±sqrt(-FhFl)         zero: ±sqrt(-FhFl)
            // S -> C --------   gain: -x                   gain: -1/x
            //        S^2+FhFl   b=C/x (Fh-Fl)/2            b=C/x (Fh-Fl)/2
            // ----------------  -------------------------  ------------------------
            double prodr = 1.0;
            double prodi = 0.0;
            for (int i=0; i<p; ++i) {
                double prodr_ = prodi * imag[i] - prodr * real[i]; // real[i] * -1, imag[i] * -1
                prodi = -1.0 * prodr * imag[i] - prodi * real[i];// real[i] * -1, imag[i] * -1
                prodr = prodr_;
            }
            gain *= prodr / (prodr*prodr + prodi*prodi);


            const double coef = C*(Fh-Fl)/2.0;
            QVector<double> br(p, 0.0);
            QVector<double> bi(p, 0.0);
            for (int i=0; i<p; ++i) {
                br[i] = coef * real[i] / (real[i] * real[i] + imag[i] * imag[i]);
                bi[i] = coef * (-1.0) * imag[i] / (real[i] * real[i] + imag[i] * imag[i]);
            }

            real.resize(p*2);
            imag.resize(p*2);
            for (int i = 0; i < br.size(); ++i) {
                double sq = sqrt(br[i]*br[i] + bi[i]*bi[i] - Fh*Fl);
                real[i] = br[i] + sq;
                imag[i] = bi[i] + sq;
                real[i+br.size()] = br[i] - sq;
                imag[i+br.size()] = bi[i] - sq;
            }
            QVector<double> extend(2);
            extend[0] = sqrt(-1.0*Fh*Fl);
            extend[1] = -1.0*sqrt(-1.0*Fh*Fl);

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
            gain *= pow(C/(Fh-Fl) , z-p);
            QVector<double> br(p, 0.0);
            QVector<double> bi(p, 0.0);
            for (int i=0; i<p; ++i) {
                br[i] = real[i] * ((Fh-Fl)/(2.0*C));
                bi[i] = imag[i] * ((Fh-Fl)/(2.0*C));
            }

            real.resize(p*2);
            imag.resize(p*2);
            for (int i = 0; i < p; ++i) {
                double sq = sqrt(br[i]*br[i] + bi[i]*bi[i] - Fh*Fl);
                real[i] = br[i] + sq;
                imag[i] = bi[i] + sq;
                real[i+br.size()] = br[i] - sq;
                imag[i+br.size()] = bi[i] - sq;
            }

            zero = QVector<double>(p, 0.0);
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

            double prodr = 1.0;
            double prodi = 0.0;
            for (int i=0; i<p; ++i) {
                double prodr_ = prodi * imag[i] - prodr * real[i]; // real[i] * -1, imag[i] * -1
                prodi = -1.0 * prodr * imag[i] - prodi * real[i];// real[i] * -1, imag[i] * -1
                prodr = prodr_;
            }
            gain *= prodr / (prodr*prodr + prodi*prodi);

            for(int i=0; i<p; ++i) {
                real[i] = C * Fc * real[i] / (real[i] * real[i] + imag[i] * imag[i]);
                imag[i] = -1.0 * C * Fc * imag[i] / (real[i] * real[i] + imag[i] * imag[i]);
            }

            zero = QVector<double>(p, 0.0);
        }
        else {
            // ----------------  -------------------------  ------------------------
            // Low Pass          zero: Fc x/C               pole: Fc x/C
            // S -> C S/Fc       gain: C/Fc                 gain: Fc/C
            // ----------------  -------------------------  ------------------------
            gain *= pow(C/Fc, z-p);
            for (int i=0; i<p; ++i) {
                real[i] *= Fc/C;
                imag[i] *= Fc/C;
            }
            for (int i=0; i<zero.size(); ++i)
                zero[i] *= Fc / C;
        }
    }
}

void bilinear(QVector<double> &Sz,
              QVector<double> &Spr,
              QVector<double> &Spi, double &Sg)
{
    int p = Spr.size();
    int z = Sz.size();

    double prodSz = 1.0;
    foreach (double val, Sz) {
        prodSz *= (1.0 - val);
    }
    double prodSpr = 1.0;
    double prodSpi = 1.0;
    for (int i=0; i<p; ++i) {
        double prod = prodSpr * (1 - Spr[i]) + prodSpi * Spi[i];
        prodSpi = prodSpi * (1 - Spr[i]) - prodSpr * Spi[i];
        prodSpr = prod;
    }
    double Zg = Sg * prodSz * prodSpr / (prodSpr*prodSpr + prodSpi*prodSpi);

    QVector<double> Zpr(Spr.size()), Zpi(Spi.size());
    for (int i=0; i<p; ++i) {
        Zpr[i] = (1.0 - Spr[i]*Spr[i] -Spi[i]*Spi[i])/((1.0-Spr[i])*(1.0-Spr[i])+Spi[i]*Spi[i]);
        Zpi[i] = (2.0*Spi[i])/((1.0-Spr[i])*(1.0-Spr[i])+Spi[i]*Spi[i]);
    }

    QVector<double> Zz(p,-1.0);
    if (z>0) {
        for (int i=0; i<z; ++i) {
            Zz[i] = (1.0+Sz[i])/(1.0-Sz[i]);
        }
    }
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

    QVector<double> zero;
    double gain = 1.0;

    sftrans(zero, real, imag, gain, Wl, Wh, highPass);
    bilinear(zero, real, imag, gain);

    QVector<double> p = poly(zero);
    for (int i=0; i<p.size(); ++i) p[i] *= gain;

    b = p;
    a = poly(real,imag);

    state.resize(b.size());
    reset();
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
