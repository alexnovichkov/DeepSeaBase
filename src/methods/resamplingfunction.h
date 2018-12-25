#ifndef RESAMPLINGFUNCTION_H
#define RESAMPLINGFUNCTION_H

#include "abstractfunction.h"
#include "resampler.h"

class ResamplingFunction : public AbstractFunction
{
public:
    explicit ResamplingFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
private:
    Resampler resampler;
    int exponent;
    QVector<double> output;
    double xStep = 0.0;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // RESAMPLINGFUNCTION_H
