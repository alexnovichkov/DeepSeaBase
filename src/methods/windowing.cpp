#include "windowing.h"
#include "algorithms.h"
#include "logging.h"

QString Windowing::windowDescription(WindowType windowType)
{DD;
    switch (windowType) {
        case WindowType::Square:          return "Прямоуг.";
        case WindowType::Triangular:      return "Треугольное";
        case WindowType::Hann:            return "Ханна";
        case WindowType::Hamming:         return "Хемминга";
        case WindowType::Nuttall:         return "Натолла";
        case WindowType::Gauss:           return "Гаусса";
        case WindowType::Force:           return "Сила";
        case WindowType::Exponential:     return "Экспонента";
        case WindowType::Tukey:           return "Тьюки";
        case WindowType::Bartlett:        return "Бартлетта";
        case WindowType::Blackman:        return "Блэкмана";
        case WindowType::BlackmanNuttall: return "Блэкмана-Натолла";
        case WindowType::BlackmanHarris:  return "Блэкмана-Харриса";
        case WindowType::Flattop:         return "Flat top";
        case WindowType::Welch:           return "Уэлча";
    }
    return "";
}

QStringList Windowing::windowDescriptions()
{DD;
    return {"Прямоуг.",
        "Треугольное",
        "Ханна",
        "Хемминга",
        "Натолла",
        "Гаусса",
        "Сила",
        "Экспонента",
        "Тьюки",
        "Бартлетта",
        "Блэкмана",
        "Блэкмана-Натолла",
        "Блэкмана-Харриса",
        "Flat top",
        "Уэлча"
    };
}

QString Windowing::windowDescriptionEng(WindowType windowType)
{DD;
    switch (windowType) {
        case WindowType::Square:          return "no";
        case WindowType::Triangular:      return "triangular";
        case WindowType::Hann:            return "Hann";
        case WindowType::Hamming:         return "Hamming";
        case WindowType::Nuttall:         return "Nuttall";
        case WindowType::Gauss:           return "Gauss";
        case WindowType::Force:           return "force";
        case WindowType::Exponential:     return "exponential";
        case WindowType::Tukey:           return "Tukey";
        case WindowType::Bartlett:        return "Bartlett";
        case WindowType::Blackman:        return "Blackman";
        case WindowType::BlackmanNuttall: return "Blackman-Nuttall";
        case WindowType::BlackmanHarris:  return "Blackman-Harris";
        case WindowType::Flattop:         return "flat top";
        case WindowType::Welch:           return "Welch";
    }
    return "";
}

bool Windowing::windowAcceptsParameter(WindowType windowType)
{DD;
    switch (windowType) {
        case WindowType::Square:          return false;
        case WindowType::Triangular:      return false;
        case WindowType::Hann:            return false;
        case WindowType::Hamming:         return false;
        case WindowType::Nuttall:         return false;
        case WindowType::Gauss:           return true;
        case WindowType::Force:           return true;
        case WindowType::Exponential:     return true;
        case WindowType::Tukey:           return false;
        case WindowType::Bartlett:        return false;
        case WindowType::Blackman:        return false;
        case WindowType::BlackmanNuttall: return false;
        case WindowType::BlackmanHarris:  return false;
        case WindowType::Flattop:         return false;
        case WindowType::Welch:           return false;
    }
    return false;
}

QString Windowing::correctionDescription(Windowing::CorrectionType type)
{DD;
    switch (type) {
        case CorrectionType::NoCorrection: return "без коррекции";
        case CorrectionType::Amplitude: return "амплитудная";
        case CorrectionType::Energy: return "энергетическая";
    }
    return QString();
}

QString Windowing::correctionDescriptionEng(Windowing::CorrectionType type)
{DD;
    switch (type) {
        case CorrectionType::NoCorrection: return "no correction";
        case CorrectionType::Amplitude: return "amplitude";
        case CorrectionType::Energy: return "energy";
    }
    return QString();
}

Windowing::Windowing() :
    bufferSize(0), param(50.0)
{DD;

}

Windowing::Windowing(int bufferSize, WindowType windowType, double parameter) :
    bufferSize(bufferSize), windowType(windowType), param(parameter)
{DD;
    init();
}

void Windowing::setParameter(double parameter)
{DD;
    param = parameter;
    init();
}

