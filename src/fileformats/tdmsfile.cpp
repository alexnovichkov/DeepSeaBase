#include "tdmsfile.h"


#include <QDateTime>
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "algorithms.h"

TDMSFile::TDMSFile(const QString &fileName) : fileName(fileName)
{DD;
    int error;

    QString fn = fileName;
    fn.replace("/","\\");

    error = DDC_OpenFileEx(fn.toUtf8().data(), 0, 1, &file);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        _isValid = false;
        return;
    }

    // Reading file properties
    uint numberOfProperties = 0;

    error = DDC_GetNumFileProperties(file, &numberOfProperties);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"File contains"<<numberOfProperties<<"properties";

    for (size_t i=0; i<numberOfProperties; ++i) {
        //property name length
        size_t length;
        error = DDC_GetFilePropertyNameLengthFromIndex(file, i, &length);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            return;
        }

        //property name
        char *nameBuf;
        nameBuf = (char*)malloc(length+1);
        error = DDC_GetFilePropertyNameFromIndex(file, i, nameBuf, length+1);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            return;
        }
        QString name = QString::fromLocal8Bit(nameBuf);

        //property type
        DDCDataType dataType;
        error = DDC_GetFilePropertyType(file, nameBuf, &dataType);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            return;
        }

        //property data
        QVariant value;
        switch (dataType) {
            case DDC_UInt8: {
                quint8 val;
                error = DDC_GetFileProperty(file, nameBuf, &val, 1);
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value =val;
                break;
            }
            case DDC_Int16: {
                qint16 val;
                error = DDC_GetFileProperty(file, nameBuf, &val, 2);
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value =val;
                break;
            }
            case DDC_Int32: {
                qint32 val;
                error = DDC_GetFileProperty(file, nameBuf, &val, 4);
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value =val;
                break;
            }
            case DDC_Float: {
                float val;
                error = DDC_GetFileProperty(file, nameBuf, &val, sizeof(float));
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value =val;
                break;
            }
            case DDC_Double: {
                double val;
                error = DDC_GetFileProperty(file, nameBuf, &val, sizeof(double));
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value =val;
                break;
            }
            case DDC_String: {
                char *buffer=0;
                uint propLength;
                error = DDC_GetFileStringPropertyLength (file, nameBuf, &propLength);
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }

                if (propLength > 0) {
                    buffer = (char*)malloc(propLength+1);
                    error = DDC_GetFileProperty(file, nameBuf, buffer, propLength+1);
                    if (error < 0) {
                        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                        return;
                    }
                    value = QString::fromLocal8Bit(buffer);
                    free (buffer);
                }
                break;
            }
            case DDC_Timestamp: {
                uint year, month, day, hour, minute, second, weekDay;
                double millisecond;
                error = DDC_GetFilePropertyTimestampComponents(file, nameBuf, &year, &month, &day,
                                                               &hour, &minute, &second, &millisecond, &weekDay);
                if (error < 0) {
                    qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
                    return;
                }
                value = QDateTime(QDate(year,month,day),QTime(hour, minute, second,int(millisecond)));
                break;
            }
            default: break;
        }

