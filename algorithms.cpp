#include "algorithms.h"


QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep)
{
    QPair<QVector<double>, QVector<double> > result;
    // определяем верхнюю границу данных
    const double F_min = xBegin;
    const double Step_f = xStep;
    const double F_max = F_min + Step_f * spectrum.size();

    const double twothird = pow(2.0,1.0/3.0);
    const double tenpow = pow(10.0, -1.4);
    const double twosixth = pow(2.0,1.0/6.0);

   // double f_lower = 0;
    double f_upper = 0;
    double f_median = 0;
    int k_max = 52;
    for (int k = 52; k>=0; --k) {
        f_median = pow(10.0, 0.1 * k);
       // f_lower = f_median / twosixth;
        f_upper = f_median * twosixth;
        if (f_upper <= F_max) {
            k_max = k;
            break;
        }
    }
    if (f_median == 1) return result;

    k_max++;

    QVector<double> xValues(k_max);
    for (int k = 0; k < k_max; ++k)
        xValues[k] = pow(10.0, 0.1*k);

    QVector<double> values(k_max);

    for (int k = 0; k < k_max; ++k) {
        // определяем диапазон в данном фильтре и число отсчетов для обработки
        int i_min = -1;
        int i_max = -1;
        f_median = xValues.at(k);

        double f_lower = f_median / twosixth;
        double f_upper = f_median * twosixth;

        for (int i = 0; i<spectrum.size(); ++i) {
            double f = F_min+i*Step_f;
            if (std::abs(f-f_lower) <= Step_f) {
                i_min = i;
                break;
            }
        }
        for (int i = spectrum.size()-1; i>=0; --i) {
            double f = F_min+i*Step_f;
            if (std::abs(f-f_upper) <= Step_f) {
                i_max = i;
                break;
            }
        }

        int steps = i_max - i_min + 1;
        double sum_i = 0.0;
        if (steps >= 5) {
            for (int i = i_min; i <= i_max; ++i) {
                double L = spectrum.at(i);
                double ratio = (F_min+i*Step_f)/f_median;
                double coef = 1.0;
                if (ratio>0.5 && ratio<=(1.0/twothird))
                    coef = tenpow/(1.0/twothird-0.5)*(ratio-0.5);
                else if (ratio>(1.0/twothird) && ratio<=(1.0/twosixth))
                    coef = (1.0-tenpow)/(1.0/twosixth-1.0/twothird)*(ratio - 1.0/twothird)+tenpow;
                else if (ratio>twosixth && ratio<=twothird)
                    coef = (tenpow-1.0)/(twothird-twosixth)*(ratio - twosixth)+1.0;
                else if (ratio>twothird && ratio<=2)
                    coef = tenpow/(twothird-2.0)*(ratio-twothird)+tenpow;
                Q_ASSERT(coef>0);
                L = L*coef;

                sum_i += pow(10.0, (L / 10.0));
            }
        }
        else {
            for (int i = i_min; i <= i_max; ++i) {
                double L = spectrum.at(i);
                sum_i += pow(10.0, (L / 10.0));
            }
        }
        values[k] = 10.0 * log(sum_i) / log(10.0);
    }
    result.first = xValues;
    result.second = values;
    return result;
}

/** Возвращает тип окна, применяемый в uff заголовке 1858
    wind - тип окна, применяемый в DeepSea*/
int uffWindowType(int dfdWindowType)
{
    //12 window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
   //4=exponential, 5=impact, 6=impact and exponential
    switch (dfdWindowType) {
        case 0: return 3;//"Прямоуг.";
        case 1: return 0;//"Бартлетта";
        case 2: return 1;//"Хеннинга";
        case 3: return 0;//"Хемминга";
        case 4: return 0;//"Натолл";
        case 5: return 0;//"Гаусс";
    }
    return 0;
}


int uffMethodFromDfdMethod(int methodId)
{
    switch (methodId) {
        case 9: return 4; //4 - Frequency Response Function
        case 0: return 1; //1 - Time Response
        case 1: return 12; //12 - Spectrum
        case 18: return 12; //12 - Spectrum
    }
    return 0;
    // ниже - нереализованные методы
    //                                       0 - General or Unknown
    //                                       2 - Auto Spectrum
    //                                       3 - Cross Spectrum
    //                                       5 - Transmissibility
    //                                       6 - Coherence
    //                                       7 - Auto Correlation
    //                                       8 - Cross Correlation
    //                                       9 - Power Spectral Density (PSD)
    //                                       10 - Energy Spectral Density (ESD)
    //                                       11 - Probability Density Function
    //                                       13 - Cumulative Frequency Distribution
    //                                       14 - Peaks Valley
    //                                       15 - Stress/Cycles
    //                                       16 - Strain/Cycles
    //                                       17 - Orbit
    //                                       18 - Mode Indicator Function
    //                                       19 - Force Pattern
    //                                       20 - Partial Power
    //                                       21 - Partial Coherence
    //                                       22 - Eigenvalue
    //                                       23 - Eigenvector
    //                                       24 - Shock Response Spectrum
    //                                       25 - Finite Impulse Response Filter
    //                                       26 - Multiple Coherence
    //                                       27 - Order Function
}
