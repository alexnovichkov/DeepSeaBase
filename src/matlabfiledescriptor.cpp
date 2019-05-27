#include "matlabfiledescriptor.h"
#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "converters.h"

struct XChannel
{
    QString name;
    QString units;
    double logRef;
    double scale;
    QString generalName;
    QString catLabel;
    QString sensorId;
    QString sensorSerial;
    QString sensorName;
    double fd;
    QString chanUnits;
    QString pointId;
    QString direction;
    QStringList info;
};

struct Dataset
{
    QString id;
    QString fileName;
    QStringList titles;
    QString date;
    QString time;
    QList<XChannel> channels;
};

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


MatlabConvertor::MatlabConvertor(QObject *parent) : QObject(parent)
{

}

bool MatlabConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    // Reading XML file
    QList<Dataset> sets;
    QFile xmlFile(xmlFileName);
    if (!xmlFile.open(QFile::ReadOnly | QFile::Text)) {
        emit message("<font color=red>Error!</font> Не могу прочитать файл " + xmlFileName);
        emit finished();
        return false;
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
                if (xml.attributes().hasAttribute("logref"))
                    c.logRef = xml.attributes().value("logref").toString().toDouble();
                if (xml.attributes().hasAttribute("scale"))
                    c.scale = xml.attributes().value("scale").toString().toDouble();
                sets.last().channels.append(c);
            }
            else if (name == "General" && inDatasets) {
                if (xml.attributes().hasAttribute("Name"))
                    sets.last().channels.last().generalName = xml.attributes().value("Name").toString();
                if (xml.attributes().hasAttribute("CatLabel"))
                    sets.last().channels.last().catLabel = xml.attributes().value("CatLabel").toString();
            }
            else if (name == "Sensor" && inDatasets) {
                if (xml.attributes().hasAttribute("id"))
                    sets.last().channels.last().sensorId = xml.attributes().value("id").toString();
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
        MatlabFile matlabFile(fi);
        matlabFile.read();
        if (matlabFile.writtenAsMatrix) {
            emit message(QString("-- Файл mat записан матрицей. Он будет пропущен. Экспортируйте файл без галочки \"Group similar blocks in a matrix\""));
            emit tick();
            noErrors = false;
            continue;
        }
        emit message(QString("-- Файл mat содержит %1 каналов").arg(matlabFile.channels.size()));

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
        dfdFileDescriptor.dataDescription = datade;
        dfdFileDescriptor.setSamplesCount(matlabFile.channels.first().size);
        dfdFileDescriptor.BlockSize = 0;
        dfdFileDescriptor.XName="с";
        dfdFileDescriptor.XBegin = matlabFile.channels.first().xStart;
        dfdFileDescriptor.XStep = matlabFile.channels.first().xStep;
        dfdFileDescriptor.DescriptionFormat = "lms2dfd.DF";
        dfdFileDescriptor.CreatedBy = "Конвертер lms2raw by Алексей Новичков";

        //writing raw file channel by channel (blockSize=0)
        QFile rawFile(rawFileName);
        if (rawFile.open(QFile::WriteOnly)) {
            int channelIndex=-1;
            for (int i=0; i<matlabFile.channels.size(); ++i) {
                if (QThread::currentThread()->isInterruptionRequested()) return false;
                //1. search dataset for channel label
                XChannel c;
                for (int j=0; j<set.channels.size(); ++j) {
                    if (set.channels.at(j).catLabel == matlabFile.channels.at(i).label) {
                        c = set.channels.at(j);
                        break;
                    }
                }
                if (c.catLabel.isEmpty()) {
                    emit message("<font color=red>Error!</font> Не могу найти канал "+matlabFile.channels.at(i).label);
                    noErrors = false;
                    continue;
                }

                //2. read source data
                QByteArray rawFromMatfile;
                QFile matFile(fi);
                int itemSize = 4;
                if (matlabFile.channels.at(i).dataType == miDOUBLE) itemSize = 8;
                if (matFile.open(QFile::ReadOnly)) {
                    matFile.seek(matlabFile.channels.at(i).startPos);
                    rawFromMatfile = matFile.read(matlabFile.channels.at(i).size * itemSize);
                }
                else {
                    emit message(QString("<font color=red>Error!</font> Не могу прочитать канал %1").arg(i+1));
                    noErrors = false;
                    continue;
                }

                if (rawFromMatfile.isEmpty() || (rawFromMatfile.size() != int(matlabFile.channels.at(i).size * itemSize))) {
                    emit message(QString("<font color=red>Error!</font> Не могу прочитать канал %1").arg(i+1));
                    noErrors = false;
                    continue;
                }

                // в зависимости от типа данных в массиве жизнь или сильно упрощается, либо совсем наоборот
                switch (matlabFile.channels.at(i).dataType) {
                    case miSINGLE: {//ничего не меняем, записываем сырые данные.
                        rawFile.write(rawFromMatfile);
                        break;
                    }
                    case miDOUBLE: {//ради экономии места записываем в формате single
                        QDataStream stream(rawFromMatfile);
                        stream.setByteOrder(QDataStream::LittleEndian);
                        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

                        QVector<float> data = readBlock<double>(&stream, matlabFile.channels.at(i).size, c.scale);

                        stream.setDevice(&rawFile);
                        stream.setByteOrder(QDataStream::LittleEndian);
                        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
                        for (int k=0; k<data.size();++k) {
                            stream << data[k];
                        }
                        break;
                    }
                    default: {
                        emit message(QString("<font color=red>Error!</font> Неизвестный формат данных в канале %1").arg(i+1));
                        noErrors = false;
                        break;
                    }
                }



                DfdChannel *channel = new DfdChannel(&dfdFileDescriptor, ++channelIndex);
                channel->setName(c.generalName+" -"+c.pointId+"-"+c.direction);
                channel->ChanAddress = QString("SCADAS\\")+c.catLabel;
                c.info.append(channel->ChanAddress);
                channel->setDescription(c.info.join(" \\"));
                channel->IndType = 0xC0000004; //характеристика отсчета
                channel->ChanBlockSize = matlabFile.channels.at(i).size; //размер блока в отсчетах
                channel->YName = c.units;
                channel->InputType="U";

//                channel->BandWidth = c.fd / 2.56;
//                channel->AmplShift = 0.0;
//                channel->AmplLevel = 5.12 / max;
//                channel->Sens0Shift = 0.0;
//                channel->ADC0 = hextodouble("FEF2C98AE17A14C0");
//                channel->ADCStep = hextodouble("BEF8BF05F67A243F");
//                channel->SensSensitivity = hextodouble("000000000000F03F");
//                channel->SensName = c.sensorName+"\\ sn-"+c.sensorSerial;

                dfdFileDescriptor.channels.append(channel);
            }
        }
        dfdFileDescriptor.DataType = CuttedData; // насильно записываем данные как Raw, потому что DeepSea
                                        // не умеет работать с файлами DataType=1, у которых IndType=2
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

