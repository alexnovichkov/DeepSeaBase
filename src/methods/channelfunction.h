#ifndef CHANNELFUNCTION_H
#define CHANNELFUNCTION_H

#include "abstractfunction.h"
#include "channelselector.h"

/* Channel/filter - [string] строка фильтра каналов
 * Channel/channelIndex
 * Channel/referenceChannelIndex - отображение выбора опорного канала
 *
 * Отдает:
 * ?/channelIndex - номер канала исходного файла
 * ?/referenceChannelIndex - номер опорного канала исходного файла
 * ?/channels - [string] список каналов
 * ?/xBegin
 * ?/xName
 * ?/xType
 * ?/xStep
 * ?/abscissaEven
 * ?/dataFormat
 * ?/yType
 * ?/yName
 * ?/yValuesUnits
 * ?/threshold
 * ?/zName
 * ?/zCount
 * ?/zStep
 * ?/zBegin
 * ?/zAxisUniform
 *
 * Спрашивает:
 */
class ChannelFunction : public AbstractFunction
{
public:
    explicit ChannelFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QString displayName() const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
private:
    friend class RefChannelFunction;

    ChannelSelector selector;
    int channel = 0;
    QVector<double> output;

};

class RefChannelFunction : public ChannelFunction
{


    // AbstractFunction interface
public:
    virtual QString name() const override;
    explicit RefChannelFunction(QObject *parent = nullptr);
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual bool compute(FileDescriptor *file) override;
    virtual QString displayName() const override;

protected:
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;

};

#endif // CHANNELFUNCTION_H
