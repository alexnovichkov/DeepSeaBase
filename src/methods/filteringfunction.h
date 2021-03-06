#ifndef FILTERINGFUNCTION_H
#define FILTERINGFUNCTION_H

#include "abstractfunction.h"
#include "filtering.h"
#include <QMap>

/* Filtering/type
 * Filtering/approximation
 * Filtering/order
 * Filtering/frequency
 * Filtering/Q
 * Filtering/bandwidth
 * Filtering/bandwidthHz
 * Filtering/gain
 * Filtering/slope
 * Filtering/rippleDb
 * Filtering/stopDb
 * Filtering/rolloff
 *
 * Спрашивает:
 *
 */
class FilteringFunction : public AbstractFunction
{
public:
    explicit FilteringFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual bool propertyShowsFor(const QString &property) const override;
private:
    Filtering filtering;
    QMap<QString, QVariant> map;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // FILTERINGFUNCTION_H
