#include "tdmsfile.h"


#include <QDateTime>
#include "../../logging.h"
#include "../../algorithms.h"
#include "../../dataholder.h"

int NITypeToUffType(const QVariantMap &properties)
{
    const QString NI_function_type = properties.value("NI_function_type").toString();
    const QString wf_xunit_string = properties.value("wf_xunit_string").toString();

    if (NI_function_type.isEmpty()) {
        //смотрим по суффиксу у wf_xname
        if (wf_xunit_string.compare("s", Qt::CaseInsensitive)==0) return Descriptor::TimeResponse;
        return Descriptor::Unknown;
    }

    if (NI_function_type.endsWith("signal", Qt::CaseInsensitive)) return Descriptor::TimeResponse;
    return Descriptor::Unknown;
}

QString NITypeToUffName(const QVariantMap &properties)
{
    const QString NI_function_type = properties.value("NI_function_type").toString();
    const QString wf_xname = properties.value("wf_xname").toString();

    if (NI_function_type.isEmpty()) {
        //смотрим по суффиксу у wf_xname
        return wf_xname.section('_',-1,-1);
    }
    return NI_function_type.section(' ',-1,-1);
}
QString NIDataTypeToPrecision(DDCDataType dataType)
{
    switch (dataType) {
        case DDC_UInt8: return "uint8";
        case DDC_Int16: return "int16";
        case DDC_Int32: return "int32";
        case DDC_Float: return "float";
        case DDC_Double: return "double";
        case DDC_String: return "string";
        case DDC_Timestamp: return "time";
    }
    return "";
}


TDMSFile::TDMSFile(const QString &fileName) : fileName(fileName)
{DD;
    int error;

    QString fn = fileName;
    fn.replace("/","\\");

    error = DDC_OpenFileEx(fn.toLocal8Bit().data(), 0, 1, &file);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }

    // Reading file properties
    uint numberOfProperties = 0;

    error = DDC_GetNumFileProperties(file, &numberOfProperties);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
    }
    else {
        for (size_t i=0; i<numberOfProperties; ++i) {
            //property name length
            size_t length = 0;
            error = DDC_GetFilePropertyNameLengthFromIndex(file, i, &length);
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                continue;
            }
            //property name
            char *nameBuf;
            nameBuf = (char*)malloc(length+1);
            error = DDC_GetFilePropertyNameFromIndex(file, i, nameBuf, length+1);
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                free(nameBuf);
                continue;
            }
            QString name = QString::fromLocal8Bit(nameBuf);
            //property data
            QVariant value;

            //property type
            DDCDataType dataType;
            error = DDC_GetFilePropertyType(file, nameBuf, &dataType);
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            }
            else {
                switch (dataType) {
                    case DDC_UInt8: {
                        quint8 val;
                        error = DDC_GetFileProperty(file, nameBuf, &val, 1);
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = val;
                        break;
                    }
                    case DDC_Int16: {
                        qint16 val;
                        error = DDC_GetFileProperty(file, nameBuf, &val, 2);
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = val;
                        break;
                    }
                    case DDC_Int32: {
                        qint32 val;
                        error = DDC_GetFileProperty(file, nameBuf, &val, 4);
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = val;
                        break;
                    }
                    case DDC_Float: {
                        float val;
                        error = DDC_GetFileProperty(file, nameBuf, &val, sizeof(float));
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = val;
                        break;
                    }
                    case DDC_Double: {
                        double val;
                        error = DDC_GetFileProperty(file, nameBuf, &val, sizeof(double));
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = val;
                        break;
                    }
                    case DDC_String: {
                        char *buffer=0;
                        uint propLength;
                        error = DDC_GetFileStringPropertyLength (file, nameBuf, &propLength);
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else {
                            if (propLength > 0) {
                                buffer = (char*)malloc(propLength+1);
                                error = DDC_GetFileProperty(file, nameBuf, buffer, propLength+1);
                                if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                                else value = QString::fromLocal8Bit(buffer);
                                free (buffer);
                            }
                        }
                        break;
                    }
                    case DDC_Timestamp: {
                        uint year, month, day, hour, minute, second, weekDay;
                        double millisecond;
                        error = DDC_GetFilePropertyTimestampComponents(file, nameBuf, &year, &month, &day,
                                                                       &hour, &minute, &second, &millisecond, &weekDay);
                        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                        else value = QDateTime(QDate(year,month,day),QTime(hour, minute, second,int(millisecond)));
                        break;
                    }
                    default: break;
                }
            }
            properties.insert(name, value);

            free(nameBuf);
        }
    }
    //LOG(DEBUG)<<properties;

    // Reading Channel groups
    uint numberOfChannelGroups=0;
    error = DDC_GetNumChannelGroups(file, &numberOfChannelGroups);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }

    _groups = (DDCChannelGroupHandle *)calloc(numberOfChannelGroups, sizeof (DDCChannelGroupHandle));
    error = DDC_GetChannelGroups(file, _groups, numberOfChannelGroups);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }

    for (size_t i=0; i<numberOfChannelGroups; ++i) {
        //LOG(DEBUG)<<"Reading channel group"<<i+1<<"of"<<numberOfChannelGroups;
        TDMSGroup *group = new TDMSGroup(_groups[i], fileName);
        group->parent = this;
        groups << group;
    }
    _isValid = true;
}