//        qDebug()<<"Property"<<i+1<<name<<value.toString();
        properties.insert(name, value);

        free(nameBuf);
    }

    // Reading Channel groups
    uint numberOfChannelGroups=0;
    error = DDC_GetNumChannelGroups(file, &numberOfChannelGroups);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"\nChannel groups:"<<numberOfChannelGroups;

    _groups = (DDCChannelGroupHandle *)calloc(numberOfChannelGroups, sizeof (DDCChannelGroupHandle));
    error = DDC_GetChannelGroups(file, _groups, numberOfChannelGroups);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }

    for (size_t i=0; i<numberOfChannelGroups; ++i) {
        TDMSGroup *group = new TDMSGroup(_groups[i], fileName);
        group->parent = this;
        groups << group;
    }
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
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"Channel group"<<"contains"<<numberOfProperties<<"properties";

    for (size_t j=0; j<numberOfProperties; ++j) {
        //property name length
        size_t length;
        error = DDC_GetChannelGroupPropertyNameLengthFromIndex(group, j, &length);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        //property name
        char *nameBuf;
        nameBuf = (char*)malloc(length+1);
        error = DDC_GetChannelGroupPropertyNameFromIndex(group, j, nameBuf, length+1);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        QString name = QString::fromLocal8Bit(nameBuf);
        //property type
        DDCDataType dataType;
        error = DDC_GetChannelGroupPropertyType(group, nameBuf, &dataType);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        //property data
        QVariant value;
        switch (dataType) {
            case DDC_UInt8: {
                quint8 val;
                error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 1);
                value =val;
                break;
            }
            case DDC_Int16: {
                qint16 val;
                error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 2);
                value =val;
                break;
            }
            case DDC_Int32: {
                qint32 val;
                error = DDC_GetChannelGroupProperty(group, nameBuf, &val, 4);
                value =val;
                break;
            }
            case DDC_Float: {
                float val;
                error = DDC_GetChannelGroupProperty(group, nameBuf, &val, sizeof(float));
                value =val;
                break;
            }
            case DDC_Double: {
                double val;
                error = DDC_GetChannelGroupProperty(group, nameBuf, &val, sizeof(double));
                value =val;
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
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
//        qDebug()<<"Channel group property"<<j+1<<name<<value.toString();
        properties.insert(name, value);

        free(nameBuf);
    }

    // number of channels
    uint numberOfChannels = 0;
    error = DDC_GetNumChannels(group, &numberOfChannels);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"Channel group"<<"contains"<<numberOfChannels<<"channels";

    // reading channels
    _channels = (DDCChannelHandle *)calloc(numberOfChannels, sizeof(DDCChannelHandle));
    error = DDC_GetChannels(group, _channels, numberOfChannels);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
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
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"Channel"<<"contains"<<numberOfProperties<<"properties";

    for (size_t k=0; k<numberOfProperties; ++k) {
        //property name length
        size_t length;
        error = DDC_GetChannelPropertyNameLengthFromIndex(channel, k, &length);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        //property name
        char *nameBuf;
        nameBuf = (char*)malloc(length+1);
        error = DDC_GetChannelPropertyNameFromIndex(channel, k, nameBuf, length+1);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        QString name = QString::fromLocal8Bit(nameBuf);
        //property type
        DDCDataType dataType;
        error = DDC_GetChannelPropertyType(channel, nameBuf, &dataType);
        if (error < 0) {
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
        //property data
        QVariant value;
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
            qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
            continue;
        }
//        qDebug()<<"Channel property"<<k+1<<name<<value.toString();
        properties.insert(name, value);

        free(nameBuf);
    }

    // reading data
    error = DDC_GetNumDataValues(channel, &numberOfValues);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
    error = DDC_GetDataType(channel, &dataType);
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
        return;
    }
//    qDebug()<<"Channel"<<"contains"<<numberOfValues<<"values of type"<<dataType;

    double xBegin = properties.value("wf_start_offset").toDouble();
    double xStep = properties.value("wf_increment").toDouble();
    data()->setXValues(xBegin, xStep, numberOfValues);

    data()->setZValues(0.0, 1.0, 1);

    data()->setThreshold(properties.value("NI_dbReference").toDouble());
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
        case DDC_Timestamp:
            data.clear();
            break;
        default: break;
    }
    if (error < 0) {
        qDebug() << "Error:" << DDC_GetLibraryErrorDescription(error);
    }
    return data;
}

QVariant TDMSChannel::info(int, bool) const
{
    return QVariant();
}

int TDMSChannel::columnsCount() const
{
    return 5;
}

QVariant TDMSChannel::channelHeader(int) const
{
    return QVariant();
}

