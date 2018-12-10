#ifndef FFTFUNCTION_H
#define FFTFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>

class FftFunction : public AbstractFunction
{
public:
    FftFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;
    virtual QVector<double> get(FileDescriptor *file, const QVector<double> &data) override;

private:
    QMap<QString, int> map;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute() override;
    virtual void reset() override;
};

#endif // FFTFUNCTION_H
