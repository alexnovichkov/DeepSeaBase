#ifndef AVERAGINGFUNCTION_H
#define AVERAGINGFUNCTION_H

#include "abstractfunction.h"

#include "averaging.h"

/* Averaging/type - Без усреднения / Линейное / Экспоненциальное /
 *                  Хранение максимума / Энергетическое
 * Averaging/maximum - Число усреднений
 *
 * Отдает:
 * ?/averaging - [string]
 * ?/averagingType
 * ?/zCount
 *
 * Спрашивает:
 *
 */
class AveragingFunction : public AbstractFunction
{
public:
    explicit AveragingFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString parameterDescription(const QString &parameter) const override;
    virtual QVariant m_getParameter(const QString &parameter) const override;
    virtual void m_setParameter(const QString &parameter, const QVariant &val) override;
protected:
    virtual bool m_parameterShowsFor(const QString &parameter) const override;

private:
    Averaging averaging;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // AVERAGINGFUNCTION_H
