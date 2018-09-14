#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>

QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep);
int uffWindowType(int dfdWindowType);
int uffMethodFromDfdMethod(int methodId);

#endif // ALGORITHMS_H
