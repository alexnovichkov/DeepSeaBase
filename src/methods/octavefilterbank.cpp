#include "octavefilterbank.h"

//#include "iirfilter.h"
#include "resampler.h"
#include "logging.h"

#include "filtering.h"
#include "algorithms.h"

OctaveFilterBank::OctaveFilterBank()
{
    update();
}

OctaveFilterBank::OctaveFilterBank(OctaveType type, OctaveBase base) : type(type), base(base)
{DD;
    update();
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

//    if (qIsFinite(p)) {
//        if (p > 0.0) return 10.0*log10(p/ref/ref);
//    }

//    return  0.0;
    return p;
}

//decimates x by factor q using libresample
QVector<double> decimate(const QVector<double> &x, int q)
{DD;
    Resampler filter(q, x.size());
    QVector<double> y = filter.process(x);
    return y;
}

QVector<QVector<double>> OctaveFilterBank::compute(QVector<double> timeData, double sampleRate, double logref)
{DD;
    int N = 8;  // Order of analysis filters.
    int decimation = 10; //величина децимации

    QVector<double> P(freqs.size());

    //обрезаем список частот по частоте Найквиста
    int upperFrequency = freqs.size()-1;
    auto idx = std::lower_bound(freqs.constBegin(), freqs.constEnd(), sampleRate/2.0);
    if (idx!=freqs.constEnd()) upperFrequency = idx-freqs.constBegin()-1;


    int lowerFrequency = 0;

    // All filters below range Fc / 200 will be implemented after a decimation.
    int decimationFrequency = upperFrequency;
    idx = std::lower_bound(freqs.constBegin(), freqs.cend(), sampleRate/200);
    if (idx!=freqs.cend()) decimationFrequency = idx-freqs.cbegin()-1;
    if (decimationFrequency < 0) decimationFrequency = 0;

    //обрезаем список частот снизу, чтобы в полосу попадало хотя бы 3 отсчета
    for (int i=0; i<=upperFrequency; ++i) {
        double sr = (i <= decimationFrequency ? sampleRate/decimation : sampleRate);
        double bw = getBandWidth(freqs[i]);
//        qDebug()<<i<<freqs[i]<<sr<<bw;
        if (int(bw / (sr/blockSize)) >= 3) {
            lowerFrequency = i;
            break;
        }
    }
//    qDebug()<<i_low<<i_dec<<i_up;

    // Design filters and compute RMS powers in 1/3-oct. bands.
    // Higher octave band, direct implementation of filters.
    double f1 = 1.0 / fd;
    double f2 = 1.0 * fd;



    for (int i = upperFrequency; i>decimationFrequency; --i) {
        Filtering filt(timeData.size(), Filtering::BandPass, Filtering::ChebyshevI);
        filt.setParameters({sampleRate, double(N), freqs[i], freqs[i]*(f2-f1)});

        QVector<double> y = timeData;
        double *data = y.data();
        filt.apply(data);
        P[i] = leq(y, logref);
    }


    // Lower frequencies, decimation by series of 3 bands.
    if (decimationFrequency >= lowerFrequency) {
        timeData = decimate(timeData, decimation);
    }


    for (int i=decimationFrequency; i>=lowerFrequency; --i) {
        Filtering filt1(timeData.size(), Filtering::BandPass, Filtering::ChebyshevI);
        filt1.setParameters({sampleRate/decimation, double(N), freqs[i], freqs[i]*(f2-f1)});

        QVector<double> y = timeData;
        double *data = y.data();
        filt1.apply(data);

        P[i] = leq(y, logref);
    }
    correctedFreqs = freqs.mid(lowerFrequency, upperFrequency-lowerFrequency+1);
    return {freqs.mid(lowerFrequency, upperFrequency-lowerFrequency+1), P.mid(lowerFrequency, upperFrequency-lowerFrequency+1)};
}

void OctaveFilterBank::setType(OctaveType type)
{
    if (this->type != type) {
        this->type = type;
        update();
    }
}

