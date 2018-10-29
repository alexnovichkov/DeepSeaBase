#ifndef OCTAVEFILTERBANK_H
#define OCTAVEFILTERBANK_H

#include <QtCore>
#include "methods/abstractmethod.h"

class OctaveFilterBank
{
public:
    OctaveFilterBank(const Parameters &p);
    QVector<double> compute(QVector<float> &signal, QVector<double> &xValues);
private:
    Parameters m_p;
    QVector<double> thirdOctaveFreqs;
};

#endif // OCTAVEFILTERBANK_H
