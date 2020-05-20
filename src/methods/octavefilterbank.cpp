#include "octavefilterbank.h"

#include "iirfilter.h"
#include "resampler.h"
#include "logging.h"

#include "filtering.h"

OctaveFilterBank::OctaveFilterBank(const Parameters &p) : m_p(p)
{DD;
    thirdOctaveFreqs.resize(44); //точные значения частот третьоктавных фильтров, от 1 Гц до 20000 Гц,
                                 //частота 1000 Гц имеет индекс 30
    for (int i=0; i<44; ++i) {
        thirdOctaveFreqs[i] = 1000.0*pow(10.0,0.1*(i-30));
    }
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
template <typename T>
double leq(const QVector<T> &x, /*int T,*/ double ref=1.0)
{DD;
    const int xSize = x.size();
    T p = std::accumulate(x.constBegin(), x.constEnd(), static_cast<T>(0.0), [xSize](T v1,T v2)->T{return v1+v2*v2/xSize;});

    if (qIsFinite(p)) {
        if (p > 0.0) return 10.0*log10(p/ref/ref);
    }

    return  0.0;
}

//decimates x by factor q using libresample
QVector<double> decimate(const QVector<double> &x, int q)
{DD;
    Resampler filter(q, x.size());
    QVector<double> y = filter.process(x);
    return y;
}

QVector<double> OctaveFilterBank::compute(const QVector<double> &data, QVector<double> &xValues)
{DD;
    int N = 8;  // Order of analysis filters.
    QVector<double> x = data;

    QVector<double> P(thirdOctaveFreqs.size());

    int i_up = 43;
    int i_low = 0;

    ///i_up = max(find(Fc<=samplerate/3));
    auto idx = std::lower_bound(thirdOctaveFreqs.constBegin(), thirdOctaveFreqs.constEnd(), m_p.sampleRate/2.56);
    if (idx!=thirdOctaveFreqs.constEnd()) i_up = idx-thirdOctaveFreqs.constBegin();

    // All filters below range Fc / 200 will be implemented after a decimation.
    int i_dec = i_up;
    idx = std::lower_bound(thirdOctaveFreqs.constBegin(), thirdOctaveFreqs.cend(), m_p.sampleRate/200);
    if (idx!=thirdOctaveFreqs.cend()) i_dec = idx-thirdOctaveFreqs.cbegin();

    // Design filters and compute RMS powers in 1/3-oct. bands.
    // Higher octave band, direct implementation of filters.
    double f1 = 1.0 / pow(2.0, 1.0/6.0);
    double f2 = 1.0 * pow(2.0, 1.0/6.0);

    for (int i = i_up; i>i_dec; --i) {
        Filtering filt(x.size(), Filtering::BandPass, Filtering::ChebyshevI);

        filt.setParameters(QVector<double>()<<m_p.sampleRate<<N<<thirdOctaveFreqs[i]<<thirdOctaveFreqs[i]*(f2-f1));

        QVector<double> y = x;
        double *data = y.data();
        filt.apply(data);
        P[i] = leq(y, m_p.threshold);
    }


    // Lower frequencies, decimation by series of 3 bands.
    if (i_dec > 0) {
        x = decimate(x, 2);
    }
    for (int i=i_dec; i>=i_low; --i) {
        Filtering filt(x.size(), Filtering::BandPass, Filtering::ChebyshevI);
        filt.setParameters(QVector<double>()<<(m_p.sampleRate/2.0)<<N<<thirdOctaveFreqs[i]<<(thirdOctaveFreqs[i]*(f2-f1)));

        QVector<double> y = x;
        double *data = y.data();
        filt.apply(data);

        P[i] = leq(y, m_p.threshold);
    }
    xValues = thirdOctaveFreqs.mid(i_low, i_up-i_low+1);
    return P.mid(i_low, i_up-i_low+1);
}
