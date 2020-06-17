#include "tdmsfile.h"


#include <QtDebug>
#include <QDateTime>
#include "logging.h"
#include "fileformats/dfdfiledescriptor.h"
#include <QLibrary>
#include "algorithms.h"

TDMSFile::TDMSFile(const QString &fileName) : fileName(fileName)
{DDD;
    int error;

//    m.load();
//    if (!m.loaded) {
//        qDebug() << "Error: Не удалось загрузить библиотеку nilibddc";
//        _isValid = false;
//        return;
//    }

    QString fn = fileName;
    fn.replace("/","\\");

    error = DDC_OpenFileEx(fn.toUtf8().data(), 0, 1, &file);
//    error = DDC_OpenFileEx(fn.toUtf8().data(), 0, 1, &file);
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
        TDMSGroup *group = new TDMSGroup(_groups[i], &m);
        group->parent = this;
        groups << group;
    }
}

TDMSFile::~TDMSFile()
{DDD;
    free(_groups);
    qDeleteAll(groups);
    if (file) m.closeFile(file);
}

TDMSGroup::TDMSGroup(DDCChannelGroupHandle group, DDCMethods *m) : group(group), m(m)
{
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
        TDMSChannel *channel = new TDMSChannel(_channels[i], m);
        channel->parent = this;
        channels << channel;
    }
}

TDMSGroup::~TDMSGroup()
{
    free(_channels);
    qDeleteAll(channels);
    DDC_CloseChannelGroup(group);
}

TDMSChannel::TDMSChannel(DDCChannelHandle channel, DDCMethods *m) :channel(channel), m(m)
{
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
}

TDMSChannel::~TDMSChannel()
{
    DDC_CloseChannel(channel);
}

