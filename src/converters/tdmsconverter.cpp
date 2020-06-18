#include "tdmsconverter.h"

#include "logging.h"
#include "algorithms.h"
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"
#include "fileformats/tdmsfile.h"

TDMSFileConverter::TDMSFileConverter(QObject *parent) : QObject(parent)
{DDD;

}

bool TDMSFileConverter::convert()
{DDD;
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    //Converting
    for (const QString &tdmsFileName: filesToConvert) {
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
            if (g->properties.value("DecimationLevel").toString() == "0") {
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

        FileDescriptor *destinationFile = FormatFactory::createDescriptor(*group,
                                                                          destinationFileName);
        if (destinationFile)
            newFiles << destinationFile->fileName();
        delete destinationFile;

        emit message("Готово.");
        emit tick();
    }

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();
    return true;
}

//FileDescriptor *TDMSFileConverter::saveAs(const QString &name, TDMSGroup *g)
//{DDD;
//    QString suffix = QFileInfo(name).suffix().toLower();
//    if (suffix == "dfd") return saveAsDfd(name, g);
//    if (suffix == "d94") return saveAsD94(name, g);
//    if (suffix == "uff") return saveAsUff(name, g);
//    return 0;
//}

//FileDescriptor *TDMSFileConverter::saveAsDfd(const QString &name, TDMSGroup *g)
//{DDD;
//    QString rawFileName = changeFileExt(name, "raw");

//    //writing dfd file
//    auto dfdFileDescriptor = new DfdFileDescriptor(name);
//    dfdFileDescriptor->rawFileName = rawFileName;
//    dfdFileDescriptor->updateDateTimeGUID();
//    dfdFileDescriptor->DataType = CuttedData;

//    if (g->parent->properties.contains("datetime")) {
//        dfdFileDescriptor->Date = g->parent->properties.value("datetime").toDateTime().date();
//        dfdFileDescriptor->Time = g->parent->properties.value("datetime").toDateTime().time();
//    }
//    else {
//        dfdFileDescriptor->Date = QDate::currentDate();
//        dfdFileDescriptor->Time = QTime::currentTime();
//    }



//    DataDescription *datade = new DataDescription(dfdFileDescriptor);
//    QString s = g->parent->properties.value("name").toString();
//    if (!s.isEmpty()) datade->data.append(DescriptionEntry("name",s));
//    s = g->parent->properties.value("description").toString();
//    if (!s.isEmpty()) datade->data.append(DescriptionEntry("description",s));
//    dfdFileDescriptor->dataDescription = datade;

//    // эти переменные заполняем по первой записи в файле
//    bool isSet = false;
//    quint64 samplescount = 0;
//    double xBegin = 0;
//    double xStep = 0;

//    //writing raw file channel by channel (blockSize=0)
//    QFile rawFile(rawFileName);
//    if (rawFile.open(QFile::WriteOnly)) {

//        for (int i=0; i<g->channels.size(); ++i) {
//            // определяем нужные переменные оси Х
//            if (!isSet) {
//                xBegin = g->channels[i]->properties.value("wf_start_offset").toDouble();
//                xStep = g->channels[i]->properties.value("wf_increment").toDouble();
//                samplescount = g->channels[i]->numberOfValues;
//                isSet = true;
//            }

//            // теперь данные по оси Y

//            QVector<float> data = g->channels[i]->getFloat();
//            if (data.isEmpty()) {
//                emit message(QString("<font color=red>Error!</font> Не могу прочитать канал %1").arg(i+1));
//                continue;
//            }


//            // теперь данные
//            float max = 1.0;
//            QDataStream stream(&rawFile);
//            stream.setByteOrder(QDataStream::LittleEndian);
//            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

//            if (rawFileFormat == 1) {// целые числа
//                max = qAbs(data[0]);
//                for (int k=1; k<data.size();++k)
//                    if (qAbs(data[k]) > max) max = qAbs(data[k]);

//                for (int k = 0; k < data.size(); ++k) {
//                    data[k] = data[k]/max*32768.0;
//                    int v = qMin(65535,int(data[k])+32768);
//                    stream << (quint16)v;
//                }
//            }
//            else {// вещественные числа
//                int idx = data.size();
//                for (int k = 0; k < idx; ++k) {
//                    stream << data[k];
//                }
//            }



//            if (rawFileFormat == 0) {// данные в формате float
//                DfdChannel *channel = new DfdChannel(dfdFileDescriptor, i);
//                channel->setName(g->channels[i]->properties.value("name").toString());
//                channel->ChanAddress = g->channels[i]->properties.value("NI_ChannelName").toString();
//                channel->setDescription(g->channels[i]->properties.value("description").toString());
//                channel->IndType = 0xC0000004; //характеристика отсчета
//                channel->ChanBlockSize = samplescount; //размер блока в отсчетах
//                channel->YName = g->channels[i]->properties.value("unit_string").toString();
//                channel->InputType="U";
//            }
//            else {
//                RawChannel *channel = new RawChannel(dfdFileDescriptor, i);
//                channel->setName(g->channels[i]->properties.value("name").toString());
//                channel->ChanAddress = g->channels[i]->properties.value("NI_ChannelName").toString();
//                channel->setDescription(g->channels[i]->properties.value("description").toString());
//                channel->IndType = 2; //характеристика отсчета
//                channel->ChanBlockSize = samplescount; //размер блока в отсчетах
//                channel->YName = g->channels[i]->properties.value("unit_string").toString();
//                channel->InputType="U";

//                channel->BandWidth = float(1.0/xStep/ 2.56);
//                channel->AmplShift = 0.0;
//                channel->AmplLevel = 5.12 / max;
//                channel->Sens0Shift = 0.0;
//                channel->ADC0 = hextodouble("FEF2C98AE17A14C0");
//                channel->ADCStep = hextodouble("BEF8BF05F67A243F");
//                channel->SensSensitivity = hextodouble("000000000000F03F");
//                //channel->SensName = c.sensorName+"\\ sn-"+c.sensorSerial;
//            }
//        }
//    }

//    dfdFileDescriptor->setSamplesCount(samplescount);
//    dfdFileDescriptor->BlockSize = 0;
//    dfdFileDescriptor->XName="с";
//    dfdFileDescriptor->XBegin = xBegin;
//    dfdFileDescriptor->XStep = xStep;
//    dfdFileDescriptor->DescriptionFormat = "lms2dfd.DF";
//    dfdFileDescriptor->CreatedBy = "Конвертер tdms2raw by Алексей Новичков";
//    if (rawFileFormat == 0)
//        dfdFileDescriptor->DataType = CuttedData; // насильно записываем данные как Raw, потому что DeepSea
//                                                 // не умеет работать с файлами DataType=1, у которых IndType=2
//    else
//        dfdFileDescriptor->DataType = SourceData;
//    dfdFileDescriptor->setChanged(true);
//    dfdFileDescriptor->write();
//    return dfdFileDescriptor;
//}

//FileDescriptor *TDMSFileConverter::saveAsD94(const QString &name, TDMSGroup *g)
//{DDD;
//    auto file = new Data94File(name);

//    file->updateDateTimeGUID();

//    if (g->parent->properties.contains("datetime")) {
//        file->setDateTime(g->parent->properties.value("datetime").toDateTime());
//    }

//    DescriptionList data;
//    QString s = g->parent->properties.value("name").toString();
//    if (!s.isEmpty()) data.append(DescriptionEntry("name",s));
//    s = g->parent->properties.value("description").toString();
//    if (!s.isEmpty()) data.append(DescriptionEntry("description",s));
//    file->setDataDescriptor(data);

//    QFile f(name);
//    if (!f.open(QFile::WriteOnly)) {
//        qDebug()<<"Не удалось открыть файл для записи:"<<name;
//        delete file;
//        return 0;
//    }

//    QDataStream r(&f);
//    r.setByteOrder(QDataStream::LittleEndian);
//    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

//    //переписываем описательную часть
//    r.device()->write("data94  ");

//    QJsonDocument doc(description);
//    QByteArray json = doc.toJson();
//    descriptionSize = json.size();
//    r << descriptionSize;
//    r.writeRawData(json.data(), descriptionSize);

//    quint32 ccount = indexes.size();
//    r << ccount;

//    for (int i: indexes) {
//        Channel *sourceChannel = other.channel(i);
//        Data94Channel *c = new Data94Channel(sourceChannel, this);

//        bool populated = sourceChannel->populated();
//        if (!populated) sourceChannel->populate();

//        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
//        qint64 pos = r.device()->pos();
//        r.device()->write("d94chan ");

//        //description
//        QJsonDocument doc(c->_description);
//        QByteArray json = doc.toJson();
//        c->descriptionSize = json.size();
//        r << c->descriptionSize;
//        r.writeRawData(json.data(), c->descriptionSize);

//        c->position = pos;

//        c->xAxisBlock.write(r);
//        c->zAxisBlock.write(r);

//        const quint32 format = c->isComplex ? 2 : 1;
//        r << format;
//        r << c->sampleWidth;

//        if (c->sampleWidth == 8)
//            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

//        c->dataPosition = r.device()->pos();
//        for (int block = 0; block < c->data()->blocksCount(); ++block) {
//            if (!c->isComplex) {
//                const QVector<double> yValues = sourceChannel->data()->rawYValues(block);
//                if (yValues.isEmpty()) {
//                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
//                    continue;
//                }

//                for (double v: yValues) {
//                    if (c->sampleWidth == 4)
//                        r << (float)v;
//                    else
//                        r << v;
//                }
//            } // !c->isComplex
//            else {
//                const auto yValues = sourceChannel->data()->yValuesComplex(block);
//                if (yValues.isEmpty()) {
//                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
//                    continue;
//                }
//                for (cx_double v: yValues) {
//                    if (c->sampleWidth == 4) {
//                        r << (float)v.real();
//                        r << (float)v.imag();
//                    }
//                    else {
//                        r << v.real();
//                        r << v.imag();
//                    }
//                }
//            } // c->isComplex
//        }
//        c->size = r.device()->pos() - c->position;

//        if (!populated) {
//            sourceChannel->clear();
//        }
//    }
//    return file;
//}

//FileDescriptor *TDMSFileConverter::saveAsUff(const QString &name, TDMSGroup *g)
//{DDD;
//    auto file = new UffFileDescriptor(name);

//    return file;
//}
