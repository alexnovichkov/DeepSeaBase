#ifndef IIRFILTER_H
#define IIRFILTER_H

#include <QtCore>

class Parameters;

void butter(QVector<double> &B, QVector<double> &A, int N, double W1, double W2);
void cheby1(QVector<double> &B, QVector<double> &A, int n, double Rp, double W);
QVector<double> filter(const QVector<double> &B, const QVector<double> &A,
                       const QVector<double> &x,
                       const QVector<double> &si);
QVector<double> filtfilt(const QVector<double> &B, const QVector<double> &A, const QVector<double> &x);
QVector<double> sinetone(double freq, double rate, double sec, double ampl);

#endif // IIRFILTER_H