void OctaveFilterBank::setBase(OctaveBase base)
{
    if (this->base != base) {
        this->base = base;
        update();
    }
}

void OctaveFilterBank::setRange(double startFreq, double endFreq)
{
    if (this->startFreq != startFreq || this->endFreq != endFreq) {
        this->startFreq = startFreq;
        this->endFreq = endFreq;
        update();
    }
}

QVector<double> OctaveFilterBank::getFrequencies(bool corrected) const
{
    if (corrected) return correctedFreqs;
    return freqs;
}

double OctaveFilterBank::getBandWidth(double frequency) const
{
    if (qFuzzyIsNull(fd)) return 0.0;

    return frequency*(fd-1.0/fd);
}

QVector<double> OctaveFilterBank::octaveStrips(int octave, int count, int base)
{
    QVector<double> v(count);
    switch (octave) {
        case 1:
            if (base == 10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, 3.0/10.0*i);
            if (base == 2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, i-10)*1000.0;
            break;
        case 2:
            if (base == 10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, 3.0/40.0*(2*i+1));
            if (base==2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, 0.25*(2*(i-20)+1))*1000.0;
            break;
        case 3:
            if (base==10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, 0.1*i);
            if (base==2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, 1.0*(i-30)/3.0)*1000.0;
            break;
        case 6:
            if (base == 10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, (2.0*i+1)/40.0);
            if (base==2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, 1.0*(2*(i-60)+1)/12.0)*1000.0;
            break;
        case 12:
            if (base == 10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, (2.0*i+1)/80.0);
            if (base==2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, 1.0*(2*(i-120)+1)/24.0)*1000.0;
            break;
        case 24:
            if (base == 10)
                for (int i=0; i<count; ++i) v[i] = pow(10.0, (2.0*i+1)/160.0);
            if (base==2)
                for (int i=0; i<count; ++i) v[i] = pow(2.0, 1.0*(2*(i-240)+1)/48.0)*1000.0;
            break;
    }
    return v;
}

void OctaveFilterBank::update()
{
    freqs.clear();
    if (endFreq < startFreq) std::swap(endFreq, startFreq);
    //if (startFreq < 1.0) startFreq = 1.0;

    int n = 0;
    switch (type) {
        case OctaveType::Octave1:
            n = 21;
            fd = (base == OctaveBase::Base2 ? sqrt(2.0) : std::pow(10.0, 0.15));
            break;
        case OctaveType::Octave2:
            n = 41;
            fd = (base == OctaveBase::Base2 ? pow(2.0, 0.25) : std::pow(10.0, 0.075));
            break;
        case OctaveType::Octave3:
            n = 61;
            fd = (base == OctaveBase::Base2 ? pow(2.0, 1.0/6.0) : std::pow(10.0, 0.05));
            break;
        case OctaveType::Octave6:
            n = 121;
            fd = (base == OctaveBase::Base2 ? pow(2.0, 1.0/12.0) : std::pow(10.0, 0.025));
            break;
        case OctaveType::Octave12:
            n = 241;
            fd = (base == OctaveBase::Base2 ? pow(2.0, 1.0/24) : std::pow(10.0, 0.0125));
            break;
        case OctaveType::Octave24:
            n = 481;
            fd = (base == OctaveBase::Base2 ? pow(2.0, 1.0/48.0) : std::pow(10.0, 0.00625));
            break;
    }

    freqs = octaveStrips(static_cast<int>(type), n, static_cast<int>(base));
    int begin = 0;
//    qDebug()<<freqs;

    auto idx = std::upper_bound(freqs.constBegin(), freqs.constEnd(), startFreq);
    if (idx!=freqs.constEnd()) begin = idx - freqs.constBegin();

    int end = freqs.length()-1;
    idx = std::lower_bound(freqs.constBegin(), freqs.constEnd(), endFreq);
    if (idx!=freqs.constEnd()) end = idx - freqs.constBegin()-1;

    freqs = freqs.mid(begin, end-begin+1);
}
