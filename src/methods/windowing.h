#ifndef WINDOWING_H
#define WINDOWING_H

#include <QtCore>

class Windowing
{
public:
    enum class WindowType {
        Square = 0,
        Triangular = 1,
        Hann = 2,
        Hamming = 3,
        Nuttall = 4,
        Gauss = 5,
        Force = 6,
        Exponential = 7,
        Tukey = 8,
        Bartlett = 9,
        Blackman = 10,
        BlackmanNuttall = 11,
        BlackmanHarris = 12,
        Flattop = 13,
        Welch = 14
    };

    static QString windowDescription(WindowType windowType);
    static QStringList windowDescriptions();
    static QString windowDescriptionEng(WindowType windowType);
    static bool windowAcceptsParameter(WindowType windowType);

    Windowing();
    Windowing(int bufferSize, WindowType windowType, double parameter = 0.0);

    void applyTo(QVector<double> &values);

    QVector<double> windowing() {return w;}

    void setParameter(double parameter);
    double getParameter() const {return param;}

    WindowType getWindowType() const;
    void setWindowType(WindowType value);

    int getBufferSize() const;
    void setBufferSize(int value);

private:
    void init();
    void square();
    void hann();
    void triangular();
    void hamming();
    void nuttall();
    void gauss();
    void force();
    void tukey();
    void exponential();
    void bartlett();
    void blackman();
    void blackmanNuttall();
    void blackmanHarris();
    void flattop();
    void welch();
    void normalize();

    QVector<double> w;
    int bufferSize;
    WindowType windowType = WindowType::Square;
    double param;
};

#endif // WINDOWING_H
