#include "windowing.h"
#include "algorithms.h"

QString Windowing::windowDescription(int windowType)
{
    switch (windowType) {
        case 0: return "Прямоуг.";
        case 1: return "Бартлетта";
        case 2: return "Хеннинга";
        case 3: return "Хемминга";
        case 4: return "Натолл";
        case 5: return "Гаусс";
        case 6: return "Сила";
        case 7: return "Экспонента";
        case 8: return "Тьюки";
    }
    return "";
}

bool Windowing::windowAcceptsParameter(int windowType)
{
    switch (windowType) {
        case Square: return false;
        case Triangular: return false;
        case Hann: return false;
        case Hamming: return false;
        case Natoll: return false;
        case Gauss: return false;
        case Force: return true;
        case Exponential: return true;
        case Tukey: return false;
    }
    return false;
}

Windowing::Windowing() :
    bufferSize(0), windowType(Hann), param(50.0)
{

}

Windowing::Windowing(int bufferSize, int windowType, double parameter) :
    bufferSize(bufferSize), windowType(windowType), param(parameter)
{
    init();
}

void Windowing::setParameter(double parameter)
{
    param = parameter;
    init();
}

void Windowing::init()
{
    w.clear();
    w = QVector<double>(bufferSize, 1.0);

    switch (windowType) {
        case Square: square(); break;
        case Triangular: triangular(); break;
        case Hann: hann(); break;
        case Hamming: hamming(); break;
        case Natoll: natoll(); break;
        case Gauss: gauss(); break;
        case Force: force(); break;
        case Tukey: tukey(); break;
        case Exponential: exponential(); break;
        default: break;
    }
    //normalize();
}

void Windowing::square()
{
    // do nothing: window is square by default
//    for (int i=0; i<w.size(); ++i)
//        w[i] = 1.0;
}

void Windowing::hann()
{
    const int N = w.size();
    //const double correction = 2.0;

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        //коррекция уже добавлена
        w[i] = 1.0 - cos(2.0*M_PI*t);
    }
}

void Windowing::triangular()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 2.0 - 4.0*qAbs(t);
    }
}

void Windowing::hamming()
{
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = 1.85*(0.54+0.46*cos(2.0*M_PI*t));
    }
}

void Windowing::natoll()
{//
    const int N = w.size();
    const double correction = 2.75;

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = correction*(0.3635819
                     + 0.4891775*cos(2.0*M_PI*t)
                     + 0.1365995*cos(2.0*M_PI*t*2.0)
                     + 0.0106411*cos(2.0*M_PI*t*3.0));
    }
}

void Windowing::gauss()
{
    const int N = w.size();
    const double correction = 2.02;
    const double alpha = 2.5;

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1) - 0.5;
        w[i] = correction*exp(-0.5*pow(2.0*alpha*t,2.0));
    }
}

void Windowing::force()
{
    if (param > 100.0) param = 100.0;
    if (param < 0.0) param = 0.0;

    const int N = w.size();
    int M = int(N*param/100);
    int delta = N*5/256; // 2% переходный процесс
    for (int i=M; i<M+delta; ++i) w[i] = cos(M_PI/2.0*(i-M)/delta);
    for (int i=M+delta; i<N; ++i) w[i] = 0.0;
}

void Windowing::tukey()
{
    const int N = w.size();
    if (param > 100.0) param = 100.0;
    if (param < 0.0) param = 0.0;

    QVector<double> v(N/2);
    for (int i=0; i<v.size(); ++i) {
        v[i]=double(i)/double(N-1);
        v[i]=(1.0+cos(M_PI*(2.0*v[i]/param/100-1.0)))/2.0;
    }
    for (int i=v.size()-1; i>=1; --i) {
        if (v[i]<v[i-1]) v.removeLast();
        else break;
    }
    for (int i=0; i<v.size(); ++i) {
        w[i] = v[i];
        w[N-1-i]=v[i];
    }
    //qDebug()<<w;
}

void Windowing::exponential()
{
    const int N = w.size();
    if (param > 100.0) param = 100.0;
    if (param < 0.0) param = 0.0;

    const double alpha = -1.0*log(param/100.0);
    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = exp(-1.0*alpha*t);
    }
}

void Windowing::normalize() {
  const int size = w.size();
  double sum = 0.0;
  for (int i=0; i<size; i++) {
    sum += std::abs(w[i]);
  }

  if (qFuzzyIsNull(sum)) {
    return;
  }

  // as we have half of the energy in negative frequencies, we need to scale, but
  // multiply by two. Otherwise a sinusoid at 0db will result in 0.5 in the spectrum.
  double scale = 2.0 / sum;

  for (int i=0; i<size; i++) {
    w[i] *= scale;
  }
}

int Windowing::getBufferSize() const
{
    return bufferSize;
}

void Windowing::setBufferSize(int value)
{
    bufferSize = value;
    init();
}

int Windowing::getWindowType() const
{
    return windowType;
}

void Windowing::setWindowType(int value)
{
    windowType = value;
    init();
}


void Windowing::applyTo(QVector<double> &values)
{
    if (w.size() != values.size()) {
        setBufferSize(values.size());
    }

    const int min = qMin(values.size(), w.size());
    for (int i=0; i<min; ++i)
        values[i] *= w[i];
}