TDMSFile::~TDMSFile()
{DD;
    free(_groups);
    qDeleteAll(groups);
    if (file) DDC_CloseFile(file);
}

TDMSGroup::TDMSGroup(DDCChannelGroupHandle group, const QString &name) : FileDescriptor(name),
    group(group)
{DD;
    // reading channel group properties
    uint numberOfProperties = 0;
    int error = DDC_GetNumChannelGroupProperties(group, &numberOfProperties);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
    }
    else {
        //LOG(DEBUG)<<"Channel group"<<"contains"<<numberOfProperties<<"properties";

        for (size_t j=0; j<numberOfProperties; ++j) {
            //property name length
            size_t length;
            error = DDC_GetChannelGroupPropertyNameLengthFromIndex(group, j, &length);
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                continue;
            }
            //property name
            char *nameBuf;
            nameBuf = (char*)malloc(length+1);
            error = DDC_GetChannelGroupPropertyNameFromIndex(group, j, nameBuf, length+1);
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
                continue;
            }
            QString name = QString::fromLocal8Bit(nameBuf);
            //property data
            QVariant value;
            //property type
            DDCDataType dataType;
            error = DDC_GetChannelGroupPropertyType(group, nameBuf, &dataType);
            if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            else {
                switch (dataType) {
                    case DDC_UInt8: {
                        quint8 val;
                        error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 1);
                        value = val;
                        break;
                    }
                    case DDC_Int16: {
                        qint16 val;
                        error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 2);
                        value = val;
                        break;
                    }
                    case DDC_Int32: {
                        qint32 val;
                        error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 4);
                        value = val;
                        break;
                    }
                    case DDC_Float: {
                        float val;
                        error = DDC_GetChannelGroupProperty(group, nameBuf, &val, sizeof(float));
                        value = val;
                        break;
                    }
                    case DDC_Double: {
                        double val;
                        error = DDC_GetChannelGroupProperty(group, nameBuf, &val, sizeof(double));
                        value = val;
                        break;
                    }
                    case DDC_String: {
                        char *buffer;
                        uint propLength;
                        error = DDC_GetChannelGroupStringPropertyLength(group, nameBuf, &propLength);
                        if (propLength > 0) {
                            buffer = (char*)malloc(propLength+1);
                            error = DDC_GetChannelGroupProperty(group, nameBuf, buffer, propLength+1);
                            value = QString::fromLocal8Bit(buffer);
                            free (buffer);
                        }
                        break;
                    }
                    case DDC_Timestamp: {
                        uint year, month, day, hour, minute, second, weekDay;
                        double millisecond;
                        error = DDC_GetChannelGroupPropertyTimestampComponents(group, nameBuf, &year, &month, &day,
                                                                               &hour, &minute, &second, &millisecond, &weekDay);
                        value = QDateTime(QDate(year,month,day),QTime(hour, minute, second,int(millisecond)));
                        break;
                    }
                    default: break;
                }
                if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            }
            properties.insert(name, value);
            free(nameBuf);
        }
    }
//    LOG(DEBUG)<<properties;

    // number of channels
    uint numberOfChannels = 0;
    error = DDC_GetNumChannels(group, &numberOfChannels);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }
    //LOG(DEBUG)<<"Channel group"<<"contains"<<numberOfChannels<<"channels";

    // reading channels
    _channels = (DDCChannelHandle *)calloc(numberOfChannels, sizeof(DDCChannelHandle));
    error = DDC_GetChannels(group, _channels, numberOfChannels);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }

    for (uint i=0; i<numberOfChannels; ++i) {
        TDMSChannel *channel = new TDMSChannel(_channels[i], this);
        channels << channel;
    }
}

TDMSGroup::~TDMSGroup()
{DD;
    free(_channels);
    qDeleteAll(channels);
    DDC_CloseChannelGroup(group);
}

