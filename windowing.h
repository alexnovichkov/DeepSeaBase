#ifndef WINDOWING_H
#define WINDOWING_H

#include <QtCore>

class Windowing
{
public:
    Windowing(int windowType, int block);
    QVector<double> windowing() {return w;}
private:
    void square();
    void hann();
    void triangular();
    void hamming();
    void natoll();
    void gauss();

    QVector<double> w;
};

#endif // WINDOWING_H
