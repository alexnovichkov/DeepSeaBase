#ifndef FILTERING_H
#define FILTERING_H

#include <QVector>

namespace Dsp {
    class Filter;
    class Params;
}

class Filtering
{
public:
    struct Parameter {
        double sampleRate;
        double frequency;
        double Q;
        double bandwidth;
        double bandwidthHz;
        double gain;
        double slope;
        double order;
        double rippleDb;
        double stopDb;
        double rolloff;
    };

    enum Type {
        NoFiltering = 0,
        LowPass=1,
        HighPass,
        BandPass,
        BandStop,
        LowShelf,
        HighShelf,
        BandShelf,
        TypeCount
    };
    enum ParamType
    {
      idSampleRate=0,
      idFrequency,
      idQ,
      idBandwidth,
      idBandwidthHz,
      idGain,
      idSlope,
      idOrder,
      idRippleDb,
      idStopDb,
      idRolloff,

      idPoleRho,
      idPoleTheta,
      idZeroRho,
      idZeroTheta,

      idPoleReal,
      idZeroReal
    };
    enum Approximation {
        Butterworth=0,
        ChebyshevI,
        ChebyshevII,
        Bessel,
        Elliptic,
        Legendre,
        RBJ,
        ApproximationCount
    };

    Filtering();
    Filtering(int blockSize, int type, int approximation);
    ~Filtering();

    double getParameter(int paramType) const;
    void setParameter(int paramType, double value);
    void setParameters(const QVector<double> &params);

    void apply(double *data);
    QVector<double> filter(const QVector<double> &data);

    int getBlockSize() const;
    void setBlockSize(int value);

    int getType() const;
    void setType(int value);

    int getApproximation() const;
    void setApproximation(int value);
    void create();
    void reset();
private:


    int blockSize;
    int type = NoFiltering;
    int approximation = Butterworth;
    Dsp::Filter* f;
};

#endif // FILTERING_H