TDMSChannel::TDMSChannel(DDCChannelHandle channel, TDMSGroup *parent) : channel(channel), parent(parent)
{DD;
    // reading channel properties
    uint numberOfProperties = 0;
    int error = DDC_GetNumChannelProperties(channel, &numberOfProperties);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }

    for (size_t k=0; k<numberOfProperties; ++k) {
        //property name length
        size_t length;
        error = DDC_GetChannelPropertyNameLengthFromIndex(channel, k, &length);
        if (error < 0) {
            LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        //property name
        char *nameBuf;
        nameBuf = (char*)malloc(length+1);
        error = DDC_GetChannelPropertyNameFromIndex(channel, k, nameBuf, length+1);
        if (error < 0) {
            LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        QString name = QString::fromLocal8Bit(nameBuf);
//        QString name = QString::fromUtf8(nameBuf);
        //property data
        QVariant value;
        //property type
        DDCDataType dataType;
        error = DDC_GetChannelPropertyType(channel, nameBuf, &dataType);
        if (error < 0) LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        else {
            switch (dataType) {
                case DDC_UInt8: {
                    quint8 val;
                    error = DDC_GetChannelProperty(channel, nameBuf, &val, 1);
                    value =val;
                    break;
                }
                case DDC_Int16: {
                    qint16 val;
                    error = DDC_GetChannelProperty(channel, nameBuf, &val, 2);
                    value =val;
                    break;
                }
                case DDC_Int32: {
                    qint32 val;
                    error = DDC_GetChannelProperty(channel, nameBuf, &val, 4);
                    value =val;
                    break;
                }
                case DDC_Float: {
                    float val;
                    error = DDC_GetChannelProperty(channel, nameBuf, &val, sizeof(float));
                    value =val;
                    break;
                }
                case DDC_Double: {
                    double val;
                    error = DDC_GetChannelProperty(channel, nameBuf, &val, sizeof(double));
                    value =val;
                    break;
                }
                case DDC_String: {
                    char *buffer;
                    uint propLength;
                    error = DDC_GetChannelStringPropertyLength(channel, nameBuf, &propLength);
                    if (propLength > 0) {
                        buffer = (char*)malloc(propLength+1);
                        error = DDC_GetChannelProperty(channel, nameBuf, buffer, propLength+1);
                        value = QString::fromLocal8Bit(buffer);
                        free (buffer); // Free the buffer when you are finished with it.
                    }
                    break;
                }
                case DDC_Timestamp: {
                    uint year, month, day, hour, minute, second, weekDay;
                    double millisecond;
                    error = DDC_GetChannelPropertyTimestampComponents(channel, nameBuf, &year, &month, &day,
                                                                      &hour, &minute, &second, &millisecond, &weekDay);
                    value = QDateTime(QDate(year,month,day),QTime(hour, minute, second,int(millisecond)));
                    break;
                }
                default: break;
            }
            if (error < 0) {
                LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
            }
        }
        properties.insert(name, value);
        free(nameBuf);
    }
//    LOG(DEBUG) << "Channel properties" << properties;

    dataDescription().put("name", properties.value("name"));
    dataDescription().put("description", properties.value("description"));
    dataDescription().put("xname", properties.value("wf_xunit_string"));
    dataDescription().put("yname", properties.value("unit_string"));
    auto dt = properties.value("DateTime");
    if (dt.isNull()) dt = properties.value("wf_start_time");
    dataDescription().put("dateTime", dt);

    auto NI_function_type = properties.value("NI_function_type").toString();
    auto wf_xname = properties.value("wf_xname").toString();
    dataDescription().put("function.type", NITypeToUffType(properties));
    dataDescription().put("function.name", NITypeToUffName(properties));
    dataDescription().put("function.logref", properties.value("NI_dbReference",1.0).toDouble());
    dataDescription().put("function.logscale", "linear"); //TODO: добавить определение
    dataDescription().put("function.format", "real"); //TODO: добавить определение

    // reading data
    error = DDC_GetNumDataValues(channel, &numberOfValues);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }
    error = DDC_GetDataType(channel, &dataType);
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
        return;
    }
    //LOG(DEBUG)<<"Channel"<<"contains"<<numberOfValues<<"values of type"<<dataType;
    dataDescription().put("function.precision", NIDataTypeToPrecision(dataType));
    dataDescription().put("samples", numberOfValues);
    dataDescription().put("blocks", 1); //1 block by default

    //LOG(DEBUG)<<"TDMS channel description:"<<dataDescription().data;

    double xBegin = properties.value("wf_start_offset").toDouble();
    double xStep = properties.value("wf_increment").toDouble();
    //workaround for empty channels
    if (qFuzzyIsNull(xStep)) xStep=1.0;
    data()->setXValues(xBegin, xStep, numberOfValues);

    data()->setZValues(0.0, 1.0, 1);

    data()->setThreshold(properties.value("NI_dbReference",1.0).toDouble());
    data()->setYValuesFormat(DataHolder::YValuesReals);
    data()->setYValuesUnits(DataHolder::UnitsLinear);
}

