#ifndef GXYFUNCTION_H
#define GXYFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

class GxyFunction : public AbstractFunction
{
public:
    GxyFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;
    virtual DataDescription getFunctionDescription() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // GXYFUNCTION_H
