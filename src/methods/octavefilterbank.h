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

enum class OctaveFrequency {
    Exact,
    Smoothed
};

class OctaveFilterBank
{
public:
    OctaveFilterBank();
    OctaveFilterBank(OctaveType type, OctaveBase base);
    //QVector<double> compute(QVector<double> timeData, QVector<double> &xValues);

    //возвращает вектор {X, L}
    QVector<QVector<double>> compute(QVector<double> timeData, double sampleRate, double logref);

    OctaveType getType() const {return type;}
    void setType(OctaveType type);

    OctaveBase getBase() const {return base;}
    void setBase(OctaveBase base);

    QPair<double,double> getRange() const {return {startFreq, endFreq};}
    void setRange(double startFreq, double endFreq);

    void setBlockSize(int blockSize) {this->blockSize = blockSize;}

    QVector<double> getFrequencies(bool corrected) const;

    //возвращает ширину полосы f2-f1 = fm*fd-fm/fd, fm - центральная частота полосы
    double getBandWidth(double frequency) const;



    //низкоуровневая функция, которая расчитывает нужное количество октавных полос, начиная с 1 Гц
    static QVector<double> octaveStrips(int octave, int count, int base = 10);
    static QString frequencyLabel(int octave, double frequency);
private:
    //обновление списка центральных частот без обрезки по частоте Найквиста
    void update();

    OctaveType type = OctaveType::Octave3;
//    double sampleRate = 8192;
//    double logref = 1.0;
    double startFreq = 0.0;
    double endFreq = 40000.0;
    double fd = 1.0; //половина ширины полосы
    int blockSize = 65536; //количество отсчетов в одном блоке
    OctaveBase base = OctaveBase::Base10;
    QVector<double> freqs;
    QVector<double> correctedFreqs;
};

#endif // OCTAVEFILTERBANK_H
