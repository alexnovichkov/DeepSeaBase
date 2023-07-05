#include "channelfunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

ChannelFunction::ChannelFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}


QString ChannelFunction::name() const
{DD;
    return "Channel";
}

QString ChannelFunction::description() const
{DD;
    return "Выбор каналов для обработки";
}

QVariant ChannelFunction::m_getParameter(const QString &property) const
{DD;
    Channel *ch = 0;
    if (m_file && channel >=0) ch = m_file->channel(channel);

    if (property.startsWith("?/")) {
        if (property == "?/channelIndex") return channel;
        if (property == "?//referenceChannelIndex") return -1;

        if (property == "?/channels") return selector.indexes();

        //это - свойства исходного файла
        if (property == "?/sampleRate" && ch) {
            if (qFuzzyIsNull(ch->data()->xStep())) return 0.0;
            //LOG(DEBUG)<<ch->data()->xStep()<<(1.0 / ch->data()->xStep());
            return 1.0 / ch->data()->xStep();
        }
        if (property == "?/xBegin" && ch) return ch->data()->xMin();
        if (property == "?/xName") return "с";
//        if (property == "?/xType") return 17; //Time
        if (property == "?/xStep" && ch) return ch->data()->xStep();
        if (property == "?/abscissaEven") return true;
        if (property == "?/dataFormat") return "real"; //<- временные данные всегда имеют такой формат
        if (property == "?/zAxisUniform") return true; //всегда равномерная шкала для временных данных
//        if (property == "?/yType" && ch) return abscissaType(ch->yName());
        if (property == "?/yName" && ch) return ch->yName();
        if (property == "?/yNameOld" && ch) return ch->yName();
        if (property == "?/yValuesUnits" && ch) return ch->data()->yValuesUnits();
        if (property == "?/logref" && ch) return ch->data()->threshold();
        if (property == "?/zName" && ch) return ch->zName();
        if (property == "?/zCount" && ch) return ch->data()->blocksCount();
        if (property == "?/zStep" && ch) return ch->data()->zStep();
        if (property == "?/zBegin" && ch) return ch->data()->zMin();
        if (property == "?/portionsCount") return 1; //единственная порция данных
        if (property == "?/blockSize" && ch) return ch->data()->samplesCount(); //единственная порция данных

        if (property == "?/dataType") return 1;//Данные
//        if (property == "?/functionType") return 1;//Time response
        if (property == "?/functionDescription") return "Time Response";
    }
    else {
        if (!property.startsWith(name()+"/")) return QVariant();
        QString p = property.section("/",1);

        if (p == "filter") return selector.filter();
        else if (p == "selection") return static_cast<int>(selector.selection());

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

void ChannelFunction::m_setParameter(const QString &property, const QVariant &val)
{DD;
    if (!property.startsWith(name()+"/")) return;
    QString p = property.section("/",1);

    if (p == "filter") selector.setFilter(val.toString());
    else if (p == "selection") {
        switch (val.toInt()) {
            case 0: selector.setSelection(ChannelSelector::Selection::All); break;
            case 1: selector.setSelection(ChannelSelector::Selection::Some); break;
            default: break;
        }
    }

    else if (p == "channelIndex") channel = val.toInt();

//    else if (p == "minSec") minSec = val.toDouble();
//    else if (p == "maxSec") maxSec = val.toDouble();
}



QStringList ChannelFunction::parameters() const
{DD;
    return {"selection", "filter" /*, "minSec", "maxSec"*/};
}

QString ChannelFunction::m_parameterDescription(const QString &property) const
{DD;
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
    if (property == "selection") return "{"
                                     "  \"name\"        : \"selection\"   ,"
                                     "  \"type\"        : \"enum\"   ,"
                                     "  \"displayName\" : \"Каналы\"   ,"
                                     "  \"defaultValue\": 0         ,"
                                     "  \"toolTip\"     : \"\","
                                     "  \"values\"      : [\"Все\", \"Выборка\"]," //для enum
                                     "  \"minimum\"     : 0," //для int и double
                                     "  \"maximum\"     : 0" //для int и double
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
{DD;
    return "Каналы";
}

bool ChannelFunction::compute(FileDescriptor *file)
{DD;
    if (channel < 0 || file->channelsCount() <= channel) return false;
    LOG(INFO) << QString("Запуск отбора каналов");

    if (selector.includes(channel)) {
        if (!file->channel(channel)->populated())
            file->channel(channel)->populate();
        //всегда первый блок, так как предполагаем, что временные данные
        //содержат только один блок
        output = file->channel(channel)->data()->yValues(0);
    }

    if (triggerData.isEmpty()) {
        if (triggerChannel >= 0 && triggerChannel < file->channelsCount()) {
            LOG(DEBUG) << QString("Канал триггера")<<triggerChannel+1;
            LOG(DEBUG) << QString("Чтение канала триггера в ChannelFunction");
            bool populated = file->channel(triggerChannel)->populated();
            if (!populated) file->channel(triggerChannel)->populate();
            triggerData = file->channel(triggerChannel)->data()->yValues(0);
            if (!populated) file->channel(triggerChannel)->clear();
            LOG(DEBUG)<<QString("Канал триггера %1 имеет %2 отсчетов").arg(triggerChannel+1).arg(triggerData.size());
        }
    }
    else
        LOG(DEBUG)<<QString("Канал триггера %1 имеет %2 отсчетов").arg(triggerChannel+1).arg(triggerData.size());

    return !output.isEmpty();
}

void ChannelFunction::updateParameter(const QString &property, const QVariant &val)
{DD;
    if (property == "?/triggerChannel") triggerChannel = val.toInt();
}

QString RefChannelFunction::name() const
{DD;
    return "RefChannel";
}

RefChannelFunction::RefChannelFunction(QObject *parent, const QString &name)
    : ChannelFunction(parent, name)
{DD;

}

QStringList RefChannelFunction::parameters() const
{DD;
    return {"referenceChannelIndex"};
}

QString RefChannelFunction::m_parameterDescription(const QString &property) const
{DD;
    if (property == "referenceChannelIndex") return "{"
                                               "  \"name\"        : \"referenceChannelIndex\"   ,"
                                               "  \"type\"        : \"int\"   ,"
                                               "  \"displayName\" : \"Опорный канал\"   ,"
                                               "  \"defaultValue\": -1         ,"
                                               "  \"toolTip\"     : \"Номер опорного канала или -1\","
                                               "  \"values\"      : []," //для enum
                                               "  \"minimum\"     : -1," //для int и double
                                               "  \"maximum\"     : 1000000" //для int и double
                                               "}";
    return QString();
}

bool RefChannelFunction::compute(FileDescriptor *file)
{DD;
    if (channel < 0 || file->channelsCount() <= channel) return false;

    if (!file->channel(channel)->populated())
        file->channel(channel)->populate();
    //всегда первый блок, так как предполагаем, что временные данные
    //содержат только один блок
    output = file->channel(channel)->data()->yValues(0);

    if (triggerChannel >= 0 && triggerChannel < file->channelsCount() && triggerData.isEmpty()) {
        //LOG(DEBUG) << "computing trigger channel in RefChannelFunction";
        if (!file->channel(triggerChannel)->populated())
            file->channel(triggerChannel)->populate();
        triggerData = file->channel(triggerChannel)->data()->yValues(0);
        //LOG(DEBUG)<<"trigger data at channel"<<triggerChannel+1<<"has"<<triggerData.size()<<"samples";
    }

    return !output.isEmpty();
}

QString RefChannelFunction::displayName() const
{DD;
    return "Опорный канал";
}

QVariant RefChannelFunction::m_getParameter(const QString &property) const
{DD;
    if (property == name()+"/referenceChannelIndex") return channel+1;
    else if (property == "?/referenceChannelIndex") return channel+1;

    Channel *ch = 0;
    if (m_file && channel >=0) ch = m_file->channel(channel);
    if (property == "?/yName" && ch) return ch->yName();
    if (property == "?/yNameOld" && ch) return ch->yName();

    return ChannelFunction::m_getParameter(property);
}

void RefChannelFunction::m_setParameter(const QString &property, const QVariant &val)
{DD;
    if (property == name()+"/referenceChannelIndex") channel = val.toInt()-1;
    else ChannelFunction::m_setParameter(property, val);
}


DataDescription ChannelFunction::getFunctionDescription() const
{DD;
    DataDescription result = AbstractFunction::getFunctionDescription();

    result.put("function.name", "Time Response");
    result.put("function.type", 1);
    result.put("function.format", "real");
    result.put("function.logscale", "linear");

    return result;
}


bool ChannelFunction::m_parameterShowsFor(const QString &parameter) const
{
    if (parameter == "filter") return selector.selection() == ChannelSelector::Selection::Some;
    return true;
}
