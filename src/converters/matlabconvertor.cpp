#include "matlabconvertor.h"
#include <QtWidgets>

#include "algorithms.h"
#include "fileformats/formatfactory.h"
#include "logging.h"

//QString lms2dsunit(const QString &unit)
//{
//    if (QString("m;cm;mm;nm;km;stat.mi;in;mil;м;см;мм;нм;км").split(";").contains(unit))
//        return "м";

//   if (QString("m/s;nm/s;km/h;nm/s;in/s;mph;m/s(pv);м/с;см/с;мм/с;нм/с;км/с").split(";").contains(unit))
//       return "м/с";

//   if (QString("m/s2;g;m/s?;m/s^2;(m/s^2);м/с^2;м/с2").split(";").contains(unit))
//          return "м/с^2";
//   if (QString("Pa;hPa;kPa;MPa;N/m2;N/mm2;psi;kg/(ms2);bar;Па;гПа;кПа;МПа;Н/м2;Н/м^2;Н/мм2;Н/мм^2").split(";").contains(unit))
//       return "Па";
//   if (QString("N;lbf;kN;daN;Н;кН").split(";").contains(unit))
//       return "Н";
//   if (QString("Nm;kNm;Нм;кНм").split(";").contains(unit))
//       return "Нм";
//   return unit;
//}

#include <QXmlStreamReader>

MatlabConvertor::MatlabConvertor(QObject *parent) : QObject(parent)
{

}

bool MatlabConvertor::convert()
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    if (xmlFileName.isEmpty() || xml.isEmpty()) {
        emit message("<font color=red>Error!</font> Не удалось прочитать файл " + xmlFileName);
        emit finished();
        return false;
    }

    //Converting
    for(const QString &fi: filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("<font color=green>Конвертируем файл " + fi+"</font");
        QString xdfFileName = changeFileExt(fi, "xdf");

        Dataset set;
        QString fileToSearch = QFileInfo(xdfFileName).fileName();
        if (datasets.value(fi, -1) == -1) {
            emit message("<font color=red>Error!</font> Не могу найти "
                         +fileToSearch+" в Analysis.xml. Файл mat будет пропущен!");
            noErrors = false;
            emit tick();
            continue;
        }
        else {
            set = xml.at(datasets.value(fi));
            emit message("-- Файл "+QFileInfo(fileToSearch).fileName()+" в Analysis.xml записан как запись "+set.id);
        }

        //reading mat file structure
        MatFile matlabFile(fi);
        matlabFile.setXml(set);

        matlabFile.read();
        emit message(QString("-- Файл mat содержит %1 переменных").arg(matlabFile.records.size()));

        QString destinationFileName = changeFileExt(fi, destinationFormat);

        //writing file
        QList<QVector<int>> groupedChannelsIndexes = matlabFile.groupChannels();
        if (groupedChannelsIndexes.size() == 1) {
            FileDescriptor *destinationFile = FormatFactory::createDescriptor(matlabFile,
                                                                              destinationFileName);
            if (destinationFile)
                newFiles << destinationFile->fileName();
            else
                noErrors = false;
            delete destinationFile;
        }
        else for (int i=0; i<groupedChannelsIndexes.size(); ++i) {
            QString fname = createUniqueFileName("",
                                                 destinationFileName,
                                                 QString::number(i+1),
                                                 QFileInfo(destinationFileName).suffix(), false);
            FileDescriptor *destinationFile = FormatFactory::createDescriptor(matlabFile,
                                                                              fname,
                                                                              groupedChannelsIndexes.at(i));
            if (!destinationFile) noErrors = false;
            newFiles << destinationFile->fileName();
            delete destinationFile;
        }

        emit converted(fi);

        emit message("Готово.");
        emit tick();
    }
    emit tick();

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();
    return true;
}

