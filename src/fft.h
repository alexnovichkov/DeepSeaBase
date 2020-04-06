#ifndef FFT_H
#define FFT_H

#include "algorithms.h"
#include "fftw3.h"

class Fft
{
public:
    /**
     * Calculate a forward transform of size n.
     *
     * All input and output arrays are of size n.
     */
    static QVector<cx_double> compute(const QVector<double> &input);

    static QVector<cx_double> computeWithFftw(const QVector<double> &input);

    //реализация, использующая AlgLib
//    static QVector<cx_double> computeWithAlgLib(const QVector<double> &input);

    //прямая реализация FFT
    static QVector<cx_double> compute1(const QVector<double> &input);

    // returns a vector of length n*2 that contains both re and im parts
    // in the form [re0, im0, re1, im1, re2, im2...]
//    static QVector<double> computeReal(const QVector<double> &input);
    static QVector<cx_double> computeComplex(const QVector<cx_double> &input);

    /**
     * Calculate an inverse transform of size n.
     *
     * The output is scaled
     * by 1/n. The output pointers may not be NULL, even if the output
     * is expected to be real.
     *
     * All input and output arrays are of size n.
     */
    static QVector<cx_double> computeInverse(const QVector<cx_double> &input);
//private:
//    fftw_plan p;
};

#endif // FFT_H