TDMSChannel::~TDMSChannel()
{DD;
    DDC_CloseChannel(channel);
}

QVector<double> TDMSChannel::getDouble()
{DD;
    QVector<double> data(numberOfValues);
    int error = 0;
    switch (dataType) {
        case DDC_UInt8: {// unsigned char
            unsigned char *dataUint8 = new uchar[numberOfValues];
            error = DDC_GetDataValuesUInt8(channel, 0, numberOfValues, dataUint8);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = double(dataUint8[i]);
            delete [] dataUint8;
            break;
        }
        case DDC_Int16: {
            short *dataInt16 = new short[numberOfValues];
            error = DDC_GetDataValuesInt16(channel, 0, numberOfValues, dataInt16);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = double(dataInt16[i]);
            delete [] dataInt16;
            break;
        }
        case DDC_Int32: {
            long *dataInt32 = new long[numberOfValues];
            error = DDC_GetDataValuesInt32(channel, 0, numberOfValues, dataInt32);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = double(dataInt32[i]);
            delete [] dataInt32;
            break;
        }
        case DDC_Float: {
            float *dataFloat = new float[numberOfValues];
            error = DDC_GetDataValuesFloat(channel, 0, numberOfValues, dataFloat);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = double(dataFloat[i]);
            delete [] dataFloat;
            break;
        }
        case DDC_Double: {
            error = DDC_GetDataValuesDouble(channel, 0, numberOfValues, data.data());
            break;
        }
        case DDC_String:
            break;
        case DDC_Timestamp: {
            unsigned int *year = new uint[numberOfValues];
            unsigned int *month = new uint[numberOfValues];
            unsigned int *day = new uint[numberOfValues];
            unsigned int *hour = new uint[numberOfValues];
            unsigned int *minute = new uint[numberOfValues];
            unsigned int *second = new uint[numberOfValues];
            double *milliSecond = new double[numberOfValues];
            unsigned int *weekDay = new uint[numberOfValues];
            error = DDC_GetDataValuesTimestampComponents(channel,0,numberOfValues,year,month,day,hour,
                                                         minute,second, milliSecond,weekDay);
            auto firstStamp = QDateTime(QDate(year[0],month[0],day[0]),
                    QTime(hour[0], minute[0], second[0],int(milliSecond[0])));
            for (uint i=0; i<numberOfValues; ++i) {
                auto stamp = QDateTime(QDate(year[i],month[i],day[i]),
                        QTime(hour[i], minute[i], second[i],int(milliSecond[i])));
                data[i] = firstStamp.msecsTo(stamp);
            }
            delete [] year;
            delete [] month;
            delete [] day;
            delete [] hour;
            delete [] minute;
            delete [] second;
            delete [] milliSecond;
            delete [] weekDay;
            break;
        }
        default: break;
    }
    if (error < 0) {
        LOG(ERROR) << DDC_GetLibraryErrorDescription(error);
    }
    return data;
}

Descriptor::DataType TDMSChannel::type() const
{
    //TODO: добавить поддержку разных сигналов в TDMS
    return Descriptor::DataType(NITypeToUffType(properties));
}

void TDMSChannel::populate()
{
    _data->clear();
    setPopulated(false);

    QVector<double> vec = getDouble();
    if (!vec.isEmpty()) {
        this->data()->setYValues(vec, DataHolder::YValuesReals, 0);
        setPopulated(true);
    }
}

FileDescriptor *TDMSChannel::descriptor() const
{
    return parent;
}

int TDMSChannel::index() const
{
    return parent->channels.indexOf(const_cast<TDMSChannel*>(this), 0);
}



/*****************************************************/

void TDMSGroup::read()
{
}

void TDMSGroup::write()
{
}

//QDateTime TDMSGroup::dateTime() const
//{
//    return parent->properties.value("datetime").toDateTime();
//}

void TDMSGroup::deleteChannels(const QVector<int> &)
{
}

int TDMSGroup::channelsCount() const
{
    return channels.count();
}

void TDMSGroup::move(bool , const QVector<int> &, const QVector<int> &)
{
}

Channel *TDMSGroup::channel(int index) const
{
    if (index >=0 && index < channels.size()) return channels.at(index);
    return 0;
}

