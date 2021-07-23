#ifndef OCTAVEFILTERBANK_H
#define OCTAVEFILTERBANK_H

#include <QtCore>
//#include "methods/dfdmethods/abstractmethod.h"

enum class OctaveType {
    Octave1 = 1,
    Octave3 = 3,
    Octave2 = 2,
    Octave6 = 6,
    Octave12 = 12,
    Octave24 = 24
};

enum /*class*/ OctaveBase {
    Base2  = 2,
    Base10 = 10
};

class OctaveFilterBank
{
public:
    OctaveFilterBank(OctaveType type, OctaveBase base);
    //QVector<double> compute(QVector<double> timeData, QVector<double> &xValues);

    QVector<QVector<double>> compute(QVector<double> timeData, double sampleRate, double logref);

    OctaveType getType() const {return type;}
    void setType(OctaveType type);

    OctaveBase getBase() const {return base;}
    void setBase(OctaveBase base);

    QPair<double,double> getRange() const {return {startFreq, endFreq};}
    void setRange(double startFreq, double endFreq);

    QVector<double> getFrequencies() const {return freqs;}

    //низкоуровневая функция, которая рассчитывает нужное количество октавных полос, начиная с 1 Гц
    static QVector<double> octaveStrips(int octave, int count, int base = 10);
private:
    //обновление списка центральных частот без обрезки по частоте Найквиста
    void update();

    OctaveType type = OctaveType::Octave3;
//    double sampleRate = 8192;
//    double logref = 1.0;
    double startFreq = 20.0;
    double endFreq = 20000.0;
    double fd = 1.0; //половина ширины полосы
    OctaveBase base = OctaveBase::Base10;
    QVector<double> freqs;
};

#endif // OCTAVEFILTERBANK_H
