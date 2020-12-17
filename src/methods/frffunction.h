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
    FrfFunction(QObject *parent = nullptr);

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
    QVector<double> cashedReferenceOutput;

    // AbstractFunction interface
public:
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

#endif // FRFFUNCTION_H
