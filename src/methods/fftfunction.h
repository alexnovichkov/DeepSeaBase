#ifndef FFTFUNCTION_H
#define FFTFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

/*
 * Spectrum/output - Комплексные / Действительные / Мнимые / Амплитуды / Фазы
 *
 * Отдает:
 * ?/processData
 * ?/dataType
 * ?/xName
 * ?/xType
 * ?/xBegin
 * ?/xStep
 * ?/functionType
 * ?/functionDescription
 * ?/normalization
 * ?/dataFormat
 * ?/yValuesUnits
 * ?/yName
 *
 * Спрашивает:
 * ?/sampleRate
 *
 */

/**
 * @brief The FftFunction class
 * Вычисляет FFT от временного сигнала по формуле:
 * F = FFT(s) * factor, где
 * factor = sqrt(2) / N,
 * N - блина блока данных, в отсчетах
 *
 * Если сохраняются амплитуды, то FFT фактически вычисляет спектр СКЗ ДО УСРЕДНЕНИЯ,
 * то есть F = average(RMS(s))
 * DeepSea вычисляет спектр СКЗ ПОСЛЕ УСРЕДНЕНИЯ, то есть F = RMS(average(PS))
 */
class FftFunction : public AbstractFunction
{
public:
    FftFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &parameter, const QVariant &val) override;
    virtual QString displayName() const override;

private:
    QMap<QString, int> map;
    int portionsCount = 0;
    // AbstractFunction interface
public:
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // FFTFUNCTION_H
