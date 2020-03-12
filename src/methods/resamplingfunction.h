#ifndef RESAMPLINGFUNCTION_H
#define RESAMPLINGFUNCTION_H

#include "abstractfunction.h"
#include "resampler.h"

/**
 * @brief The ResamplingFunction class
 * Осуществляет передискретизацию временных реализаций
 */
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
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual void updateProperty(const QString &property, const QVariant &val) override;
private:
    Resampler resampler;
    int exponent = 0;
    double factor = 1.0; //коэффициент new sample rate = sample rate / factor
    int currentResamplingType = 0; //тип передискретизации (0=factor, 1=range, 2=sampleRate)
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
