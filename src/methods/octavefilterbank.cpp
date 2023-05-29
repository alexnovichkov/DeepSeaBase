#include "octavefilterbank.h"

//#include "iirfilter.h"
#include "resampler.h"
#include "logging.h"

#include "filtering.h"
#include "algorithms.h"


OctaveFilterBank::OctaveFilterBank()
{DD;
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
    Q_UNUSED(ref);
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
    int decimationFactor = 200; // All filters below range Fc/decimationFactor will be implemented after a decimation.

    QVector<double> P(freqs.size());

//    //use only first 2 sec of data
//    int impulseSize = qMin(int(sampleRate/2.0), timeData.size());
//    timeData.resize(impulseSize);

    //обрезаем список частот по частоте Найквиста
    int upperFrequency = freqs.size()-1;
    auto idx = std::lower_bound(freqs.constBegin(), freqs.constEnd(), sampleRate/2.0);
    if (idx!=freqs.constEnd()) upperFrequency = idx-freqs.constBegin()-1;


    int lowerFrequency = 0;

    // All filters below range Fc/decimationFactor will be implemented after a decimation.
    int decimationFrequency = upperFrequency;
    idx = std::lower_bound(freqs.constBegin(), freqs.cend(), sampleRate/decimationFactor);
    if (idx!=freqs.cend()) decimationFrequency = idx-freqs.cbegin()-1;
    if (decimationFrequency < 0) decimationFrequency = 0;

    //обрезаем список частот снизу, чтобы в полосу попадало хотя бы 3 отсчета
//    for (int i=0; i<=upperFrequency; ++i) {
//        double sr = (i <= decimationFrequency ? sampleRate/decimation : sampleRate);
//        double bw = getBandWidth(freqs[i]);
//        if (int(bw / (sr/blockSize)) >= 3) {
//            lowerFrequency = i;
//            break;
//        }
//    }

    // Design filters and compute RMS powers in 1/3-oct. bands.
    // Higher octave band, direct implementation of filters.
    double f1 = 1.0 / fd;
    double f2 = 1.0 * fd;


    // computing bands without decimation
    for (int i = upperFrequency; i>decimationFrequency; --i) {
        Filtering filt(timeData.size(), Filtering::BandPass, Filtering::ChebyshevI);
        filt.setParameters({sampleRate,
                            freqs[i]*(f2+f1)/2.0, //frequency
                            0,// Q
                            0,// bandwidth
                            freqs[i]*(f2-f1),// bandwidthHz
                            0,// gain
                            0,// slope
                            double(N)//order
                           });

        QVector<double> y = timeData;
        double *data = y.data();
        filt.apply(data);
        P[i] = leq(y, logref);
    }


    //Lower frequencies, decimation
    if (decimationFrequency >= lowerFrequency) {
        timeData = decimate(timeData, decimation);
        sampleRate /= decimation;
    }

    for (int i=decimationFrequency; i>=lowerFrequency; --i) {
        Filtering filt1(timeData.size(), Filtering::BandPass, Filtering::ChebyshevI);
        filt1.setParameters({sampleRate,
                             freqs[i]*(f2+f1)/2.0, //frequency
                             0,// Q
                             0,// bandwidth
                             freqs[i]*(f2-f1),// bandwidthHz
                             0,// gain
                             0,// slope
                             double(N)//order
                            });

        QVector<double> y = timeData;
        double *data = y.data();
        filt1.apply(data);

        P[i] = leq(y, logref);
    }

    correctedFreqs = freqs.mid(lowerFrequency, upperFrequency-lowerFrequency+1);
    return {freqs.mid(lowerFrequency, upperFrequency-lowerFrequency+1), P.mid(lowerFrequency, upperFrequency-lowerFrequency+1)};
}

void OctaveFilterBank::setType(OctaveType type)
{DD;
    if (this->type != type) {
        this->type = type;
        update();
    }
}

void OctaveFilterBank::setBase(OctaveBase base)
{DD;
    if (this->base != base) {
        this->base = base;
        update();
    }
}

