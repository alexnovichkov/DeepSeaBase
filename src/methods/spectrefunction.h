#ifndef PSDFUNCTION_H
#define PSDFUNCTION_H

#include "abstractfunction.h"
#include <QMap>

class SpectreFunction : public AbstractFunction
{
public:
    SpectreFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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


    AbstractFunction * channelF;
    AbstractFunction * filteringF;
    AbstractFunction * resamplingF;
    AbstractFunction * samplingF;
    AbstractFunction * windowingF;
    AbstractFunction * averagingF;
    AbstractFunction * fftF;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file, const QString &tempFolderName) override;

    // AbstractFunction interface
public:
    virtual QVector<double> getData(const QString &id) override;
};

#endif // PSDFUNCTION_H
