#ifndef CHANNELFUNCTION_H
#define CHANNELFUNCTION_H

#include "abstractfunction.h"
#include "channelselector.h"

class ChannelFunction : public AbstractFunction
{
public:
    explicit ChannelFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
private:
    ChannelSelector selector;
    int channel = -1;
    int file = -1;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual QVector<double> get(FileDescriptor *file, const QVector<double>  &data) override;

    // AbstractFunction interface
public:
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute() override;
};

#endif // CHANNELFUNCTION_H
