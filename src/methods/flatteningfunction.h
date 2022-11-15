#ifndef FLATTENINGFUNCTION_H
#define FLATTENINGFUNCTION_H

#include "methods/abstractfunction.h"

/* Flattener
 *
 * Отдает:
 * ?/zCount
 *
 * Спрашивает:
 *
 */

class FlatteningFunction : public AbstractFunction
{
public:
    FlatteningFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString displayName() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual bool compute(FileDescriptor *file) override;

protected:
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
};

#endif // FLATTENINGFUNCTION_H