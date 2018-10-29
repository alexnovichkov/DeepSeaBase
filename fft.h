#ifndef FFT_H
#define FFT_H

#include "algorithms.h"

class Fft
{
public:
    /**
     * Calculate a forward transform of size n.
     *
     * ri and ii must point to the real and imaginary component arrays
     * of the input. For real input, ii may be NULL.
     *
     * ro and io must point to enough space to receive the real and
     * imaginary component arrays of the output.
     *
     * All input and output arrays are of size n.
     */
    static QVector<cx_double> compute(const QVector<float> &input);
    static QVector<cx_double> compute(const QVector<cx_double> &input);

    /**
     * Calculate an inverse transform of size n.
     *
     * ri and ii must point to the real and imaginary component arrays
     * of the input. For real input, ii may be NULL.
     *
     * ro and io must point to enough space to receive the real and
     * imaginary component arrays of the output. The output is scaled
     * by 1/n. The output pointers may not be NULL, even if the output
     * is expected to be real.
     *
     * All input and output arrays are of size n.
     */
    static QVector<cx_double> computeInverse(const QVector<cx_double> &input);
};

#endif // FFT_H
