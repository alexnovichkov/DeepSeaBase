#include "matlabfiledescriptor.h"
#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "converters.h"
#include "algorithms.h"


QString lms2dsunit(const QString &unit)
{
    if (QString("m;cm;mm;nm;km;stat.mi;in;mil;м;см;мм;нм;км").split(";").contains(unit))
        return "м";

   if (QString("m/s;nm/s;km/h;nm/s;in/s;mph;m/s(pv);м/с;см/с;мм/с;нм/с;км/с").split(";").contains(unit))
       return "м/с";

   if (QString("m/s2;g;m/s?;m/s^2;(m/s^2);м/с^2;м/с2").split(";").contains(unit))
          return "м/с^2";
   if (QString("Pa;hPa;kPa;MPa;N/m2;N/mm2;psi;kg/(ms2);bar;Па;гПа;кПа;МПа;Н/м2;Н/м^2;Н/мм2;Н/мм^2").split(";").contains(unit))
       return "Па";
   if (QString("N;lbf;kN;daN;Н;кН").split(";").contains(unit))
       return "Н";
   if (QString("Nm;kNm;Нм;кНм").split(";").contains(unit))
       return "Нм";
   return unit;
}

#include <QXmlStreamReader>

template <typename T>
T findSubrecord(const QString &name, MatlabStructArray *rec)
{
    T result;
    int index = rec->fieldNames.indexOf(name);
    if (index >=0) result = dynamic_cast<T>(rec->subRecords[index]);
    return result;
}

MatlabConvertor::MatlabConvertor(QObject *parent) : QObject(parent)
{

}

