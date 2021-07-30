#include "filtering.h"

#include "../3rdParty/DspFilters/Dsp.h"
#include "logging.h"

Filtering::Filtering() : blockSize(0), type(NoFiltering), approximation(Butterworth), f(0)

{

}

Filtering::Filtering(int blockSize, int type, int approximation)
    : blockSize(blockSize), type(type), approximation(approximation)
      ,f(0)
{
    create();
}

Filtering::~Filtering()
{
    delete f;
}

double Filtering::getParameter(int paramType) const
{
    if (!f) return 0.0;
    int index = f->findParamId(paramType);
    if (index != -1) return f->getParam(index);
    return 0.0;
}

void Filtering::setParameter(int paramType, double value)
{
    if (!f) return;

    int index = f->findParamId(paramType);

    if (index != -1) f->setParam(index, value);
}

void Filtering::setParameters(const QVector<double> &params)
{
    Dsp::Params p = f->getDefaultParams();
    for (int i=0; i<params.size(); ++i) {
        int index = f->findParamId(i);
        if (index>-1) p[index] = params.at(i);
    }
    f->setParams(p);
}

void Filtering::apply(double *data)
{
    if (f) f->process(blockSize, &data);
}

QVector<double> Filtering::filter(const QVector<double> &data)
{
    if (!f) return data;

    QVector<double> x = data;
    double *d = x.data();
    f->process(blockSize, &(d));
    return x;
}

void Filtering::create()
{
    delete f;
    f = 0;

    switch (approximation) {
        case Butterworth: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::LowPass <50>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::HighPass <50>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::BandPass <50>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::BandStop <50>, 1, Dsp::DirectFormII>;
                    break;
                case LowShelf: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::LowShelf <50>, 1, Dsp::DirectFormII>;
                    break;
                case HighShelf: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::HighShelf <50>, 1, Dsp::DirectFormII>;
                    break;
                case BandShelf: f = new Dsp::FilterDesign<Dsp::Butterworth::Design::BandShelf <50>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            }
            break;
        }
        case ChebyshevI: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::LowPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::HighPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::BandPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::BandStop<50>, 1, Dsp::DirectFormII>;
                    break;
                case LowShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::LowShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                case HighShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::HighShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevI::Design::BandShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }
        case ChebyshevII: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::LowPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::HighPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::BandPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::BandStop<50>, 1, Dsp::DirectFormII>;
                    break;
                case LowShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::LowShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                case HighShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::HighShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandShelf: f = new Dsp::FilterDesign<Dsp::ChebyshevII::Design::BandShelf<50>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }
        case Bessel: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::Bessel::Design::LowPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::Bessel::Design::HighPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::Bessel::Design::BandPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::Bessel::Design::BandStop<25>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }
        case Elliptic: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::Elliptic::Design::LowPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::Elliptic::Design::HighPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::Elliptic::Design::BandPass<50>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::Elliptic::Design::BandStop<50>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }
        case Legendre: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::Legendre::Design::LowPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::Legendre::Design::HighPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::Legendre::Design::BandPass<25>, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::Legendre::Design::BandStop<25>, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }
        case RBJ: {
            switch (type) {
                case LowPass: f = new Dsp::FilterDesign<Dsp::RBJ::Design::LowPass, 1, Dsp::DirectFormII>;
                    break;
                case HighPass: f = new Dsp::FilterDesign<Dsp::RBJ::Design::HighPass, 1, Dsp::DirectFormII>;
                    break;
                case BandPass: f = new Dsp::FilterDesign<Dsp::RBJ::Design::BandPass2, 1, Dsp::DirectFormII>;
                    break;
                case BandStop: f = new Dsp::FilterDesign<Dsp::RBJ::Design::BandStop, 1, Dsp::DirectFormII>;
                    break;
                case LowShelf: f = new Dsp::FilterDesign<Dsp::RBJ::Design::LowShelf, 1, Dsp::DirectFormII>;
                    break;
                case HighShelf: f = new Dsp::FilterDesign<Dsp::RBJ::Design::HighShelf, 1, Dsp::DirectFormII>;
                    break;
                case BandShelf: f = new Dsp::FilterDesign<Dsp::RBJ::Design::BandShelf, 1, Dsp::DirectFormII>;
                    break;
                default: break;
            };
            break;
        }

        default: break;
    }
}

void Filtering::reset()
{
    delete f;
    f = 0;
    blockSize = 0;
    type = NoFiltering;
    approximation = Butterworth;
}

int Filtering::getApproximation() const
{
    return approximation;
}

void Filtering::setApproximation(int value)
{
    approximation = value;
}

int Filtering::getType() const
{
    return type;
}

void Filtering::setType(int value)
{
    type = value;
}

int Filtering::getBlockSize() const
{
    return blockSize;
}

void Filtering::setBlockSize(int value)
{
    blockSize = value;
}