void Windowing::init()
{DD;
    w.clear();
    w = QVector<double>(bufferSize, 1.0);
    correction = 1.0;

    switch (windowType) {
        case WindowType::Square:            break;
        case WindowType::Triangular:        triangular(); break;
        case WindowType::Hann:              hann(); break;
        case WindowType::Hamming:           hamming(); break;
        case WindowType::Nuttall:           nuttall(); break;
        case WindowType::Gauss:             gauss(); break;
        case WindowType::Force:             force(); break;
        case WindowType::Exponential:       exponential(); break;
        case WindowType::Tukey:             tukey(); break;
        case WindowType::Bartlett:          bartlett(); break;
        case WindowType::Blackman:          blackman(); break;
        case WindowType::BlackmanNuttall:   blackmanNuttall(); break;
        case WindowType::BlackmanHarris:    blackmanHarris(); break;
        case WindowType::Flattop:           flattop(); break;
        case WindowType::Welch:             welch(); break;
        default: break;
    }

    //normalize();
}

void Windowing::square()
{DD;
    // do nothing: window is square by default
//    for (int i=0; i<w.size(); ++i)
//        w[i] = 1.0;
}

void Windowing::hann()
{DD;
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.0; break;
        case CorrectionType::Energy: correction = 1.63; break;
    }
    const int N = w.size();

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.5 - 0.5*cos(2.0*M_PI*t));
    }
}

void Windowing::triangular()
{DD;
    const int N = w.size();

    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 1.027; break;
        case CorrectionType::Energy: correction = 0.77; break;
    }

    for (int i=0; i<N; i++) {
        double t = (2.0*i-N)/(N+1);
        w[i] = correction*(1.0 - qAbs(t));
    }
}

void Windowing::bartlett()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 1.027; break;
        case CorrectionType::Energy: correction = 0.77; break;
    }

    for (int i=0; i<N; i++) {
        double t = (2.0*i-N)/N;
        w[i] = correction*(1.0 - qAbs(t));
    }
}

void Windowing::hamming()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 1.85; break;
        case CorrectionType::Energy: correction = 1.59; break;
    }

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        //коррекция уже добавлена
        w[i] = correction*(0.54-0.46*cos(2.0*M_PI*t));
    }
}

void Windowing::nuttall()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.8; break;
        case CorrectionType::Energy: correction = 1.97; break;
    }

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.355768
                     - 0.487396*cos(2.0*M_PI*t)
                     + 0.144232*cos(4.0*M_PI*t)
                     - 0.012604*cos(8.0*M_PI*t));
    }
}

void Windowing::blackman()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.8; break;
        case CorrectionType::Energy: correction = 1.97; break;
    }

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.42
                     - 0.5*cos(2.0*M_PI*t)
                     + 0.08*cos(4.0*M_PI*t));
    }
}

void Windowing::blackmanNuttall()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.8; break;
        case CorrectionType::Energy: correction = 1.97; break;
    }

    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.3635819
                     - 0.4891775*cos(2.0*M_PI*t)
                     + 0.1365995*cos(4.0*M_PI*t)
                     - 0.0106411*cos(6.0*M_PI*t));
    }
}

void Windowing::blackmanHarris()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.8; break;
        case CorrectionType::Energy: correction = 1.97; break;
    }


    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.35875
                     - 0.48829*cos(2.0*M_PI*t)
                     + 0.14128*cos(4.0*M_PI*t)
                     - 0.01168*cos(6.0*M_PI*t));
    }
}

void Windowing::flattop()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 4.18; break;
        case CorrectionType::Energy: correction = 2.26; break;
    }

    //коэффициенты согласно Matlab, коррекция согласно TestXpress
    //max(w) = w(N/2) = 0.9
    for (int i=0; i<N; i++) {
        double t = (double)i/(N-1);
        w[i] = correction*(0.21557895
                     - 0.41663158*cos(2.0*M_PI*t)
                     + 0.277263158*cos(4.0*M_PI*t)
                     - 0.083578947*cos(6.0*M_PI*t))
                     + 0.006947368*cos(8.0*M_PI*t);
    }
}

