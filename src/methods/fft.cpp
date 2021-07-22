#include "fft.h"
//#include "../3rdParty/alglib/fasttransforms.h"
#include "fftw3.h"

QVector<cx_double> Fft::compute(const QVector<double> &input)
{
//    QVector<cx_double> a = computeWithAlgLib(input);
//    QVector<cx_double> b = computeWithFftw(input);

    return computeWithFftw(input);
//    return computeWithAlgLib(input);
//    return compute1(input);
}

QVector<cx_double> Fft::computeWithFftw(const QVector<double> &input)
{
    double *in;
    fftw_complex *out;
    static fftw_plan p;
    static int oldN = 0;

    const int N = input.size();

    in = const_cast<double*>(input.data());
    QVector<cx_double> output(N);
    out = reinterpret_cast<fftw_complex*>(output.data());
    if (oldN != N)  {
        p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE | FFTW_UNALIGNED);
        oldN = N;
    }

    fftw_execute_dft_r2c(p, in, out);

//    fftw_destroy_plan(p);
    return output;
}

//QVector<cx_double> Fft::computeWithAlgLib(const QVector<double> &input)
//{
//    const int n = input.size();
//    QVector<double> y = input;

//    alglib::real_1d_array x;
//    x.attach_to_ptr(n, y.data()); // no data copying

//    alglib::complex_1d_array f;
//    alglib::fftr1d(x, f);

//    QVector<cx_double> output(n);
//    for (int i=0; i<n; ++i) output[i] = {f[i].x, f[i].y};
//    return output;
//}

QVector<cx_double> Fft::compute1(const QVector<double> &input)
{
    int Nvl = input.size();

    int i, j, n, m, Mmax, Istp;
    double Tmpr, Tmpi, Wtmp, Theta;
    double Wpr, Wpi, Wr, Wi;
    QVector<double> Tmvl;

    n = Nvl * 2;
    Tmvl = QVector<double>(n, 0.0);

    for (i = 0; i < n; i+=2) {
        Tmvl[i] = 0;
        Tmvl[i+1] = input[i/2];
    }

    i = 1; j = 1;
    while (i < n) {
        if (j > i) {
            Tmpr = Tmvl[i]; Tmvl[i] = Tmvl[j]; Tmvl[j] = Tmpr;
            Tmpr = Tmvl[i+1]; Tmvl[i+1] = Tmvl[j+1]; Tmvl[j+1] = Tmpr;
        }
        i = i + 2; m = Nvl;
        while ((m >= 2) && (j > m)) {
            j = j - m; m = m >> 1;
        }
        j = j + m;
    }

    Mmax = 2;
    while (n > Mmax) {
        Theta = -6.283185307179586 / Mmax; Wpi = sin(Theta);
        Wtmp = sin(Theta / 2); Wpr = Wtmp * Wtmp * 2;
        Istp = Mmax * 2; Wr = 1; Wi = 0; m = 1;

        while (m < Mmax) {
            i = m; m = m + 2; Tmpr = Wr; Tmpi = Wi;
            Wr = Wr - Tmpr * Wpr - Tmpi * Wpi;
            Wi = Wi + Tmpr * Wpi - Tmpi * Wpr;

            while (i < n) {
                j = i + Mmax;
                Tmpr = Wr * Tmvl[j] - Wi * Tmvl[j-1];
                Tmpi = Wi * Tmvl[j] + Wr * Tmvl[j-1];

                Tmvl[j] = Tmvl[i] - Tmpr; Tmvl[j-1] = Tmvl[i-1] - Tmpi;
                Tmvl[i] = Tmvl[i] + Tmpr; Tmvl[i-1] = Tmvl[i-1] + Tmpi;
                i = i + Istp;
            }
        }

        Mmax = Istp;
    }

    QVector<cx_double> result = QVector<cx_double>(Nvl);
    for (i = 0; i < Nvl; i++) {
        result[Nvl-i-1] = {Tmvl[2*i+1], Tmvl[2*i]};
    }
    return result;
}

//QVector<double> Fft::computeReal(const QVector<double> &input)
//{
//    const int n = input.size();
//    QVector<double> y = input;

//    alglib::real_1d_array x;
//    x.attach_to_ptr(n, y.data()); // no data copying

//    alglib::complex_1d_array f;
//    alglib::fftr1d(x, f);

//    QVector<double> output(n*2);
//    for (int i=0; i<n; ++i) {
//        output[i*2] = f[i].x;
//        output[i*2+1] = f[i].y;
//    }
//    return output;
//}

QVector<cx_double> Fft::computeComplex(const QVector<cx_double> &input)
{
//    const int n = input.size();
//    // copying data
//    alglib::complex_1d_array x;
//    x.setlength(n);
//    for (int i=0; i<n; ++i) x[i] = alglib::complex(input[i].real(), input[i].imag());

//    alglib::fftc1d(x);

//    QVector<cx_double> output(n);
//    for (int i=0; i<n; ++i) output[i] = {x[i].x, x[i].y};
//    return output;


    fftw_complex *in;
    fftw_complex *out;
    static fftw_plan p;
    static int oldN = 0;

    const int N = input.size();

    in = reinterpret_cast<fftw_complex*>(const_cast<cx_double*>(input.data()));
    QVector<cx_double> output(N);
    out = reinterpret_cast<fftw_complex*>(output.data());
    if (oldN != N)  {
        p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE | FFTW_UNALIGNED);
        oldN = N;
    }

    fftw_execute_dft(p, in, out);

//    fftw_destroy_plan(p);
    return output;
}

QVector<cx_double> Fft::computeInverse(const QVector<cx_double> &input)
{
//    const int n = input.size();
//    // copying data
//    alglib::complex_1d_array x;
//    x.setlength(n);
//    for (int i=0; i<n; ++i) x[i] = alglib::complex(input[i].real(), input[i].imag());

//    alglib::fftc1dinv(x);

//    QVector<cx_double> output(n);
//    for (int i=0; i<n; ++i) output[i] = {x[i].x, x[i].y};
//    return output;

    fftw_complex *in;
    fftw_complex *out;
    static fftw_plan p;
    static int oldN = 0;

    const int N = input.size();

    in = reinterpret_cast<fftw_complex*>(const_cast<cx_double*>(input.data()));
    QVector<cx_double> output(N);
    out = reinterpret_cast<fftw_complex*>(output.data());
    if (oldN != N)  {
        p = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE | FFTW_UNALIGNED);
        oldN = N;
    }

    fftw_execute_dft(p, in, out);

//    fftw_destroy_plan(p);
    return output;
}
