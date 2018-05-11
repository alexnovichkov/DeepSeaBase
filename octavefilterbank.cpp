#include "octavefilterbank.h"

#include "iirfilter.h"

OctaveFilterBank::OctaveFilterBank(const Parameters &p) : m_p(p)
{

}



//% OCT3DSGN  Design of a one-third-octave filter.
//%    [B,A] = OCT3DSGN(Fc,Fs,N) designs a digital 1/3-octave filter with
//%    center frequency Fc for sampling frequency Fs.
//%    The filter is designed according to the Order-N specification
//%    of the ANSI S1.1-1986 standard. Default value for N is 3.
//%    Warning: for meaningful design results, center frequency used
//%    should preferably be in range Fs/200 < Fc < Fs/5.
void oct3dsgn(QVector<double> &B, QVector<double> &A, double Fc, double Fs, int N)
{
    double f1 = Fc/pow(2.0, 1.0/6.0);
    double f2 = Fc*pow(2.0, 1.0/6.0);
    double Qr = Fc/(f2-f1);
    double Qd = (M_PI/2.0/N)/(sin(M_PI/2.0/N))*Qr;
    double alpha = (1.0 + sqrt(1.0+4.0*Qd*Qd))/2.0/Qd;
    double W1 = Fc/(Fs/2.0)/alpha;
    double W2 = Fc/(Fs/2.0)*alpha;
//    qDebug()<<QString("f1=%1, f2=%2, W1=%3, W2=%4").arg(f1).arg(f2).arg(W1).arg(W2);
    butter(B, A, N, W1, W2);
}

//function p=leq(x,t,Pref)
//% LEQ  Computes the sequence of short-time RMS powers (Leq) of a signal.
//%     P = LEQ(X,T) is a length LENGTH(X)/T column vector whose
//%     elements are the RMS powers (Leq''s) of length T frames of X.
//%     The powers are expressed in dB with 1 as reference level.
//%     LEQ(X,T,REF) uses REF as the reference level for the dB
//%     scale. If a RMS power is equal to zero, a NaN is returned
//%     for its dB value.
//%
double leq(const QVector<double> &x, /*int T,*/ double ref=1.0)
{
    double p = 0.0;
    for (int i = 0; i<x.size(); ++i) {
        p += x[i]*x[i]/x.size();
    }

    if (p > 0.0) p = 10.0*log10(p/ref/ref);
    else p = 0.0;

    return p;
}

//decimates x by factor q using chebyshev filter
QVector<double> decimate(const QVector<double> &x, int q)
{
    int n=8; //порядок фильтра
    QVector<double> b, a;
    cheby1(b, a, n, 0.05, 0.8/q);

    QVector<double> y;
    QVector<double> v = filtfilt(b,a,x);
    for (int i=0; i<v.size(); i+=q)
        y << v[i];
    return y;
}

QVector<double> OctaveFilterBank::compute(QVector<float> &signal, double sampleRate, QVector<double> &xValues)
{
    QVector<double> x(signal.size());
    for (int i=0; i<signal.size(); ++i)
        x[i] = signal[i];
//    qDebug()<<"original data"<<signal.mid(0,20);

    QVector<double> Fc(44); //точные значения частот фильтров, от 1 Гц до 20000 Гц,
                            //частота 1000 Гц имеет индекс 30
    for (int i=0; i<44; ++i) {
        Fc[i] = 1000.0*pow(10.0,0.1*(i-30));
    }
//    qDebug()<<Fc;

    int N = 3;  				  // Order of analysis filters.

    QVector<double> P(Fc.size());

    int i_up = 43;
    int i_low = 0;

    for (int i=Fc.size()-1; i>=0; --i) { //i_up = max(find(Fc<=samplerate/3));
        if (Fc[i]<=sampleRate/2.56) {
            i_up = i;
            break;
        }
    }

    // All filters below Fs/20 will be implemented after a decimation.
    int i_dec = 0;
    for (int i=Fc.size()-1; i>=0; --i) {
        if (Fc[i]<=sampleRate/20.0) {
            i_dec = i;
            break;
        }
    }
//    qDebug()<<QString("i_low=%1, i_dec=%2, i_up=%3").arg(i_low).arg(i_dec).arg(i_up);
    qDebug()<<QString("i_low=%1, i_dec=%2, i_up=%3").arg(Fc[i_low]).arg(Fc[i_dec]).arg(Fc[i_up]);
//    qDebug()<<"signal size" << x.size();
    // Design filters and compute RMS powers in 1/3-oct. bands.
    // Higher frequencies, direct implementation of filters.
    for (int i = i_up; i>i_dec; --i) {
        QVector<double> B, A;
        oct3dsgn(B, A, Fc[i], sampleRate, N);
//        qDebug()<<"i="<<i<<Fc[i];
//        qDebug()<<"B="<<B;
//        qDebug()<<"A="<<A;

        QVector<double> y = filter(B,A,x, QVector<double>());
        P[i] = leq(y, m_p.threshold);
    }


    // Lower frequencies, decimation by series of 3 bands.
    if (i_dec > 0) {

        QVector<double> Bu, Au;
        oct3dsgn(Bu,Au,Fc[i_dec],sampleRate/2.0,N); // Upper 1/3-oct. band in last octave.
        QVector<double> Bc, Ac;
        oct3dsgn(Bc,Ac,Fc[i_dec-1],sampleRate/2.0,N); // Center 1/3-oct. band in last octave.
        QVector<double> Bl, Al;
        oct3dsgn(Bl,Al,Fc[i_dec-2],sampleRate/2.0,N); // Lower 1/3-oct. band in last octave.
        int i = i_dec;
        int j = 1;
        //QVector<double> X;
        while (i >= i_low+2) {
            x = decimate(x,2);
            QVector<double> y = filter(Bu,Au,x, QVector<double>());
            P[i] = leq(y, m_p.threshold);
            y = filter(Bc,Ac,x, QVector<double>());
            P[i-1] = leq(y, m_p.threshold);
            y = filter(Bl,Al,x, QVector<double>());
            P[i-2] = leq(y, m_p.threshold);
            i = i-3;
            j++;
        }
        if (i == (i_low+1)) {
            x = decimate(x,2);
            QVector<double> y = filter(Bu,Au,x, QVector<double>());
            P[i] = leq(y, m_p.threshold);
            y = filter(Bc,Ac,x, QVector<double>());
            P[i-1] = leq(y, m_p.threshold);
        }
        else if (i == (i_low)) {
            x = decimate(x,2);
            QVector<double> y = filter(Bu,Au,x, QVector<double>());
            P[i] = leq(y, m_p.threshold);
        }
    }
    xValues = Fc.mid(i_low, i_up-i_low+1);
    return P.mid(i_low, i_up-i_low+1);
}