void OctaveFilterBank::setRange(double startFreq, double endFreq)
{DD;
    if (this->startFreq != startFreq || this->endFreq != endFreq) {
        this->startFreq = startFreq;
        this->endFreq = endFreq;
        update();
    }
}

QVector<double> OctaveFilterBank::getFrequencies(bool corrected) const
{DD;
    if (corrected) return correctedFreqs;
    return freqs;
}

double OctaveFilterBank::getBandWidth(double frequency) const
{DD;
    if (qFuzzyIsNull(fd)) return 0.0;

    return frequency*(fd-1.0/fd);
}

QPair<double, double> OctaveFilterBank::getBandBorders(int index, const QVector<double> &data, OctaveType octaveType)
{DD;
    if (data.isEmpty()) return {0,0};
    double factor = 2.0;
    switch (octaveType) {
        case OctaveType::Octave1: factor = pow(10.0, 0.15); break;
        case OctaveType::Octave2: factor = pow(10.0, 0.075); break;
        case OctaveType::Octave3: factor = pow(10.0, 0.05); break;
        case OctaveType::Octave6: factor = pow(10.0, 0.025); break;
        case OctaveType::Octave12: factor = pow(10.0, 0.0125); break;
        case OctaveType::Octave24: factor = pow(10.0, 0.00625); break;
        default: break;
    }

    if (data.size() == 1) return {data[0] / factor, data[0]*factor};
    if (index == 0) return {data[0] / factor, qSqrt(data[0]*data[1])};
    if (index == data.size()-1) return {qSqrt(data[index]*data[index-1]), data[index]*factor};
    return {qSqrt(data[index]*data[index-1]), qSqrt(data[index]*data[index+1])};
}

OctaveType OctaveFilterBank::guessOctaveType(const QVector<double> &data)
{DD;
    double step = 0;
    for (int i=0; i<data.size()-1; ++i) {
        step += data[i+1]/data[i];
    }
    step /=data.size()-1;

    if (step >= 1.9 && step <= 2.1) return OctaveType::Octave1;
    if (step >= 1.39 && step <= 1.43) return OctaveType::Octave2;
    if (step >= 1.23 && step <= 1.3) return OctaveType::Octave3;
    if (step >= 1.11 && step <= 1.14) return OctaveType::Octave6;
    if (step >= 1.05 && step <= 1.07) return OctaveType::Octave12;
    if (step >= 1.02 && step <= 1.04) return OctaveType::Octave24;
    return OctaveType::Unknown;
}

QVector<double> OctaveFilterBank::octaveStrips(int octave, int count, int base)
{DD;
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

double round_off(double N, double n)
{DD;
    int h;
    double b, d, e, i, j, m, f;
    b = N;

    // Counting the no. of digits to the left of decimal point
    // in the given no.
    for (i = 0; b >= 1; ++i)
        b = b / 10;

    d = n - i;
    b = N;
    b = b * pow(10, d);
    e = b + 0.5;
    if ((float)e == (float)ceil(b)) {
        f = (ceil(b));
        h = f - 2;
        if (h % 2 != 0) {
            e = e - 1;
        }
    }
    j = floor(e);
    m = pow(10, d);
    j = j / m;
    return j;
}

int leftmostDigit(double d)
{DD;
    while (d > 10) d /= 10;
    return std::floor(d);
}

QString roundedLabel(double frequency)
{DD;
    if (auto d = leftmostDigit(frequency); d >= 1 && d <= 4 ) {
        return QString::number(round_off(frequency, 3));
    }

    return QString::number(round_off(frequency, 2));
}

void OctaveFilterBank::update()
{DD;
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
        default: break;
    }

    freqs = octaveStrips(static_cast<int>(type), n, static_cast<int>(base));
    int begin = 0;
//    LOG(DEBUG)<<freqs;

    auto idx = std::upper_bound(freqs.constBegin(), freqs.constEnd(), startFreq);
    if (idx!=freqs.constEnd()) begin = idx - freqs.constBegin();

    int end = freqs.length()-1;
    idx = std::lower_bound(freqs.constBegin(), freqs.constEnd(), endFreq);
    if (idx!=freqs.constEnd()) end = idx - freqs.constBegin()-1;

    freqs = freqs.mid(begin, end-begin+1);
}
