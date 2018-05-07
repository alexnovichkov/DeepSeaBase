#ifndef OCTAVEFILTERBANK_H
#define OCTAVEFILTERBANK_H

#include <QtCore>
#include "methods/abstractmethod.h"

class OctaveFilterBank
{
public:
    OctaveFilterBank(const Parameters &p);
    QVector<float> xValues() const {return m_xValues;}
    QVector<double> compute(QVector<float> &signal, double sampleRate, QVector<double> &xValues);
private:
    QVector<float> m_xValues;
    Parameters m_p;
};

#endif // OCTAVEFILTERBANK_H