void Windowing::gauss()
{DD;
    const int N = w.size();
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 2.02; break;
        case CorrectionType::Energy: correction = 1.63; break; //НЕПРАВИЛЬНОЕ ЗНАЧЕНИЕ!
    }
    if (param <=0.0) param = 0.001;
    if (param > 50) param = 50;

    for (int i=0; i<N; i++) {
        double t = 2.0*i/(N-1);
        w[i] = correction*exp(-0.5*pow((t-1.0)/param/100.0, 2.0));
    }
}

void Windowing::force()
{DD;
    //коррекция не используется
    if (param > 100.0) param = 100.0;
    if (param < 0.0) param = 0.0;

    const int N = w.size();
    int M = int(N*param/100);
    int delta = N/50; // 2% переходный процесс
    for (int i=M; i<M+delta; ++i) w[i] = 0.5+0.5*cos(M_PI*(i-M)/delta);
    for (int i=M+delta; i<N; ++i) w[i] = 0.0;
}

void Windowing::welch()
{DD;
    const int N = w.size();

    for (int i=0; i<N; ++i) w[i] = 1 - qPow(2.0*(i-N*0.5)/N, 2.0);
}

void Windowing::tukey()
{DD;
    const int N = w.size();
    if (param > 100.0) param = 100.0;
    if (param < 0.0) param = 0.0;
    switch (correctionType) {
        case CorrectionType::NoCorrection: correction = 1.0; break;
        case CorrectionType::Amplitude: correction = 1.26; break;
        case CorrectionType::Energy: correction = 1.17; break;
    }

    int delta = N/10; // 10% переходный процесс
    for (int i = 0; i<delta; ++i)  {
        w[i] = correction*0.5*(1-cos(2.0*M_PI*i*5/N));
        w[N-1-i] = w[i];
    }
}

/*
 * Старый симметричный вариант
void Windowing::exponential()
{DD;
    //имеет график, симметричный относительно N/2
    const int N = w.size();
//    if (param > 100.0) param = 100.0;
    if (param <= 0.0) param = 1.0;

    //param выражает требуемую величину затухания, дБ
    const double tau = 4.345*(N-1.0)/param;
    for (int i=0; i<N; i++) {
        double t = (double)i-(N-1)*0.5;
        w[i] = exp(-1.0*qAbs(t)/tau);
    }
}*/

void Windowing::exponential()
{DD;
    //коррекция не используется

    const int N = w.size();
    if (param > 100.0) param = 100.0;
    if (param <= 0.0) param = 1.0;

    const double alpha = log(param/100);
    for (int i=0; i<N; i++) {
        double t = (double)i/(N);
        w[i] = exp(t*alpha);
    }
}

void Windowing::normalize()
{DD;
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
{DD;
    return bufferSize;
}

void Windowing::setBufferSize(int value)
{DD;
    bufferSize = value;
    init();
}

Windowing::WindowType Windowing::getWindowType() const
{DD;
    return windowType;
}

void Windowing::setWindowType(WindowType value)
{DD;
    windowType = value;
    init();
}

void Windowing::setCorrectionType(Windowing::CorrectionType correctionType)
{DD;
    this->correctionType = correctionType;
    init();
}


void Windowing::applyTo(QVector<double> &values, int blockSize)
{DD;
    //blockSize == 0 если применяем окно порциями, тогда values.size даст blockSize
    //если blockSize > 0, то values содержат все отсчеты канала

    //если применяем окно порциями, а не целиком канал
    if (blockSize == 0 && w.size() != values.size()) {
        setBufferSize(values.size());
    }

    Q_ASSERT_X(bufferSize == blockSize, "Windowing::applyTo", "block size != window size");

    int blocksCount = values.size()/bufferSize;
    int lastBlockSize = values.size() % bufferSize;
    for (int block = 0; block < blocksCount; ++block) {
        for (int i=0; i<bufferSize; ++i) {
            values[i + block*bufferSize] *= w[i];
        }
    }
    if (lastBlockSize > 0) {
        for (int i=0; i<lastBlockSize; ++i)
            values[i + blocksCount*bufferSize] *= w[i];
    }
}

//double Windowing::amplitudeCorrection() const
//{DD;
//    double result = 0.0;
//    for (double x: w) result += std::abs(x);
//    if (w.size()>0) result /= w.size();
//    return result * 2;
//}

//double Windowing::energyCorrection() const
//{DD;
//    double result = 0.0;
//    for (double x: w) result += x*x;

//    return result;
//}



