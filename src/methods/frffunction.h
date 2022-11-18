#ifndef FRFFUNCTION_H
#define FRFFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

/* FRF/type - FRF1 / FRF2
 * FRF/output - Комплексные / Действительные / Мнимые / Амплитуды / Фазы
 * FRF/referenceChannel - 0/1/2/n-1
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
class FrfFunction : public AbstractFunction
{
public:
    FrfFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;

private:
    QMap<QString, int> map;
    QVector<double> cashedReferenceOutput;

    // AbstractFunction interface
public:
    virtual bool compute(FileDescriptor *file) override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // FRFFUNCTION_H
