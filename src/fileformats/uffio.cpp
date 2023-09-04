#include "uffio.h"

#include "logging.h"
#include "filedescriptor.h"

UffIO::UffIO(const QVector<Channel *> &source, QString fileName, QObject *parent, const QMap<QString, QVariant> &parameters)
    : FileIO(fileName, parent, parameters)
{
    auto other = source.first()->descriptor();

    DataDescription dataDescription(other->dataDescription());
    QDateTime dt = QDateTime::currentDateTime();
    dataDescription.put("fileCreationTime", dt);
    dataDescription.put("guid", FileDescriptor::createGUID());
    dataDescription.put("createdBy", "DeepSea Base");

    if (channelsFromSameFile(source)) {
        dataDescription.put("source.file", other->fileName());
        dataDescription.put("source.guid", other->dataDescription().get("guid"));
        dataDescription.put("source.dateTime", other->dataDescription().get("dateTime"));
        if (other->channelsCount() > source.size()) {
            //только если копируем не все каналы
            dataDescription.put("source.channels", stringify(channelIndexes(source)));
        }

        //ищем силу и номер канала
        for (int i=0; i<source.size(); ++i) {
            Channel *ch = source.at(i);
            if (ch->xName().toLower()=="сила" || ch->xName().toLower()=="sila") {
                referenceChannelNumber = i;
                referenceChannelName = ch->name();
                break;
            }
        }
    }

    //сохраняем файл, попутно подсасывая данные из other
    QFile uff(fileName);
    if (!uff.open(QFile::WriteOnly | QFile::Text)) {
        LOG(ERROR)<<"Couldn't open file"<<fileName<<"to write";
        return;
    }

    QTextStream stream(&uff);
    UffHeader h(dataDescription);
    h.write(stream);
    UffUnits u;
    u.write(stream);
}

void UffIO::addChannel(DataDescription *description, DataHolder *data)
{
    QFile uff(fileName);
    if (!uff.open(QFile::Append | QFile::Text)) {
        LOG(ERROR)<<"Couldn't open file"<<fileName<<"to write";
        return;
    }
    QTextStream stream(&uff);

    int samplesCount = data->samplesCount();


    //заполнение инфы об опорном канале
    if (referenceChannelNumber>=0) {
        description->put("referenceName", referenceChannelName);
        description->put("referenceNode", referenceChannelNumber+1);
    }

    int samples = data->samplesCount();
    if (samplesCount > 0) samples = samplesCount;
    const int blocks = data->blocksCount();

    FunctionHeader head = FunctionHeader::fromDescription(*description);
    FunctionDescription descr = FunctionDescription::fromDescription(*description);

    head.type1858[27].value = samples;
    descr.type58[26].value = samples;
    descr.type58[27].value = data->xValuesFormat()==DataHolder::XValuesUniform ? 1 : 0;
    descr.type58[29].value = data->xStep();

    for (int block = 0; block < blocks; ++block) {
        //writing header
        FunctionHeader h = head;
        h.type1858[4].value = block+1;
        h.write(stream);

        auto t58 = descr.type58;
        t58[10].value = QString("Record %1").arg(block+1);
        t58[15].value = id+block;
        t58[16].value = block+1;
        t58[30].value = data->zValue(block);


        for (int i=0; i<60; ++i) {
            fields[t58[i].type]->print(t58[i].value, stream);
        }

        const int format = data->xValuesFormat();
        switch (t58[25].value.toInt()) {//25 Ordinate Data Type
                                            // 2 - real, single precision
                                            // 4 - real, double precision
                                            // 5 - complex, single precision
                                            // 6 - complex, double precision

            //                                    Data Values
            //                            Ordinate            Abscissa
            //                Case     Type     Precision     Spacing       Format
            //              -------------------------------------------------------------
            //                  1      real      single        even         6E13.5
            //                  2      real      single       uneven        6E13.5
            //                  3     complex    single        even         6E13.5
            //                  4     complex    single       uneven        6E13.5
            //                  5      real      double        even         4E20.12
            //                  6      real      double       uneven     2(E13.5,E20.12)
            //                  7     complex    double        even         4E20.12
            //                  8     complex    double       uneven      E13.5,2E20.12
            //              --------------------------------------------------------------
            case 2: {
                QVector<double> values = data->rawYValues(block);
                int j = 0;

                for (int i=0; i<samples; ++i) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data->xValue(i), stream);
                        j++;
                    }
                    fields[FTFloat13_5]->print(values.at(i), stream);
                    j++;
                    if (j==6) {
                        fields[FTEmpty]->print(0, stream);
                        j=0;
                    }
                }
                if (j!=0) fields[FTEmpty]->print(0, stream);
                break;
            }
            case 4: {
                QVector<double> values = data->rawYValues(block);
                int j = 0;
                for (int i=0; i<samples; ++i) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data->xValue(i), stream);
                        j++;
                    }
                    fields[FTFloat20_12]->print(values.at(i), stream);
                    j++;
                    if (j==4) {
                        fields[FTEmpty]->print(0, stream);
                        j=0;
                    }
                }
                if (j!=0) fields[FTEmpty]->print(0, stream);
                break;
            }
            case 5: {
                auto values = data->yValuesComplex(block);
                int j = 0;
                for (int i=0; i<samples; i++) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data->xValue(i), stream);
                        j++;
                    }
                    fields[FTFloat13_5]->print(values.at(i).real(), stream);
                    j++;
                    fields[FTFloat13_5]->print(values.at(i).imag(), stream);
                    j++;
                    if (j==6) {
                        fields[FTEmpty]->print(0, stream);
                        j=0;
                    }
                }
                if (j!=0) fields[FTEmpty]->print(0, stream);
                break;
            }
            case 6: {
                auto values = data->yValuesComplex(block);
                int j = 0;
                int limit = format == DataHolder::XValuesNonUniform ? 3 : 4;
                for (int i=0; i<samples; i++) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data->xValue(i), stream);
                        j++;
                    }
                    fields[FTFloat20_12]->print(values.at(i).real(), stream);
                    j++;
                    fields[FTFloat20_12]->print(values.at(i).imag(), stream);
                    j++;
                    if (j==limit) {
                        fields[FTEmpty]->print(0, stream);
                        j=0;
                    }
                }
                if (j!=0) fields[FTEmpty]->print(0, stream);
                break;
            }
            default: break;
        }
        fields[FTDelimiter]->print("", stream);
        fields[FTEmpty]->print(0, stream);
    }
    id += blocks;
}

void UffIO::finalize()
{
}
