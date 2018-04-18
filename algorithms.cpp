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
