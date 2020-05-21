#include "logscaleengine.h"
#include "qwt_math.h"
#include "qwt_interval.h"
#include "QtMath"

//#define LOG_MIN_MY 1.0e-3
#define LOG_MIN_MY 1.0

#ifndef LOG_MIN
//! Minimum value for logarithmic scales
#define LOG_MIN 1.0e-100
#endif

#ifndef LOG_MAX
//! Maximum value for logarithmic scales
#define LOG_MAX 1.0e100
#endif

static inline QwtInterval logInterval(double base, const QwtInterval &interval)
{
    return QwtInterval(log(interval.minValue()) / log(base),
            log(interval.maxValue()) / log(base));
}

void LogScaleEngine::autoScale(int maxNumSteps, double &x1, double &x2, double &stepSize) const
{
    if (x1 > x2) qSwap(x1, x2);

    const double logBase = base();

    QwtInterval interval(x1 / qPow(logBase, lowerMargin()),
                         x2 * qPow(logBase, upperMargin()));

    if (interval.maxValue() / interval.minValue() < logBase) {
        // scale width is less than one step -> try to build a linear scale
        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(lowerMargin(), upperMargin());
        linearScaler.autoScale(maxNumSteps, x1, x2, stepSize);

        QwtInterval linearInterval = QwtInterval(x1, x2).normalized();
        linearInterval = linearInterval.limited(LOG_MIN, LOG_MAX);

        if (linearInterval.maxValue() / linearInterval.minValue() < logBase) {
            // the aligned scale is still less than one step
            if (stepSize < 0.0)
                stepSize = - log(qAbs(stepSize)) / log(logBase);
            else
                stepSize = log(stepSize) / log(logBase);
            return;
        }
    }

    double logRef = 1.0;
    if (reference() > LOG_MIN_MY / 2.0)
        logRef = qMin(reference(), LOG_MAX / 2);

    if (testAttribute(QwtScaleEngine::Symmetric)) {
        const double delta = qMax(interval.maxValue() / logRef,
                                  logRef / interval.minValue());
        interval.setInterval(logRef / delta, logRef * delta);
    }

    if (testAttribute(QwtScaleEngine::IncludeReference))
        interval = interval.extend(logRef);

    interval = interval.limited(LOG_MIN_MY, LOG_MAX);

    if (qFuzzyIsNull(interval.width()))
        interval = buildInterval(interval.minValue());

    stepSize = divideInterval(logInterval(logBase, interval).width(),
                              qMax(maxNumSteps, 1));
    if (stepSize < 1.0)
        stepSize = 1.0;

    if (!testAttribute(QwtScaleEngine::Floating))
        interval = align(interval, stepSize);

    x1 = interval.minValue();
    x2 = interval.maxValue();

    if (testAttribute(QwtScaleEngine::Inverted)) {
        qSwap(x1, x2);
        stepSize = -stepSize;
    }
}

QwtScaleDiv LogScaleEngine::divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize) const
{
    QwtInterval interval = QwtInterval(x1, x2).normalized();
    interval = interval.limited(LOG_MIN_MY, LOG_MAX);

    if (interval.width() <= 0)
        return QwtScaleDiv();

    const double logBase = base();

    if (interval.maxValue() / interval.minValue() < logBase) {
        // scale width is less than one decade -> build linear scale

        QwtLinearScaleEngine linearScaler;
        linearScaler.setAttributes(attributes());
        linearScaler.setReference(reference());
        linearScaler.setMargins(lowerMargin(), upperMargin());

        if (stepSize != 0.0) {
            if (stepSize < 0.0)
                stepSize = -qPow(logBase, -stepSize);
            else
                stepSize = qPow(logBase, stepSize);
        }

        return linearScaler.divideScale(x1, x2,
                                        maxMajorSteps, maxMinorSteps, stepSize);
    }

    stepSize = qAbs(stepSize);
    if (qFuzzyIsNull(stepSize)) {
        if (maxMajorSteps < 1)
            maxMajorSteps = 1;

        stepSize = divideInterval(
                       logInterval(logBase, interval).width(), maxMajorSteps);
        if (stepSize < 1.0)
            stepSize = 1.0; // major step must be >= 1 decade
    }

    QwtScaleDiv scaleDiv;
    if (stepSize != 0.0) {
        QList<double> ticks[QwtScaleDiv::NTickTypes];
        buildTicks(interval, stepSize, maxMinorSteps, ticks);

        scaleDiv = QwtScaleDiv(interval, ticks);
    }

    if (x1 > x2)
        scaleDiv.invert();

    return scaleDiv;
}
