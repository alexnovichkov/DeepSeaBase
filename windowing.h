#ifndef WINDOWING_H
#define WINDOWING_H

#include <QtCore>

class Windowing
{
public:
    enum WindowType {
        Unknown = -1,
        Square = 0,
        Triangular = 1,
        Hann = 2,
        Hamming = 3,
        Natoll = 4,
        Gauss = 5
    };

    static QString windowDescription(int windowType) {
        switch (windowType) {
            case 0: return "Прямоуг.";
            case 1: return "Бартлетта";
            case 2: return "Хеннинга";
            case 3: return "Хемминга";
            case 4: return "Натолл";
            case 5: return "Гаусс";
        }
        return "";
    }

    Windowing(int bufferSize, int windowType);
    QVector<double> windowing() {return w;}

    void applyTo(QVector<float> &values);
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
