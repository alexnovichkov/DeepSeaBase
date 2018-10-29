#include "fft.h"
#include "alglib/fasttransforms.h"

QVector<cx_double> Fft::compute(const QVector<float> &input)
{
    const int n = input.size();
    QVector<double> input_double(n);
    std::copy(input.begin(), input.end(), input_double.begin());

    alglib::real_1d_array x;
    x.attach_to_ptr(n, input_double.data()); // no data copying

    alglib::complex_1d_array f;
    alglib::fftr1d(x, f);

    QVector<cx_double> output(n);
    for (int i=0; i<n; ++i) output[i] = {f[i].x, f[i].y};
    return output;
}

QVector<cx_double> Fft::compute(const QVector<cx_double> &input)
{
    const int n = input.size();
    // copying data
    alglib::complex_1d_array x;
    x.setlength(n);
    for (int i=0; i<n; ++i) x[i] = alglib::complex(input[i].real(), input[i].imag());

    alglib::fftc1d(x);

    QVector<cx_double> output(n);
    for (int i=0; i<n; ++i) output[i] = {x[i].x, x[i].y};
    return output;
}

QVector<cx_double> Fft::computeInverse(const QVector<cx_double> &input)
{
    const int n = input.size();
    // copying data
    alglib::complex_1d_array x;
    x.setlength(n);
    for (int i=0; i<n; ++i) x[i] = alglib::complex(input[i].real(), input[i].imag());

    alglib::fftc1dinv(x);

    QVector<cx_double> output(n);
    for (int i=0; i<n; ++i) output[i] = {x[i].x, x[i].y};
    return output;
}