Descriptor::DataType TDMSChannel::type() const
{
    //TODO: добавить поддержку разных сигналов в TDMS
    return Descriptor::TimeResponse;
}

int TDMSChannel::octaveType() const
{
    return 0;
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

QString TDMSChannel::name() const
{
    return properties.value("name").toString();
}

void TDMSChannel::setName(const QString &name)
{
    properties.insert("name", name);
}

QString TDMSChannel::description() const
{
    return properties.value("description").toString();
}

void TDMSChannel::setDescription(const QString &description)
{
    properties.insert("description", description);
}

QString TDMSChannel::xName() const
{
    return properties.value("wf_xunit_string").toString();
}

QString TDMSChannel::yName() const
{
    return properties.value("unit_string").toString();
}

QString TDMSChannel::zName() const
{
    return "";
}

void TDMSChannel::setYName(const QString &yName)
{
    properties.insert("unit_string", yName);
}

void TDMSChannel::setXName(const QString &)
{

}

void TDMSChannel::setZName(const QString &)
{

}

QString TDMSChannel::legendName() const
{
    return QString();
}

FileDescriptor *TDMSChannel::descriptor()
{
    return parent;
}

int TDMSChannel::index() const
{
    return parent->channels.indexOf(const_cast<TDMSChannel*>(this), 0);
}

QString TDMSChannel::correction() const
{
    return QString();
}

void TDMSChannel::setCorrection(const QString &)
{

}


/*****************************************************/

void TDMSGroup::fillPreliminary(FileDescriptor *file)
{
}

void TDMSGroup::read()
{
}

void TDMSGroup::write()
{
}

void TDMSGroup::writeRawFile()
{
}

void TDMSGroup::updateDateTimeGUID()
{
}

DescriptionList TDMSGroup::dataDescriptor() const
{
    return DescriptionList();
}

void TDMSGroup::setDataDescriptor(const DescriptionList &)
{
}

QString TDMSGroup::dataDescriptorAsString() const
{
    return "";
}

QDateTime TDMSGroup::dateTime() const
{
    return parent->properties.value("datetime").toDateTime();
}

void TDMSGroup::deleteChannels(const QVector<int> &)
{
}

void TDMSGroup::copyChannelsFrom(FileDescriptor *, const QVector<int> &)
{
}

QString TDMSGroup::calculateThirdOctave()
{
    return "";
}

void TDMSGroup::calculateMovingAvg(const QList<Channel *> &, int )
{
}

QString TDMSGroup::saveTimeSegment(double , double )
{
    return "";
}

int TDMSGroup::channelsCount() const
{
    return channels.count();
}

void TDMSGroup::move(bool , const QVector<int> &, const QVector<int> &)
{
}

QVariant TDMSGroup::channelHeader(int) const
{
    return QVariant();
}

int TDMSGroup::columnsCount() const
{
    return 1;
}

Channel *TDMSGroup::channel(int index) const
{
    if (index >=0 && index < channels.size()) return channels.at(index);
    return 0;
}

QString TDMSGroup::legend() const
{
    return QString();
}

bool TDMSGroup::setLegend(const QString &)
{
    return true;
}

double TDMSGroup::xStep() const
{
    if (channels.isEmpty()) return 0.0;
    return channels.constFirst()->data()->xStep();
}

void TDMSGroup::setXStep(const double)
{

}

double TDMSGroup::xBegin() const
{
    if (channels.isEmpty()) return 0.0;
    return channels.constFirst()->data()->xMin();
}

int TDMSGroup::samplesCount() const
{
    if (channels.isEmpty()) return 0;
    return channels.constFirst()->samplesCount();
}

void TDMSGroup::setSamplesCount(int)
{
}

QString TDMSGroup::xName() const
{
    if (channels.isEmpty()) return "";
    return channels.constFirst()->xName();
}

bool TDMSGroup::setDateTime(QDateTime)
{
    return true;
}

bool TDMSGroup::dataTypeEquals(FileDescriptor *) const
{
    return false;
}
