#ifndef RESAMPLINGFUNCTION_H
#define RESAMPLINGFUNCTION_H

#include "abstractfunction.h"
#include "resampler.h"

/**
 * @brief The ResamplingFunction class
 * Осуществляет передискретизацию временных реализаций
 */

/* Resampling/resampleType - Коэффициент / Частотный диапазон / Частота дискретизации
 * Resampling/factor - Коэффициент
 * Resampling/frequencyRange - Частотный диапазон
 * Resampling/sampleRate - Частота дискретизации
 *
 * Отдает:
 * ?/sampleRate
 * ?/functionDescription = RSMPL
 * ?/functionType = 1 (Time response)
 * ?/dataType = 1 (Cutted data)
 * ?/processData - для dfd
 * ?/xStep - пересчитывает
 *
 * Спрашивает:
 *
 */

class ResamplingFunction : public AbstractFunction
{
public:
    explicit ResamplingFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual void updateProperty(const QString &property, const QVariant &val) override;
private:
    Resampler resampler;
    int exponent = 0;
    double factor = 1.0; //коэффициент new sample rate = sample rate / factor
    int currentResamplingType = 0; //тип передискретизации (0=factor, 1=range, 2=sampleRate)
    double xStep = 0.0;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual bool compute(FileDescriptor *file) override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // RESAMPLINGFUNCTION_H
