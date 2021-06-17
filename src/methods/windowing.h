#ifndef WINDOWING_H
#define WINDOWING_H

#include <QtCore>

class Windowing
{
public:
    enum WindowType {
        Square = 0,
        Triangular = 1,
        Hann = 2,
        Hamming = 3,
        Natoll = 4,
        Gauss = 5,
        Force = 6,
        Exponential = 7,
        Tukey = 8,
        WindowCount
    };

    static QString windowDescription(int windowType);
    static bool windowAcceptsParameter(int windowType);

    Windowing();
    Windowing(int bufferSize, int windowType, double parameter = 0.0);

    void applyTo(QVector<double> &values);

    QVector<double> windowing() {return w;}

    void setParameter(double parameter);
    double getParameter() const {return param;}

    int getWindowType() const;
    void setWindowType(int value);

    int getBufferSize() const;
    void setBufferSize(int value);

private:
    void init();
    void square();
    void hann();
    void triangular();
    void hamming();
    void natoll();
    void gauss();
    void force();
    void tukey();
    void exponential();
    void normalize();

    QVector<double> w;
    int bufferSize;
    int windowType;
    double param;
};

#endif // WINDOWING_H
