#ifndef RESAMPLINGFUNCTION_H
#define RESAMPLINGFUNCTION_H

#include "abstractfunction.h"
#include "resampler.h"

class ResamplingFunction : public AbstractFunction
{
public:
    explicit ResamplingFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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
    int currentFactor;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual QVector<double> get(FileDescriptor *file, const QVector<double>  &data);
    virtual void reset() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute() override;
};

#endif // RESAMPLINGFUNCTION_H
