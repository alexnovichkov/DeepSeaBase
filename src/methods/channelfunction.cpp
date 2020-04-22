#include "channelfunction.h"

#include "fileformats/filedescriptor.h"

ChannelFunction::ChannelFunction(QObject *parent) :
    AbstractFunction(parent)
{

}


QString ChannelFunction::name() const
{
    return "Channel";
}

QString ChannelFunction::description() const
{
    return "Выбор каналов для обработки";
}

QVariant ChannelFunction::getProperty(const QString &property) const
{
    // we know about ?/channelIndex
    if (property == "?/channelIndex") return channel;
    if (property == "?/channels") return selector.indexesAsString();

    if (!property.startsWith(name()+"/")) return QVariant();
    QString p = property.section("/",1);

    if (p == "filter") return selector.filter();

    return QVariant();
}

void ChannelFunction::setProperty(const QString &property, const QVariant &val)
{
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "filter") selector.setFilter(val.toString());
    else if (p == "channelIndex") channel = val.toInt();
}



QStringList ChannelFunction::properties() const
{
    return QStringList()<<"filter";
}

QString ChannelFunction::propertyDescription(const QString &property) const
{
    if (property == "filter") return "{"
                                     "  \"name\"        : \"filter\"   ,"
                                     "  \"type\"        : \"string\"   ,"
                                     "  \"displayName\" : \"Фильтр\"   ,"
                                     "  \"defaultValue\": \"\"         ,"
                                     "  \"toolTip\"     : \"1, 3-6, 9-\","
                                     "  \"values\"      : []," //для enum
                                     "  \"minimum\"     : -2.4," //для int и double
                                     "  \"maximum\"     : 30.5" //для int и double
                                     "}";
    return "";
}


QString ChannelFunction::displayName() const
{
    return "Каналы";
}

QVector<double> ChannelFunction::getData(const QString &id)
{
    if (id == "input") return output;

    return QVector<double>();
}

bool ChannelFunction::compute(FileDescriptor *file)
{
    if (channel < 0) return false;

    if (selector.includes(channel)) {
        file->channel(channel)->populate();
        output = file->channel(channel)->yValues();
        return true;
    }
    return false;
}
