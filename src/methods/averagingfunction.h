#ifndef AVERAGINGFUNCTION_H
#define AVERAGINGFUNCTION_H

#include "abstractfunction.h"

#include "averaging.h"

class AveragingFunction : public AbstractFunction
{
public:
    explicit AveragingFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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
    Averaging averaging;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual QVector<double> get(FileDescriptor *file, const QVector<double> &data) override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute() override;
    virtual void reset() override;
};

#endif // AVERAGINGFUNCTION_H
