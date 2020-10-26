#ifndef CHANNELFUNCTION_H
#define CHANNELFUNCTION_H

#include "abstractfunction.h"
#include "channelselector.h"

/* Channel/filter - [string] строка фильтра каналов
 * Channel/channelIndex
 * Channel/referenceChannel - номер опорного канала
 * Channel/useReferenceChannel - отображение выбора опорного канала
 *
 * Отдает:
 * ?/channelIndex - номер канала исходного файла
 * ?/refChannelIndex - номер опорного канала исходного файла
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
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QString displayName() const override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
private:
    ChannelSelector selector;
    int channel = 0;
    int refChannel = -1; // if < 0 -> no reference channel used
    bool useRefChannel = false;
    double minSec = 0;
    double maxSec = 10;
    QVector<double> output;
    QVector<double> refOutput;

    // AbstractFunction interface
public:
    virtual bool propertyShowsFor(const QString &property) const override;
};

#endif // CHANNELFUNCTION_H