void MatlabConvertor::readXml(bool &success)
{
    if (xmlFileName.isEmpty()) {
        success = false;
        emit message("<font color=red>Error!</font> Не могу прочитать файл " + xmlFileName);
        emit finished();
        return;
    }
    xml.clear();
    datasets.clear();
    QFile xmlFile(xmlFileName);
    if (!xmlFile.open(QFile::ReadOnly | QFile::Text)) {
        success = false;
        emit message("<font color=red>Error!</font> Не могу прочитать файл " + xmlFileName);
        emit finished();
        return;
    }
    QXmlStreamReader xmlReader(&xmlFile);
    bool inDatasets = false;
    bool inChannel = false;
    int catLabel = 0;
    while (!xmlReader.atEnd()) {
        xmlReader.readNext();
        if (xmlReader.isStartElement()) {
            QStringRef name=xmlReader.name();
            if (name == "Datasets") inDatasets = true;
            if (name == "Dataset" && inDatasets) {
                Dataset set;
                if (xmlReader.attributes().hasAttribute("Id"))
                    set.id = xmlReader.attributes().value("Id").toString();
                xml << set;
            }
            else if (name == "File" && inDatasets && !inChannel) {
                if (xmlReader.attributes().hasAttribute("Name")) {
                    if (xml.last().fileName.isEmpty()) {
                        xml.last().fileName = xmlReader.attributes().value("Name").toString();
                    }
                }
            }
            else if (name == "Titles" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("Title1")) {
                    xml.last().titles.append(xmlReader.attributes().value("Title1").toString());
                    xml.last().titles.append(xmlReader.attributes().value("Title2").toString());
                    xml.last().titles.append(xmlReader.attributes().value("Title3").toString());
                }
            }
            else if (name == "Date" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("Date"))
                    xml.last().date = xmlReader.attributes().value("Date").toString();
                if (xmlReader.attributes().hasAttribute("Time"))
                    xml.last().time = xmlReader.attributes().value("Time").toString();
            }
            else if (name == "Channel" && inDatasets) {
                inChannel = true;
                XChannel c;
                if (xmlReader.attributes().hasAttribute("name"))
                    c.units = xmlReader.attributes().value("name").toString();
                if (xmlReader.attributes().hasAttribute("logref"))
                    c.logRef = xmlReader.attributes().value("logref").toString().toDouble();
                if (xmlReader.attributes().hasAttribute("logscale"))
                    c.scale = xmlReader.attributes().value("logscale").toString().toDouble();
                xml.last().channels.append(c);
            }
            else if (name == "General" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("Name"))
                    xml.last().channels.last().generalName = xmlReader.attributes().value("Name").toString();
                QString cat = xmlReader.attributes().value("Cat").toString();
                if (xmlReader.attributes().hasAttribute("CatLabel")) {
                    if (cat=="C") {
                        //исходные данные, могут быть разрывы в нумерации,
                        //поэтому запоминаем максимальный номер в переменной catLabel
                        QString calLabelStr = xmlReader.attributes().value("CatLabel").toString();
                        xml.last().channels.last().catLabel = calLabelStr;
                        calLabelStr.remove(0,1);
                        catLabel = calLabelStr.toInt();
                    }
                    else if (cat=="M") {
                        //математические каналы, идут подряд, поэтому меняем catLabel
                        QString calLabelStr = xmlReader.attributes().value("CatLabel").toString();
                        calLabelStr.remove(0,1);
                        int index = calLabelStr.toInt(); //M45 => 45
                        index = index + catLabel; //M45 => C(40+45)=C85
                        xml.last().channels.last().catLabel = QString("C%1").arg(index);
                    }
                }
            }
            else if (name == "Sensor" && inDatasets) {
//                if (xml.attributes().hasAttribute("id"))
//                    sets.last().channels.last().sensorId = xml.attributes().value("id").toString();
                if (xmlReader.attributes().hasAttribute("serial"))
                    xml.last().channels.last().sensorSerial = xmlReader.attributes().value("serial").toString();
                if (xmlReader.attributes().hasAttribute("name"))
                    xml.last().channels.last().sensorName = xmlReader.attributes().value("name").toString();
            }
            else if (name == "Xaxis" && inDatasets) {
                xml.last().channels.last().fd = 1.0 / xmlReader.attributes().value("Delta").toString().toDouble();
            }
            else if (name == "Description" && inDatasets && !xml.isEmpty() && !xml.last().channels.isEmpty()) {
                if (xmlReader.attributes().hasAttribute("ChannelUnit"))
                    xml.last().channels.last().chanUnits = xmlReader.attributes().value("ChannelUnit").toString();
                if (xmlReader.attributes().hasAttribute("PointID"))
                    xml.last().channels.last().pointId = xmlReader.attributes().value("PointID").toString();
                if (xmlReader.attributes().hasAttribute("Direction"))
                    xml.last().channels.last().direction = xmlReader.attributes().value("Direction").toString();
            }
            else if (name == "Information" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("Info1")) {
                    xml.last().channels.last().info << xmlReader.attributes().value("Info1").toString();
                    xml.last().channels.last().info << xmlReader.attributes().value("Info2").toString();
                    xml.last().channels.last().info << xmlReader.attributes().value("Info3").toString();
                    xml.last().channels.last().info << xmlReader.attributes().value("Info4").toString();
                }
            }
            else if (name == "Expression" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("Expression"))
                   xml.last().channels.last().expression = xmlReader.attributes().value("Expression").toString();
            }
            else if (name == "FFT" && inDatasets) {
                if (xmlReader.attributes().hasAttribute("DataType"))
                   xml.last().channels.last().fftDataType = xmlReader.attributes().value("DataType").toInt();
            }
        }
        else if (xmlReader.isEndElement()) {
            if (xmlReader.name() == "Datasets") inDatasets = false;
            else if (xmlReader.name() == "Channel") inChannel = false;
        }
    }
    if (xmlReader.hasError()) {
        emit message(xmlReader.errorString());
    }
}


