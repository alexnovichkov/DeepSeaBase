#ifndef SAMPLINGFUNCTION_H
#define SAMPLINGFUNCTION_H

#include "methods/abstractfunction.h"

#include "framecutter.h"

class FrameCutterFunction : public AbstractFunction
{
public:
    explicit FrameCutterFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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
    virtual QVector<double> get(FileDescriptor *file, const QVector<double> &data) override;
    virtual void reset() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute() override;
};

#endif // SAMPLINGFUNCTION_H
