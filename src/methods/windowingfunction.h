#ifndef WINDOWINGFUNCTION_H
#define WINDOWINGFUNCTION_H

#include "abstractfunction.h"
#include "windowing.h"

class WindowingFunction : public AbstractFunction
{

public:
    WindowingFunction(QObject *parent = nullptr);

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
    Windowing windowing;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

#endif // WINDOWINGFUNCTION_H