bool MatlabConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    // Reading XML file
    QList<Dataset> sets = readXml(noErrors);
    if (!noErrors) {
        emit message("<font color=red>Error!</font> Не удалось прочитать файл " + xmlFileName);
        emit finished();
        return false;
    }

    emit message("Содержит следующие записи:");
    foreach (const Dataset &dataset, sets) {
        emit message(QString("%1: %2").arg(dataset.id).arg(dataset.fileName));
    }


    //Converting
    foreach(const QString &fi, filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("Конвертируем файл " + fi);
        QString xdfFileName = fi;
        xdfFileName.replace(".mat",".xdf");

        Dataset set;
        //search for the particular dataset
        foreach (const Dataset &s, sets) {
            if (s.fileName.toLower() == QFileInfo(xdfFileName).fileName().toLower()) {
                set = s;
                break;
            }
        }
        if (set.fileName.isEmpty()) {
            emit message("<font color=red>Error!</font> Не могу найти "+QFileInfo(xdfFileName).fileName()+" в Analysis.xml. Файл mat будет пропущен!");
            noErrors = false;
            emit tick();
            continue;
        }
        else {
            emit message("Файл "+QFileInfo(xdfFileName).fileName()+" в Analysis.xml записан как канал "+set.id);
        }

        //reading mat file structure
//        MatlabFile matlabFile(fi);
//        matlabFile.read();
//        if (matlabFile.writtenAsMatrix) {
//            emit message(QString("-- Файл mat записан матрицей. Он будет пропущен. Экспортируйте файл без галочки \"Group similar blocks in a matrix\""));
//            emit tick();
//            noErrors = false;
//            continue;
//        }
//        emit message(QString("-- Файл mat содержит %1 каналов").arg(matlabFile.channels.size()));
        MatFile matlabFile(fi);
        emit message(QString("-- Файл mat содержит %1 переменных").arg(matlabFile.records.size()));

        QString rawFileName = fi;
        rawFileName.replace(".mat",".raw");

        QString dfdFileName = fi;
        dfdFileName.replace(".mat",".dfd");

        //writing dfd file
        DfdFileDescriptor dfdFileDescriptor(dfdFileName);
        dfdFileDescriptor.fillPreliminary(Descriptor::TimeResponse);
        QDate d = QDate::fromString(set.date, "dd.MM.yy");
        if (d.year()<1950) {
            d=d.addYears(100);
        }
        dfdFileDescriptor.Date = d;
        dfdFileDescriptor.Time = QTime::fromString(set.time, "hh:mm:ss");
        DataDescription *datade = new DataDescription(&dfdFileDescriptor);
        datade->data=(DescriptionList()<<DescriptionEntry("Заголовок 1",set.titles.at(0))
                                            << DescriptionEntry("Заголовок 2",set.titles.at(1))
                                            << DescriptionEntry("Заголовок 3",set.titles.at(2)));

        // эти переменные заполняем по первой записи в файле mat
        bool isSet = false;
        int samplescount = 0;
        double xBegin = 0;
        double xStep = 0;

        //writing raw file channel by channel (blockSize=0)
        QFile rawFile(rawFileName);
        if (rawFile.open(QFile::WriteOnly)) {
            int channelIndex=-1;

            for (int i=0; i<matlabFile.records.size(); ++i) {
                if (QThread::currentThread()->isInterruptionRequested()) return false;

                MatlabStructArray *rec = dynamic_cast<MatlabStructArray *>(matlabFile.records[i]);
                if (!rec) {
                    emit message("<font color=red>Error!</font> Странный тип переменной в файле "+matlabFile.fileName);
                    noErrors = false;
                    continue;
                }
                // определяем нужные переменные оси Х
                MatlabStructArray *x_values = findSubrecord<MatlabStructArray *>("x_values", rec);
                if (!x_values) {
                    emit message("<font color=red>Error!</font> Не могу прочитать параметры оси X в записи "+rec->name);
                    noErrors = false;
                    continue;
                }
                //MatlabStructArray *x_values = dynamic_cast<MatlabStructArray *>(rec->subRecords[0]);
//                MatlabNumericArray *startValue = dynamic_cast<MatlabNumericArray*>(x_values->subRecords[0]);
                MatlabNumericArray *startValue = findSubrecord<MatlabNumericArray*>("start_value", x_values);
                MatlabNumericArray *increment = findSubrecord<MatlabNumericArray*>("increment", x_values);
                MatlabNumericArray *numberOfValues = findSubrecord<MatlabNumericArray*>("number_of_values", x_values);


                if (!isSet) {
                    if (startValue) xBegin = startValue->getNumericAsDouble().first();
                    if (increment) xStep = increment->getNumericAsDouble().first();
                    if (numberOfValues) samplescount = numberOfValues->getNumericAsInt().first();
                    isSet = true;
                }

                // теперь данные по оси Y
                MatlabStructArray *y_values = findSubrecord<MatlabStructArray *>("y_values", rec);
                if (!y_values) {
                    emit message("<font color=red>Error!</font> Не могу прочитать данные в записи "+rec->name);
                    noErrors = false;
                    continue;
                }
                MatlabNumericArray *values = findSubrecord<MatlabNumericArray *>("values", y_values);
                if (!values) {
                    emit message("<font color=red>Error!</font> Не могу прочитать данные в записи "+rec->name);
                    noErrors = false;
                    continue;
                }
                MatlabNumericRecord *real_values = dynamic_cast<MatlabNumericRecord *>(values->realValues);
                if (!real_values) {
                    emit message("<font color=red>Error!</font> Не могу прочитать данные в записи "+rec->name);
                    noErrors = false;
                    continue;
                }
                MatlabStructArray *function_record = findSubrecord<MatlabStructArray *>("function_record", rec);
                if (!function_record) {
                    emit message("<font color=red>Error!</font> Не могу прочитать параметры записи "+rec->name);
                    noErrors = false;
                    continue;
                }


// ищем нужные каналы
                QStringList channelIDs;

                int typeIndex = function_record->fieldNames.indexOf("type");
//                qDebug()<<"field names"<<function_record->fieldNames;
//                qDebug()<<"type"<<function_record->subRecords[typeIndex]->getString();
//                qDebug()<<"rec name"<<rec->name;

                if (typeIndex >=0 &&
                    function_record->subRecords[typeIndex]->getString() == "Signal" &&
                    rec->name != "Signal") {// type = signal, несгруппированные временные данные
                    channelIDs << rec->name.section("_", 0, 0);
                }
                else if (rec->name == "Signal") {// сгруппированные временные данные
                    MatlabCellArray *name = dynamic_cast<MatlabCellArray *>(function_record->subRecords[0]);
                    if (!name || function_record->fieldNames.at(0) != "name") {
                        emit message("<font color=red>Error!</font> Не могу прочитать названия каналов в записи "+rec->name);
                        noErrors = false;
                        continue;
                    }
                    for (int cell = 0; cell < name->subRecords.size(); ++cell)
                        channelIDs << name->subRecords[cell]->getString().section(" ", 0, 0);
                }
                else {
                    emit message("<font color=red>Error!</font> Пропускаю математические каналы ");
                    noErrors = false;
                    continue;
                }

                // один раз прочитали данные этой переменной, далее обращаемся только к ним
                QByteArray rawFromMatfile = real_values->getRaw();
                if (rawFromMatfile.isEmpty()) {
                    emit message(QString("<font color=red>Error!</font> Не могу прочитать канал %1").arg(i+1));
                    noErrors = false;
                    continue;
                }

// перебираем каждый канал и записываем, если нашли
                for (int channelID = 0; channelID < channelIDs.size(); ++channelID) {
                    if (channelIDs.at(channelID).isEmpty()) continue;
                    XChannel c;
                    for (int j=0; j<set.channels.size(); ++j) {
                        if (set.channels.at(j).catLabel == channelIDs.at(channelID)) {
                            c = set.channels.at(j);
                            break;
                        }
                    }
                    if (c.catLabel.isEmpty()) {
                        emit message("<font color=red>Error!</font> Не могу найти канал " + channelIDs.at(channelID));
                        noErrors = false;
                        continue;
                    }

                    // теперь данные
                    QVector<float> data;
                    float max = 1.0;

                    if (rawFileFormat == 1) {// целые числа
                        int idx = rawFromMatfile.size() / channelIDs.size();
                        QByteArray raw = rawFromMatfile.mid(idx * channelID, idx);
                        QDataStream stream(&raw, QIODevice::ReadOnly);
                        stream.setByteOrder(QDataStream::LittleEndian);
                        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                        data = readNumeric<float>(&stream, real_values->actualDataSize / channelIDs.size(), real_values->header->type);

                        if (data.isEmpty()) {
                            emit message(QString("<font color=red>Error!</font> Неизвестный формат данных в канале %1").arg(i+1));
                            noErrors = false;
                            break;
                        }

                        max = qAbs(data[0]);
                        for (int k=1; k<data.size();++k)
                            if (qAbs(data[k]) > max) max = qAbs(data[k]);

                        for (int k=0; k<data.size();++k)
                            data[k] = data[k]/max*32768.0;

                        stream.setDevice(&rawFile);
                        stream.setByteOrder(QDataStream::LittleEndian);
                        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                        for (int k = 0; k < data.size(); ++k) {
                            int v = qMin(65535,int(data[k])+32768);
                            stream << (quint16)v;
                        }
                    }
                    else {// вещественные числа
                        // в зависимости от типа данных в массиве жизнь или сильно упрощается, либо совсем наоборот
                        int idx = rawFromMatfile.size() / channelIDs.size();
                        QByteArray raw = rawFromMatfile.mid(idx * channelID, idx);

                        if (real_values->header->type == MatlabRecord::miSINGLE) {
                            //ничего не меняем, записываем сырые данные.
                            rawFile.write(raw);
                        }
                        else {
                            QDataStream stream(&raw, QIODevice::ReadOnly);
                            stream.setByteOrder(QDataStream::LittleEndian);
                            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                            data = readNumeric<float>(&stream, real_values->actualDataSize / channelIDs.size(), real_values->header->type);

                            if (data.isEmpty()) {
                                emit message(QString("<font color=red>Error!</font> Неизвестный формат данных в канале %1").arg(i+1));
                                noErrors = false;
                                break;
                            }

                            stream.setDevice(&rawFile);
                            stream.setByteOrder(QDataStream::LittleEndian);
                            stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
                            int idx = data.size();
                            for (int k = 0; k < idx; ++k) {
                                stream << data[k];
                            }
                        }
                    }


                    if (rawFileFormat == 0) {// данные в формате single
                        DfdChannel *channel = new DfdChannel(&dfdFileDescriptor, ++channelIndex);
                        channel->setName(c.generalName+" -"+c.pointId+"-"+c.direction);
                        channel->ChanAddress = QString("SCADAS\\")+c.catLabel;
                        c.info.append(channel->ChanAddress);
                        channel->setDescription(c.info.join(" \\"));
                        channel->IndType = 0xC0000004; //характеристика отсчета
                        channel->ChanBlockSize = samplescount; //размер блока в отсчетах
                        channel->YName = c.units;
                        channel->InputType="U";

                        dfdFileDescriptor.channels.append(channel);
                    }
                    else {
                        RawChannel *channel = new RawChannel(&dfdFileDescriptor, ++channelIndex);
                        channel->setName(c.generalName+" -"+c.pointId+"-"+c.direction);
                        channel->ChanAddress = QString("SCADAS\\")+c.catLabel;
                        c.info.append(channel->ChanAddress);
                        channel->setDescription(c.info.join(" \\"));
                        channel->IndType = 2; //характеристика отсчета
                        channel->ChanBlockSize = samplescount; //размер блока в отсчетах
                        channel->YName = c.units;
                        channel->InputType="U";

                        channel->BandWidth = c.fd / 2.56;
                        channel->AmplShift = 0.0;
                        channel->AmplLevel = 5.12 / max;
                        channel->Sens0Shift = 0.0;
                        channel->ADC0 = hextodouble("FEF2C98AE17A14C0");
                        channel->ADCStep = hextodouble("BEF8BF05F67A243F");
                        channel->SensSensitivity = hextodouble("000000000000F03F");
                        channel->SensName = c.sensorName+"\\ sn-"+c.sensorSerial;

                        dfdFileDescriptor.channels.append(channel);
                    }
                }
            }
        }
        dfdFileDescriptor.dataDescription = datade;
        dfdFileDescriptor.setSamplesCount(samplescount);
        dfdFileDescriptor.BlockSize = 0;
        dfdFileDescriptor.XName="с";
        dfdFileDescriptor.XBegin = xBegin;
        dfdFileDescriptor.XStep = xStep;
        dfdFileDescriptor.DescriptionFormat = "lms2dfd.DF";
        dfdFileDescriptor.CreatedBy = "Конвертер lms2raw by Алексей Новичков";
        if (rawFileFormat == 0)
            dfdFileDescriptor.DataType = CuttedData; // насильно записываем данные как Raw, потому что DeepSea
                                                     // не умеет работать с файлами DataType=1, у которых IndType=2
        else
            dfdFileDescriptor.DataType = SourceData;
        dfdFileDescriptor.setChanged(true);
        dfdFileDescriptor.write();
        emit message("Готово.");
        emit tick();
    }

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();
    return true;
}

