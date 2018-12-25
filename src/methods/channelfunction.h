#ifndef CHANNELFUNCTION_H
#define CHANNELFUNCTION_H

#include "abstractfunction.h"
#include "channelselector.h"

class ChannelFunction : public AbstractFunction
{
public:
    explicit ChannelFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QString displayName() const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
private:
    ChannelSelector selector;
    int channel = -1;
    QVector<double> output;   
};

#endif // CHANNELFUNCTION_H
