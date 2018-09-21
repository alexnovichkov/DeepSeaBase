#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>

QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep);
int uffWindowType(int dfdWindowType);
int uffMethodFromDfdMethod(int methodId);

// возвращает округленные значения переменных from и to, которые различаются как минимум в первой значащей цифре
void getUniqueFromToValues(QString &fromString, QString &toString, double from, double to);

#endif // ALGORITHMS_H