QVector<double> TDMSChannel::getDouble()
{
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

QVector<float> TDMSChannel::getFloat()
{
    QVector<float> data(numberOfValues);
    int error = 0;
    switch (dataType) {
        case DDC_UInt8: {// unsigned char
            unsigned char *dataUint8 = new uchar[numberOfValues];
            error = DDC_GetDataValuesUInt8(channel, 0, numberOfValues, dataUint8);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = float(dataUint8[i]);
            delete [] dataUint8;
            break;
        }
        case DDC_Int16: {
            short *dataInt16 = new short[numberOfValues];
            error = DDC_GetDataValuesInt16(channel, 0, numberOfValues, dataInt16);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = float(dataInt16[i]);
            delete [] dataInt16;
            break;
        }
        case DDC_Int32: {
            long *dataInt32 = new long[numberOfValues];
            error = DDC_GetDataValuesInt32(channel, 0, numberOfValues, dataInt32);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = float(dataInt32[i]);
            delete [] dataInt32;
            break;
        }
        case DDC_Float: {
            error = DDC_GetDataValuesFloat(channel, 0, numberOfValues, data.data());
            break;
        }
        case DDC_Double: {
            double *dataDouble = new double[numberOfValues];
            error = DDC_GetDataValuesDouble(channel, 0, numberOfValues, dataDouble);
            for (quint64 i=0; i<numberOfValues; ++i)
                data[i] = float(dataDouble[i]);
            delete [] dataDouble;
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

TDMSFileConvertor::TDMSFileConvertor(QObject *parent) : QObject(parent)
{

}

bool TDMSFileConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    //Converting
    for(const QString &tdmsFileName: filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("Конвертируем файл " + tdmsFileName);

        //reading tdms file structure
        TDMSFile tdmsFile(tdmsFileName);
        if (!tdmsFile.isValid()) {
            emit message("Ошибка: Файл поврежден и будет пропущен!");
            emit tick();
            noErrors = false;
            continue;
        }
        emit message(QString("-- Файл TDMS содержит %1 переменных").arg(tdmsFile.groups.size()));

        //ищем группу каналов, которая содержит все данные
        TDMSGroup *group = 0;
        for (TDMSGroup *g: tdmsFile.groups) {
            if (g->properties.value("name").toString().endsWith("All Data")) {
                group = g;
                break;
            }
        }
        if (!group) {
            emit message("В файле отсутствует нужная группа каналов");
            emit tick();
            continue;
        }

        QString destinationFileName = changeFileExt(tdmsFileName, destinationFormat);
        destinationFileName = createUniqueFileName(destinationFileName);

        FileDescriptor *d = saveAs(destinationFileName, group);
        if (d) newFiles << d->fileName();

        delete d;
        emit message("Готово.");
        emit tick();
    }

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();
    return true;
}

FileDescriptor *TDMSFileConvertor::saveAs(const QString &name, TDMSGroup *g)
{
    QString suffix = QFileInfo(name).suffix().toLower();
    if (suffix == "dfd") return saveAsDfd(name, g);
    if (suffix == "d94") return saveAsD94(name, g);
    if (suffix == "uff") return saveAsUff(name, g);
    return 0;
}

FileDescriptor *TDMSFileConvertor::saveAsDfd(const QString &name, TDMSGroup *g)
{
    QString rawFileName = changeFileExt(name, "raw");

    //writing dfd file
    auto dfdFileDescriptor = new DfdFileDescriptor(name);
    dfdFileDescriptor->rawFileName = rawFileName;
    dfdFileDescriptor->updateDateTimeGUID();
    dfdFileDescriptor->DataType = CuttedData;

    if (g->parent->properties.contains("datetime")) {
        dfdFileDescriptor->Date = g->parent->properties.value("datetime").toDateTime().date();
        dfdFileDescriptor->Time = g->parent->properties.value("datetime").toDateTime().time();
    }
    else {
        dfdFileDescriptor->Date = QDate::currentDate();
        dfdFileDescriptor->Time = QTime::currentTime();
    }



    DataDescription *datade = new DataDescription(dfdFileDescriptor);
    QString s = g->parent->properties.value("name").toString();
    if (!s.isEmpty()) datade->data.append(DescriptionEntry("name",s));
    s = g->parent->properties.value("description").toString();
    if (!s.isEmpty()) datade->data.append(DescriptionEntry("description",s));
    dfdFileDescriptor->dataDescription = datade;

    // эти переменные заполняем по первой записи в файле
    bool isSet = false;
    quint64 samplescount = 0;
    double xBegin = 0;
    double xStep = 0;

    //writing raw file channel by channel (blockSize=0)
    QFile rawFile(rawFileName);
    if (rawFile.open(QFile::WriteOnly)) {

        for (int i=0; i<g->channels.size(); ++i) {
            // определяем нужные переменные оси Х
            if (!isSet) {
                xBegin = g->channels[i]->properties.value("wf_start_offset").toDouble();
                xStep = g->channels[i]->properties.value("wf_increment").toDouble();
                samplescount = g->channels[i]->numberOfValues;
                isSet = true;
            }

            // теперь данные по оси Y

            QVector<float> data = g->channels[i]->getFloat();
            if (data.isEmpty()) {
                emit message(QString("<font color=red>Error!</font> Не могу прочитать канал %1").arg(i+1));
                continue;
            }


            // теперь данные
            float max = 1.0;
            QDataStream stream(&rawFile);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            if (rawFileFormat == 1) {// целые числа
                max = qAbs(data[0]);
                for (int k=1; k<data.size();++k)
                    if (qAbs(data[k]) > max) max = qAbs(data[k]);

                for (int k = 0; k < data.size(); ++k) {
                    data[k] = data[k]/max*32768.0;
                    int v = qMin(65535,int(data[k])+32768);
                    stream << (quint16)v;
                }
            }
            else {// вещественные числа
                int idx = data.size();
                for (int k = 0; k < idx; ++k) {
                    stream << data[k];
                }
            }



            if (rawFileFormat == 0) {// данные в формате float
                DfdChannel *channel = new DfdChannel(dfdFileDescriptor, i);
                channel->setName(g->channels[i]->properties.value("name").toString());
                channel->ChanAddress = g->channels[i]->properties.value("NI_ChannelName").toString();
                channel->setDescription(g->channels[i]->properties.value("description").toString());
                channel->IndType = 0xC0000004; //характеристика отсчета
                channel->ChanBlockSize = samplescount; //размер блока в отсчетах
                channel->YName = g->channels[i]->properties.value("unit_string").toString();
                channel->InputType="U";
            }
            else {
                RawChannel *channel = new RawChannel(dfdFileDescriptor, i);
                channel->setName(g->channels[i]->properties.value("name").toString());
                channel->ChanAddress = g->channels[i]->properties.value("NI_ChannelName").toString();
                channel->setDescription(g->channels[i]->properties.value("description").toString());
                channel->IndType = 2; //характеристика отсчета
                channel->ChanBlockSize = samplescount; //размер блока в отсчетах
                channel->YName = g->channels[i]->properties.value("unit_string").toString();
                channel->InputType="U";

                channel->BandWidth = float(1.0/xStep/ 2.56);
                channel->AmplShift = 0.0;
                channel->AmplLevel = 5.12 / max;
                channel->Sens0Shift = 0.0;
                channel->ADC0 = hextodouble("FEF2C98AE17A14C0");
                channel->ADCStep = hextodouble("BEF8BF05F67A243F");
                channel->SensSensitivity = hextodouble("000000000000F03F");
                //channel->SensName = c.sensorName+"\\ sn-"+c.sensorSerial;
            }
        }
    }

    dfdFileDescriptor->setSamplesCount(samplescount);
    dfdFileDescriptor->BlockSize = 0;
    dfdFileDescriptor->XName="с";
    dfdFileDescriptor->XBegin = xBegin;
    dfdFileDescriptor->XStep = xStep;
    dfdFileDescriptor->DescriptionFormat = "lms2dfd.DF";
    dfdFileDescriptor->CreatedBy = "Конвертер tdms2raw by Алексей Новичков";
    if (rawFileFormat == 0)
        dfdFileDescriptor->DataType = CuttedData; // насильно записываем данные как Raw, потому что DeepSea
                                                 // не умеет работать с файлами DataType=1, у которых IndType=2
    else
        dfdFileDescriptor->DataType = SourceData;
    dfdFileDescriptor->setChanged(true);
    dfdFileDescriptor->write();
    return dfdFileDescriptor;
}

FileDescriptor *TDMSFileConvertor::saveAsD94(const QString &name, TDMSGroup *g)
{

}

FileDescriptor *TDMSFileConvertor::saveAsUff(const QString &name, TDMSGroup *g)
{

}
