#include "channelfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

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
{DD;
    Channel *ch = 0;
    if (m_file && channel >=0) ch = m_file->channel(channel);

    if (property.startsWith("?/")) {
        if (property == "?/channelIndex") return channel;
        if (property == "?/refChannelIndex") return refChannel;
        if (property == "?/useReferenceChannel") return useRefChannel;

        if (property == "?/channels") return selector.indexesAsString();

        //это - свойства исходного файла
        if (property == "?/sampleRate" && ch) {
            if (qFuzzyIsNull(ch->data()->xStep())) return 0.0;
            return 1.0 / ch->data()->xStep();
        }
        if (property == "?/xBegin") return 0.0;
        if (property == "?/xName") return "с";
        if (property == "?/xType") return 17; //Time
        if (property == "?/xStep" && ch) return ch->data()->xStep();
        if (property == "?/abscissaEven") return true;
        if (property == "?/dataFormat") return "real"; //<- временные данные всегда имеют такой формат
        if (property == "?/zAxisUniform") return true; //всегда равномерная шкала для временных данных
        if (property == "?/yType" && ch) return abscissaType(ch->yName());
        if (property == "?/yName" && ch) return ch->yName();
        if (property == "?/yValuesUnits" && ch) return ch->data()->yValuesUnits();
        if (property == "?/threshold" && ch) return ch->data()->threshold();
        if (property == "?/zName" && ch) return ch->xName();
        if (property == "?/zCount" && ch) return ch->data()->blocksCount();
        if (property == "?/zStep" && ch) return ch->data()->zStep();
        if (property == "?/zBegin" && ch) return ch->data()->zMin();

        if (property == "?/dataType") return 1;//Данные
        if (property == "?/functionType") return 1;//Time response
        if (property == "?/functionDescription") return "Time Response";
    }
    else {
        if (!property.startsWith(name()+"/")) return QVariant();
        QString p = property.section("/",1);

        if (p == "filter") return selector.filter();
        if (p == "refChannelIndex") return refChannel;
        if (p == "useReferenceChannel") return useRefChannel;

//        if (p == "minSec") {
//            double min = ch->data()->xMin();
//            if (ch) return min;
//            return minSec;
//        }
//        if (p == "maxSec") {
//            double max = ch->data()->xMax();
//            if (ch) return max;
//            return maxSec;
//        }
    }
    return QVariant();
}

void ChannelFunction::setProperty(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "filter") selector.setFilter(val.toString());
    else if (p == "channelIndex") channel = val.toInt();
    else if (p == "refChannelIndex") refChannel = val.toInt();
    else if (p == "useReferenceChannel") useRefChannel = val.toBool();
//    else if (p == "minSec") minSec = val.toDouble();
//    else if (p == "maxSec") maxSec = val.toDouble();
}



QStringList ChannelFunction::properties() const
{
    return QStringList()<<"filter"<<"referenceChannel"/*<<"minSec"<<"maxSec"*/;
}

QString ChannelFunction::propertyDescription(const QString &property) const
{
    if (property == "filter") return "{"
                                     "  \"name\"        : \"filter\"   ,"
                                     "  \"type\"        : \"string\"   ,"
                                     "  \"displayName\" : \"Фильтр\"   ,"
                                     "  \"defaultValue\": \"все\"         ,"
                                     "  \"toolTip\"     : \"1, 3-6, 9-\","
                                     "  \"values\"      : []," //для enum
                                     "  \"minimum\"     : -2.4," //для int и double
                                     "  \"maximum\"     : 30.5" //для int и double
                                     "}";
    if (property == "referenceChannel") return "{"
                                               "  \"name\"        : \"referenceChannel\"   ,"
                                               "  \"type\"        : \"int\"   ,"
                                               "  \"displayName\" : \"Опорный канал\"   ,"
                                               "  \"defaultValue\": -1         ,"
                                               "  \"toolTip\"     : \"Номер опорного канала или -1\","
                                               "  \"values\"      : []," //для enum
                                               "  \"minimum\"     : -1," //для int и double
                                               "  \"maximum\"     : 1000000" //для int и double
                                               "}";
//    if (property == "minSec") return "{"
//                                     "  \"name\"        : \"minSec\"   ,"
//                                     "  \"type\"        : \"double\"   ,"
//                                     "  \"displayName\" : \"Начало, сек\"   ,"
//                                     "  \"defaultValue\": 200.0         ,"
//                                     "  \"toolTip\"     : \"\","
//                                     "  \"values\"      : []," //для enum
//                                     "  \"minimum\"     : 0.0," //для int и double
//                                     "  \"maximum\"     : 1000000.0" //для int и double
//                                     "}";
//    if (property == "maxSec") return "{"
//                                     "  \"name\"        : \"maxSec\"   ,"
//                                     "  \"type\"        : \"double\"   ,"
//                                     "  \"displayName\" : \"Конец, сек\"   ,"
//                                     "  \"defaultValue\": \"0.0\"         ,"
//                                     "  \"toolTip\"     : \"\","
//                                     "  \"values\"      : []," //для enum
//                                     "  \"minimum\"     : 0.0," //для int и double
//                                     "  \"maximum\"     : 1000000.0" //для int и double
//                                     "}";
    return "";
}


QString ChannelFunction::displayName() const
{
    return "Каналы";
}

QVector<double> ChannelFunction::getData(const QString &id)
{
    if (id == "input") return output;
    if (id == "referenceInput") return refOutput;

    return QVector<double>();
}

bool ChannelFunction::compute(FileDescriptor *file)
{
    if (channel < 0) return false;

    if (selector.includes(channel)) {
        if (!file->channel(channel)->populated())
            file->channel(channel)->populate();
        //всегда первый блок, так как предполагаем, что временные данные
        //содержат только один блок
        output = file->channel(channel)->data()->yValues(0);

        if (refChannel > -1 && refChannel < file->channelsCount()
            && refOutput.isEmpty() && useRefChannel) {
            if (!file->channel(refChannel)->populated())
                file->channel(refChannel)->populate();
            //всегда первый блок, так как предполагаем, что временные данные
            //содержат только один блок
            refOutput = file->channel(refChannel)->data()->yValues(0);
        }
        return true;
    }

    return false;
}


bool ChannelFunction::propertyShowsFor(const QString &property) const
{
    if (property == (name()+"/referenceChannel")) return useRefChannel;
    return true;
}
