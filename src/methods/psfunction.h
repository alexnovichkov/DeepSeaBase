#ifndef PSFUNCTION_H
#define PSFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

/*
 *
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
class PsFunction : public AbstractFunction
{
public:
    PsFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;

    // AbstractFunction interface
public:
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // PSFUNCTION_H
