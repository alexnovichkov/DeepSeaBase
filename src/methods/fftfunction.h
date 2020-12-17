#ifndef FFTFUNCTION_H
#define FFTFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

/* Spectrum/type - FFT / Power spectrum / Power spectrum density
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
class FftFunction : public AbstractFunction
{
public:
    FftFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;

private:
    QMap<QString, int> map;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

#endif // FFTFUNCTION_H