QList<Dataset> MatlabConvertor::readXml(bool &success)
{
    QList<Dataset> sets;
    QFile xmlFile(xmlFileName);
    if (!xmlFile.open(QFile::ReadOnly | QFile::Text)) {
        emit message("<font color=red>Error!</font> Не могу прочитать файл " + xmlFileName);
        emit finished();
        success = false;
        return sets;
    }
    QXmlStreamReader xml(&xmlFile);
    bool inDatasets = false;
    bool inChannel = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            QStringRef name=xml.name();
            if (name == "Datasets") inDatasets = true;
            if (name == "Dataset" && inDatasets) {
                Dataset set;
                if (xml.attributes().hasAttribute("Id"))
                    set.id = xml.attributes().value("Id").toString();
                sets << set;
            }
            else if (name == "File" && inDatasets && !inChannel) {
                if (xml.attributes().hasAttribute("Name")) {
                    if (sets.last().fileName.isEmpty())
                        sets.last().fileName = xml.attributes().value("Name").toString();
                }
            }
            else if (name == "Titles" && inDatasets) {
                if (xml.attributes().hasAttribute("Title1")) {
                    sets.last().titles.append(xml.attributes().value("Title1").toString());
                    sets.last().titles.append(xml.attributes().value("Title2").toString());
                    sets.last().titles.append(xml.attributes().value("Title3").toString());
                }
            }
            else if (name == "Date" && inDatasets) {
                if (xml.attributes().hasAttribute("Date"))
                    sets.last().date = xml.attributes().value("Date").toString();
                if (xml.attributes().hasAttribute("Time"))
                    sets.last().time = xml.attributes().value("Time").toString();
            }
            else if (name == "Channel" && inDatasets) {
                inChannel = true;
                XChannel c;
                if (xml.attributes().hasAttribute("name"))
                    c.units = xml.attributes().value("name").toString();
//                if (xml.attributes().hasAttribute("logref"))
//                    c.logRef = xml.attributes().value("logref").toString().toDouble();
//                if (xml.attributes().hasAttribute("scale"))
//                    c.scale = xml.attributes().value("scale").toString().toDouble();
                sets.last().channels.append(c);
            }
            else if (name == "General" && inDatasets) {
                if (xml.attributes().hasAttribute("Name"))
                    sets.last().channels.last().generalName = xml.attributes().value("Name").toString();
                if (xml.attributes().hasAttribute("CatLabel"))
                    sets.last().channels.last().catLabel = xml.attributes().value("CatLabel").toString();
            }
            else if (name == "Sensor" && inDatasets) {
//                if (xml.attributes().hasAttribute("id"))
//                    sets.last().channels.last().sensorId = xml.attributes().value("id").toString();
                if (xml.attributes().hasAttribute("serial"))
                    sets.last().channels.last().sensorSerial = xml.attributes().value("serial").toString();
                if (xml.attributes().hasAttribute("name"))
                    sets.last().channels.last().sensorName = xml.attributes().value("name").toString();
            }
            else if (name == "Xaxis" && inDatasets) {
                sets.last().channels.last().fd = 1.0 / xml.attributes().value("Delta").toString().toDouble();
            }
            else if (name == "Description" && inDatasets && !sets.isEmpty() && !sets.last().channels.isEmpty()) {
                if (xml.attributes().hasAttribute("ChannelUnit"))
                    sets.last().channels.last().chanUnits = xml.attributes().value("ChannelUnit").toString();
                if (xml.attributes().hasAttribute("PointID"))
                    sets.last().channels.last().pointId = xml.attributes().value("PointID").toString();
                if (xml.attributes().hasAttribute("Direction"))
                    sets.last().channels.last().direction = xml.attributes().value("Direction").toString();
            }
            else if (name == "Information" && inDatasets) {
                if (xml.attributes().hasAttribute("Info1")) {
                    sets.last().channels.last().info << xml.attributes().value("Info1").toString();
                    sets.last().channels.last().info << xml.attributes().value("Info2").toString();
                    sets.last().channels.last().info << xml.attributes().value("Info3").toString();
                    sets.last().channels.last().info << xml.attributes().value("Info4").toString();
                }
            }
        }
        else if (xml.isEndElement()) {
            if (xml.name() == "Datasets") inDatasets = false;
            else if (xml.name() == "Channel") inChannel = false;
        }
    }
    if (xml.hasError()) {
        emit message(xml.errorString());
    }
    return sets;
}


