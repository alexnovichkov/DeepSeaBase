#ifndef GXYFUNCTION_H
#define GXYFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

class GxyFunction : public AbstractFunction
{
public:
    GxyFunction(QObject *parent = nullptr);

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
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

#endif // GXYFUNCTION_H
