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
 * ?/yNameOld
 * ?/yValuesUnits
 * ?/logref
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
    explicit ChannelFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
    virtual DataDescription getFunctionDescription() const override;
    virtual void updateParameter(const QString &property, const QVariant &val) override;
private:
    friend class RefChannelFunction;

    ChannelSelector selector;
    int channel = 0;
    int triggerChannel = -1;

    // AbstractFunction interface
protected:
    virtual bool m_parameterShowsFor(const QString &parameter) const override;
};

class RefChannelFunction : public ChannelFunction
{
    // AbstractFunction interface
public:
    virtual QString name() const override;
    explicit RefChannelFunction(QObject *parent = nullptr, const QString &name=QString());
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual bool compute(FileDescriptor *file) override;
    virtual QString displayName() const override;
    virtual DataDescription getFunctionDescription() const override;
protected:
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;

};

#endif // CHANNELFUNCTION_H
