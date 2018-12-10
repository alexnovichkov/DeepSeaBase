#include "channelfunction.h"

#include "filedescriptor.h"

ChannelFunction::ChannelFunction(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractFunction(dataBase, parent)
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
    if (property.startsWith("?/")) {
        // we know about ?/fileIndex and ?/channelIndex
        if (property == "?/fileIndex") return file;
        if (property == "?/channelIndex") return channel;
    }

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
    else if (p == "fileIndex") file = val.toInt();
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

// populates filtered channel and returns its yvalues
QVector<double> ChannelFunction::get(FileDescriptor *file, const QVector<double> &data)
{
    int i = int(data.first());

    if (selector.includes(i)) {
        file->channel(i)->populate();
        return file->channel(i)->yValues();
    }
    return QVector<double>();
}





QVector<double> ChannelFunction::getData(const QString &id)
{
    if (id == "input") return output;

    return QVector<double>();
}

bool ChannelFunction::compute()
{
    if (channel < 0) return false;

    if (file < 0 || file >= dataBase().size()) return false;

    if (selector.includes(channel)) {
        FileDescriptor *f = dataBase().at(file);
        f->channel(channel)->populate();
        output = f->channel(channel)->yValues();
        for (int i=0; i<output.size(); ++i) output[i] /= 9.81;
        return true;
    }
    return false;
}
