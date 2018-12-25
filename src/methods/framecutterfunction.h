#ifndef SAMPLINGFUNCTION_H
#define SAMPLINGFUNCTION_H

#include "methods/abstractfunction.h"

#include "framecutter.h"

class FrameCutterFunction : public AbstractFunction
{
public:
    explicit FrameCutterFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual bool propertyShowsFor(const QString &property) const override;

    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
private:
    FrameCutter frameCutter;
    QMap<QString, QVariant> parameters;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;

    // AbstractFunction interface
public slots:
    virtual void updateProperty(const QString &property, const QVariant &val) override;
};

#endif // SAMPLINGFUNCTION_H
