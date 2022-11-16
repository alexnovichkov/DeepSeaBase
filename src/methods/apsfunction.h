#ifndef APSFUNCTION_H
#define APSFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

/*
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
class ApsFunction : public AbstractFunction
{
public:
    ApsFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString parameterDescription(const QString &parameter) const override;
    virtual QVariant m_getParameter(const QString &parameter) const override;
    virtual void m_setParameter(const QString &parameter, const QVariant &val) override;
    virtual QString displayName() const override;
    virtual DataDescription getFunctionDescription() const override;
private:
    QMap<QString, int> map;

    // AbstractFunction interface
public:
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

#endif // APSFUNCTION_H
