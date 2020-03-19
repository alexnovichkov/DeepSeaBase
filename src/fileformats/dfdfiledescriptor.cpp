#include "dfdfiledescriptor.h"
#include <QtCore>

#include "converters.h"
#include <QtWidgets>
#include <QUuid>

#include "logging.h"
#include "algorithms.h"
#include "dfdsettings.h"
//#include "psimpl.h"
#include "averaging.h"

QString dataTypeDescription(int type)
{DD;
    switch (type) {
        case 0: return QString("Данные"); break;
        case 1: return QString("Данные1"); break;
        case 2: return QString("Фильтр. данные"); break;
        case 3: return QString("Данные3"); break;
        case 4: return QString("Данные4"); break;
        case 5: return QString("Данные5"); break;
        case 6: return QString("Данные6"); break;
        case 7: return QString("Данные7"); break;
        case 8: return QString("Данные8"); break;
        case 9: return QString("Данные9"); break;
        case 10: return QString("Данные10"); break;
        case 11: return QString("Данные11"); break;
        case 12: return QString("Данные12"); break;
        case 13: return QString("Данные13"); break;
        case 14: return QString("Данные14"); break;
        case 15: return QString("Данные15"); break;
        case 16: return QString("Огибающая"); break;
        case 17: return QString("Мат. ож."); break;
        case 18: return QString("СКЗ"); break;
        case 19: return QString("Ассиметрия"); break;
        case 20: return QString("Эксцесс"); break;
        case 21: return QString("Фаза (Гильб.)"); break;
        case 32: return QString("Корреляция"); break;
        case 33: return QString("X-Корр."); break;
        case 64: return QString("Гистограмма"); break;
        case 65: return QString("ЭФР"); break;
        case 66: return QString("Гистограмма (%)"); break;
        case 67: return QString("Плотн. вероятн."); break;
        case 128: return QString("Спектр"); break;
        case 129: return QString("ПСМ"); break;
        case 130: return QString("Спектр СКЗ"); break;
        case 144: return QString("X-Спектр"); break;
        case 145: return QString("X-Фаза"); break;
        case 146: return QString("Когерентность"); break;
        case 147: return QString("ПФ"); break;
        case 148: return QString("X-Спектр Re"); break;
        case 149: return QString("X-Спектр Im"); break;
        case 150: return QString("ПФ Re"); break;
        case 151: return QString("ПФ Im"); break;
        case 152: return QString("ДН X-Спектр"); break;
        case 153: return QString("Кепстр"); break;
        case 154: return QString("ДН ПФ"); break;
        case 155: return QString("Спектр огиб."); break;
        case 156: return QString("Окт.спектр"); break;
        case 157: return QString("Тр.окт.спектр"); break;
        default: return QString("Неопр.");
    }
    return QString("Неопр.");
}

template <typename T, typename D>
QVector<D> readChunk(QDataStream &readStream, int blockSize, int *actuallyRead)
{
    QVector<D> result(blockSize);
    T v;

    if (actuallyRead) *actuallyRead = 0;

    for (int i=0; i<blockSize; ++i) {
        if (readStream.atEnd()) {
            break;
        }

        readStream >> v;
        result[i] = D(v);
        if (actuallyRead) (*actuallyRead)++;
    }

    return result;
}

template <typename D>
QVector<D> getChunkOfData(QDataStream &readStream, int chunkSize, uint IndType, int *actuallyRead=0)
{
    QVector<D> result;

    switch (IndType) {
        case 0x00000001: {
            result = readChunk<quint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000001: {
            result = readChunk<qint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000002: {
            result = readChunk<quint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000002: {
            result = readChunk<qint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000004: {
            result = readChunk<quint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000004: {
            result = readChunk<qint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000008: {
            result = readChunk<qint64,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000004: {
            result = readChunk<float,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000008: //плавающий 64 бита
        case 0xC000000A: //плавающий 80 бит
            result = readChunk<double,D>(readStream, chunkSize, actuallyRead);
            break;
        default: break;
    }

    return result;
}

//QVector<double> convertFromUINT16(const QByteArray &b, uint IndType)
//{
//    const uchar *ptr = reinterpret_cast<const uchar*>(b.data());
//    int length = b.size();
//    QVector<double> temp(length / (IndType % 16), 0.0);

//    int i=0;
//    while (length) {
//        temp[i++] = static_cast<double>(qFromLittleEndian<quint16>(ptr));
//        length -= (IndType % 16);
//        ptr += (IndType % 16);
//    }
//    return temp;
//}

QVector<double> convertFromUINT16(unsigned char *ptr, qint64 length, uint IndType)
{
    uint step = IndType % 16;
    QVector<double> temp(length / step, 0.0);

    int i=0;
    while (length) {
        temp[i++] = static_cast<double>(qFromLittleEndian<quint16>(ptr));
        length -= step;
        ptr += step;
    }
    return temp;
}

DfdFileDescriptor::DfdFileDescriptor(const QString &fileName)
    : FileDescriptor(fileName),
      DataType(NotDef),
      BlockSize(0),
      NumInd(0),
      XBegin(0.0),
      realXBegin(0.0),
      XStep(0.0),
      source(0),
      process(0),
      dataDescription(0)
{DD;
//    rawFileChanged = false;
    //    changed = false;
}

// creates a copy of DfdDataDescriptor without copying data
DfdFileDescriptor::DfdFileDescriptor(const DfdFileDescriptor &d) : FileDescriptor(d.fileName())
{
    createGUID();
    this->DataType = d.DataType; // см. выше
    this->Date = QDate::currentDate();
    this->Time = QTime::currentTime();
    this->CreatedBy = "DeepSeaBase by Novichkov & sukin sons";

    this->BlockSize = 0; // всегда меняем размер блока новых файлов на 0,
                         // чтобы они записывались без перекрытия
    this->NumInd = d.NumInd;
    this->XName = d.XName;

    this->XBegin = d.XBegin;
    this->XStep = d.XStep;
    this->DescriptionFormat = d.DescriptionFormat;

    source = new Source();
    source->Date = d.Date;
    source->Time = d.Time;
    source->DFDGUID = d.DFDGUID;
    source->File = d.fileName();
    for (int i=1; i<=d.channelsCount(); ++i)
        source->ProcChansList << i;

    if (d.process) {
        process = new Process;
        process->data = d.process->data;
    }
    else process = 0;

    if (d.dataDescription) {
        dataDescription = new DataDescription(this);
        dataDescription->data = d.dataDescription->data;
    }
    else dataDescription = 0;

    foreach (DfdChannel *c, d.channels)
        this->channels << new DfdChannel(*c, this);

    _legend = d._legend;
}

DfdFileDescriptor::DfdFileDescriptor(const FileDescriptor &other) : FileDescriptor(other.fileName())

{
    createGUID();
    this->DataType = dfdDataTypeFromDataType(other.type());

    // меняем тип данных временной реализации на вырезку, чтобы DeepSea не пытался прочитать файл как сырые данные
    if (this->DataType == SourceData) this->DataType = CuttedData;

    this->Date = QDate::currentDate();
    this->Time = QTime::currentTime();
    this->CreatedBy = "DeepSeaBase by Novichkov & sukin sons";

    this->BlockSize = 0; // всегда меняем размер блока новых файлов на 0,
                         // чтобы они записывались без перекрытия
    this->NumInd = other.samplesCount();
    this->XName = other.xName();

    //this->XBegin = // will be filled in fillRest()
    //this->XStep = other.xStep(); // will be filled in fillRest()
    this->DescriptionFormat = "";

    const int count = other.channelsCount();

    source = new Source();
    source->Date=other.dateTime().date();
    source->Time=other.dateTime().time();
    source->DFDGUID = "Unknown";
    source->File = other.fileName();
    for (int i=1; i<=count; ++i)
        source->ProcChansList << i;

    dataDescription = 0;
    process = 0;

    for (int i=0; i<count; ++i) {
        Channel *c= other.channel(i);
        if (!c->populated()) c->populate();
        this->channels << new DfdChannel(*c, this);
    }
    _legend = other.legend();

    fillRest();
}

DfdFileDescriptor::~DfdFileDescriptor()
{DD;
    if (changed())
        write();
    if (dataChanged())
        writeRawFile();

    delete process;
    delete source;
    delete dataDescription;
    qDeleteAll(channels);
}

bool looksLikeOctave(QVector<double> &xvalues, DfdDataType DataType)
{
    if (xvalues.isEmpty() || xvalues.size()==1) return false;

    switch (DataType) {
        case ToSpectr: {
            double m = 0;
            for (int i=1; i<qMin(xvalues.size(),6); ++i) {
                m = std::max(xvalues[i]/xvalues[i-1],m);
                if (xvalues[i]/xvalues[i-1]<1.0) return false;
            }
            if (m<1.3 && m>0) return true;
            break;
        }
        case OSpectr: {
            double m = 0;
            for (int i=1; i<qMin(xvalues.size(),6); ++i) {
                if (xvalues[i]/xvalues[i-1]<1.0) return false;
                m = std::max(xvalues[i]/xvalues[i-1],m);
            }
            if (m<2.1 && m>0) return true;
            break;
        }
        default: break;
    }
    return false;
}

void DfdFileDescriptor::read()
{DD;
    DfdSettings dfd(fileName());
    dfd.read();

    QStringList childGroups = dfd.childGroups();

    //[DataFileDescriptor]
    rawFileName = dfd.value("DataFileDescriptor/DataReference");
    if (rawFileName.isEmpty())
        rawFileName = fileName().left(fileName().length()-4)+".raw";
    DFDGUID =   dfd.value("DataFileDescriptor/DFDGUID");
    DataType =  DfdDataType(dfd.value("DataFileDescriptor/DataType").toInt());
    Date =      QDate::fromString(dfd.value("DataFileDescriptor/Date"),"dd.MM.yyyy");
    if (!Date.isValid()) {
        Date = QDate::fromString(dfd.value("DataFileDescriptor/Date"),"dd.MM.yy");
        Date = Date.addYears(100);
    }
    Time =      QTime::fromString(dfd.value("DataFileDescriptor/Time"),"hh:mm:ss");
    int NumChans =  dfd.value("DataFileDescriptor/NumChans").toInt(); //DebugPrint(NumChans);
    NumInd = dfd.value("DataFileDescriptor/NumInd").toUInt();      //DebugPrint(NumInd);
    BlockSize = dfd.value("DataFileDescriptor/BlockSize").toInt();   //DebugPrint(BlockSize);
    XName = dfd.value("DataFileDescriptor/XName");  //DebugPrint(XName);
    XBegin = hextodouble(dfd.value("DataFileDescriptor/XBegin"));   //DebugPrint(XBegin);
    XStep = hextodouble(dfd.value("DataFileDescriptor/XStep")); //DebugPrint(XStep);
    DescriptionFormat = dfd.value("DataFileDescriptor/DescriptionFormat"); //DebugPrint(DescriptionFormat);
    CreatedBy = dfd.value("DataFileDescriptor/CreatedBy"); //DebugPrint(CreatedBy);

    // [DataDescription]
    if (childGroups.contains("DataDescription")) {
        dataDescription = new DataDescription(this);
        dataDescription->read(dfd);
    }

    // [Source]
    if (childGroups.contains("Source") || childGroups.contains("Sources")) {
        source = new Source;
        source->read(dfd);
    }

    // [Process]
    if (childGroups.contains("Process")) {
        process = new Process();
        process->read(dfd);
    }

    // [Channel#]
    for (int i = 0; i<NumChans; ++i) {
        DfdChannel *ch = newChannel(i);
        ch->read(dfd, NumChans);
    }

    //    if (XStep == 0.0) {//uneven abscissa
    // изменили название оси х с "№№ полос"
    if (DataType == OSpectr || DataType == ToSpectr) {
        XName = "Гц";

        // на один канал меньше
        if (!channels.isEmpty()) {
            // первым каналом записаны центральные частоты, сохраняем их как значения по X
            DfdChannel *firstChannel = channels.first();
            firstChannel->populate();
            QVector<double> xvalues = firstChannel->data()->yValues();
            //определяем, действительно ли это значения фильтров
            if (looksLikeOctave(xvalues, DataType)) {
                xValues = xvalues;
            }
            else {
                xValues.clear();
                if (DataType == ToSpectr) {
                    for (int i=0; i<NumInd; ++i) xValues << pow(10.0, 0.1*(i+1));
                }
                else if (DataType == OSpectr) {
                    for (int i=0; i<NumInd; ++i) xValues << pow(2.0, i+1);
                }
                //xValues изменились, пихаем их в канал
                firstChannel->data()->setXValues(xValues);
            }
            //            // перенумерация каналов
            //            for (int i=0; i<channels.size(); ++i) {
            //                channels[i]->channelIndex = i;
            //            }

            //            delete firstChannel;
        }
        //    }
    }
}

void DfdFileDescriptor::write()
{DD;
    if (!changed()) return;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");

    QFile file(fileName());
    if (!file.open(QFile::WriteOnly | QFile::Text)) return;
    QTextStream dfd(&file);
    dfd.setCodec(codec);

//    if (XStep == 0.0) {
//        // возвращаем старое название оси х
//        XName = "№№ полос";
//    }

    // убираем перекрытие блоков, если пишем не сырые данные
    if (type() != Descriptor::TimeResponse)
        BlockSize = 0;
    else {
        // если временные данные, то убираем перекрытие, только если были изменены данные
        if (dataChanged()) BlockSize = 0;
    }

    /** [DataFileDescriptor]*/
    dfd << "[DataFileDescriptor]" << endl;
    dfd << "DFDGUID="<<DFDGUID << endl;
    dfd << "DataType="<<DataType << endl;
    dfd << "Date="<<Date.toString("dd.MM.yyyy") << endl;
    dfd << "Time="<<Time.toString("hh:mm:ss") << endl;
    dfd << "NumChans="<<channels.size() << endl;
    dfd << "NumInd="<<samplesCount() << endl;
    dfd << "BlockSize="<<BlockSize << endl;
    dfd << "XName="<<xName() << endl;
    dfd << "XBegin="<<doubletohex(XBegin).toUpper() << endl;
    dfd << "XStep="<<doubletohex(xStep()).toUpper() << endl;
    dfd << "DescriptionFormat="<<DescriptionFormat << endl;
    dfd << "CreatedBy="<<CreatedBy << endl;

    /** [DataDescription] */
    if (dataDescription) {
        dataDescription->write(dfd);
    }

    /** [Source] */
    if (source) {
        source->write(dfd);
    }

    /** [Process] */
    if (process) {
        process->write(dfd);
    }

    // сохраняем каналы так, чтобы не пришлось добавлять канал абсцисс к списку каналов.
    // Для этого создаем временный канал
    /** Channels*/
    int b = 0;
//    if (XStep == 0.0) {// uneven abscissa, adding channel
//        if (!channels.isEmpty()) {
//            DfdChannel ch(*(channels.first()));
//            ch.ChanAddress.clear(); //
//            ch.ChanName = "ось X"; //
//            ch.YName="Гц";
//            ch.YNameOld.clear();
//            ch.InputType.clear();
//            ch.ChanDscr.clear();
//            ch.channelIndex = 0; // нумерация с 0

//            ch.write(dfd, 0);
//            b = 1;
//        }
//    }

    for (int i=0; i<channels.size(); ++i)
        channels[i]->write(dfd, i+b);

    setChanged(false);
}

void DfdFileDescriptor::writeRawFile()
{DD;
    if (!dataChanged()) return;

    //be sure all channels were read. May take too much memory
    populate();

    QFile rawFile(rawFileName);
    if (rawFile.open(QFile::WriteOnly)) {
//        //fixing values with correction
//        for (int i = 0; i<channels.size(); ++i) {
//            if (!channels[i]->correct()) {
//                channels[i]->data()->removeCorrection();
//            }
//        }

        QDataStream writeStream(&rawFile);
        writeStream.setByteOrder(QDataStream::LittleEndian);



        if (BlockSize == 0) {
//            // записываем данные нулевого канала, если это третьоктава
//            if (!channels.isEmpty()) {
//                DfdChannel *ch = channels.first();
//                if (ch->xValuesFormat() == DataHolder::XValuesNonUniform) {
//                    if (xValues.isEmpty()) xValues = ch->xValues();

//                    if (ch->IndType==0xC0000004)
//                        writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
//                    for (int val = 0; val < xValues.size(); ++val) {
//                        ch->setValue(xValues[val], writeStream);
//                    }
//                }
//            }

            // пишем поканально
            for (int i = 0; i<channels.size(); ++i) {
                DfdChannel *ch = channels[i];

                if (ch->IndType==0xC0000004)
                    writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                const int sc = ch->samplesCount();
                QVector<double> yValues = ch->data()->rawYValues();
                if (yValues.isEmpty() && !ch->data()->yValuesComplex().isEmpty())
                    yValues = ch->data()->linears();
                for (int val = 0; val < sc; ++val) {
                    ch->setValue(yValues[val], writeStream);
                }
            }
        }
        else {
            // пишем блоками размером BlockSize
            int pos = 0;
            while (pos < samplesCount()) {
                for (int i = 0; i<channels.size(); ++i) {
                    DfdChannel *ch = channels[i];

                    if (ch->IndType==0xC0000004)
                        writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                    QVector<double> yValues = ch->data()->rawYValues();
                    if (yValues.isEmpty() && !ch->data()->yValuesComplex().isEmpty())
                        yValues = ch->data()->linears();

                    //const int sc = ch->samplesCount();
                    for (int val = 0; val < BlockSize; ++val) {
                        if (val+pos >= samplesCount()) break;
                        ch->setValue(yValues[val+pos], writeStream);
                    }
                }
                pos += BlockSize;
            }
        }
    }
    else qDebug()<<"Cannot open file"<<rawFileName<<"to write";
    setDataChanged(false);
}

void DfdFileDescriptor::updateDateTimeGUID()
{DD;
    DFDGUID = DfdFileDescriptor::createGUID();
    Date = QDate::currentDate();
    Time = QTime::currentTime();
    CreatedBy = "DeepSeaBase by Novichkov & sukin sons";
}

void DfdFileDescriptor::fillPreliminary(Descriptor::DataType type)
{DD;
    rawFileName = fileName().left(fileName().length()-4)+".raw";
    updateDateTimeGUID();
    DataType = dfdDataTypeFromDataType(type);

     // time data tweak, so deepseabase doesn't take the file as raw time data
    //так как мы вызываем эту функцию только из новых файлов,
    //все сведения из файлов rawChannel нам не нужны
    if (DataType == SourceData) DataType = CuttedData;
}

void DfdFileDescriptor::fillRest()
{DD;
    if (channels.isEmpty()) return;

    setSamplesCount(channels.first()->samplesCount());
    BlockSize = 0;
//    XName = channels.first()->xName();
    XBegin = channels.first()->xMin();
    XStep = channels.first()->xStep();
}

DfdFileDescriptor *DfdFileDescriptor::newFile(const QString &fileName, DfdDataType type)
{
    if (type == ToSpectr) return newThirdOctaveFile(fileName);
    DfdFileDescriptor *dfd = new DfdFileDescriptor(fileName);
    dfd->DataType = type;
    dfd->rawFileName = fileName.left(fileName.length()-4)+".raw";
    dfd->updateDateTimeGUID();
    return dfd;
}

DfdFileDescriptor *DfdFileDescriptor::newThirdOctaveFile(const QString &fileName)
{
    DfdFileDescriptor *dfd = new DfdFileDescriptor(fileName);

    dfd->fillPreliminary(Descriptor::Unknown);
    dfd->DataType = ToSpectr;

    dfd->BlockSize = 0;
    dfd->XName="Гц";
    dfd->XStep = 0.0;

    dfd->process = new Process();
    dfd->process->data.append({"pName","1/3-октавный спектр"});
    dfd->process->data.append({"pTime","(0000000000000000)"});
    dfd->process->data.append({"TypeProc","1/3-октава"});
    dfd->process->data.append({"Values","измеряемые"});
    dfd->process->data.append({"TypeScale","в децибелах"});

    return dfd;
}

QDateTime DfdFileDescriptor::dateTime() const
{DD;
    return QDateTime(Date, Time)/*.toString("dd.MM.yy hh:mm:ss")*/;
}

Descriptor::DataType DfdFileDescriptor::type() const
{DD;
    return dataTypefromDfdDataType(DataType);
}

QString DfdFileDescriptor::typeDisplay() const
{
    return dataTypeDescription(DataType);
}

DescriptionList DfdFileDescriptor::dataDescriptor() const
{
    if (dataDescription) return dataDescription->data;
    return DescriptionList();
}

void DfdFileDescriptor::setDataDescriptor(const DescriptionList &data)
{
    if (dataDescription) {
        if (dataDescription->data == data) return;
    }
    else
        dataDescription = new DataDescription(this);

    dataDescription->data = data;
    setChanged(true);
    write();
}

void DfdFileDescriptor::setXStep(const double xStep)
{
    if (XStep == xStep) return;


    for (int i=0; i<channels.size(); ++i) {
        // если шаг для отдельного канала не равен шагу всего файла
        if (channels[i]->xStep() != XStep)
            channels[i]->data()->setXStep(xStep * channels[i]->xStep() / XStep);
        else
            channels[i]->data()->setXStep(xStep);
    }
    XStep = xStep;

    setChanged(true);
    write();
}

bool DfdFileDescriptor::setLegend(const QString &legend)
{DD;
    if (legend == _legend) return false;
    _legend = legend;
    if (!dataDescription)
        dataDescription = new DataDescription(this);
    setChanged(true);
    return true;
}

QString DfdFileDescriptor::legend() const
{DD;
    return _legend;
}

void DfdFileDescriptor::setFileName(const QString &name)
{DD;
    FileDescriptor::setFileName(name);
    rawFileName = name.left(name.length()-3)+"raw";
}

bool DfdFileDescriptor::fileExists()
{DD;
    return (FileDescriptor::fileExists() && QFileInfo(rawFileName).exists());
}

void DfdFileDescriptor::setDataChanged(bool changed)
{DD;
    FileDescriptor::setDataChanged(changed);
//    BlockSize = 0;
}

void DfdFileDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    // заполняем вектор индексов каналов, как они будут выглядеть после удаления
    const int count = channelsCount();
    QVector<QPair<int,int> > indexesVector(count);
    for (int i=0; i<count; ++i)
        indexesVector[i] = qMakePair(i, channelsToDelete.contains(i)?1:0); // 0 = keep, 1 = delete

    bool tempFileSuccessful = rewriteRawFile(indexesVector);

    if (!tempFileSuccessful)
        populate();

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    for (int i=0; i<channels.size(); ++i) {
        channels[i]->channelIndex = i;
    }

    setChanged(true);
    write();

    if (!tempFileSuccessful) {
        setDataChanged(true);
        writeRawFile();
    }
}



void DfdFileDescriptor::copyChannelsFrom_plain(FileDescriptor *file, const QVector<int> &indexes)
{DD;
    //заполняем данными файл, куда будем копировать каналы
    //читаем все каналы, чтобы сохранить файл полностью
    populate();

    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file);
    foreach (int index, indexes) {
        bool wasPopulated = file->channel(index)->populated();
        if (!wasPopulated)
            file->channel(index)->populate();
        if (dfd) {
            channels.append(new DfdChannel(*dfd->channels[index], this));
        }
        else {
            channels.append(new DfdChannel(*file->channel(index), this));
        }
        if (!wasPopulated) file->channel(index)->clear();
    }

    XName = file->xName();

    for (int i=0; i<channels.size(); ++i) {
        channels[i]->channelIndex = i;
        channels[i]->parent = this;
    }

    //меняем параметры файла dfd
    Date = QDate::currentDate();
    Time = QTime::currentTime();

    setChanged(true);
    setDataChanged(true);
    write();
    writeRawFile();
}

void DfdFileDescriptor::copyChannelsFrom(FileDescriptor *file, const QVector<int> &indexes)
{DD;
    copyChannelsFrom_plain(file, indexes);
    return;

    /* TODO: переписать весь алгоритм */

    bool rewrited = false;

//     //изначально не знаем формат данных
//    DataHolder::YValuesFormat format = DataHolder::YValuesUnknown;
//    if (channelsCount() > 0) format = channels.first()->yValuesFormat();

//    //перезаписываем файл данных, чтобы не мучаться с блоками отсчетов
//    if (BlockSize > 0) {
//        QVector<QPair<int, int> > inds;
//        for (int i=0; i<channelsCount(); ++i) inds.append(qMakePair<int,int>(i,0)); // 0 = keep, 1 = delete
//        rewrited = rewriteRawFile(inds);
//    }

//    if (BlockSize > 0) {
//        // не удалось переписать файл. Добавляем каналы стандартным способом
//        copyChannelsFrom_plain(file, indexes);
//        return;
//    }


    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file);
    if (dfd) {// работаем с файлом raw
        if (channelsCount() == 0) {// если каналов нет, то просто копируем данные из файла raw
            if (QFile::exists(this->rawFileName)) QFile::remove(this->rawFileName);







            // заполняем информацию о каналах
            for (int index=0; index<dfd->channelsCount(); ++index) {
                if (dfd->DataType == SourceData) {
                    RawChannel *raw = dynamic_cast<RawChannel*>(dfd->channels[index]);
                    if (raw)
                        channels.append(new RawChannel(*raw, this));
                    else
                        channels.append(new DfdChannel(*dfd->channels[index], this));
                }
                else
                    channels.append(new DfdChannel(*dfd->channels[index], this));
            }
            BlockSize = dfd->BlockSize;

            QVector<QPair<int, int> > inds;
            for (int i=0; i<dfd->channelsCount(); ++i)
                if (indexes.contains(i))
                    inds.append(qMakePair<int,int>(i,0)); // 0 = keep, 1 = delete
                else
                    inds.append(qMakePair<int,int>(i,1)); // 0 = keep, 1 = delete
            rewrited = rewriteRawFile(inds);



            XName = file->xName();

            for (int i=0; i<channels.size(); ++i) {
                channels[i]->channelIndex = i;
                channels[i]->parent = this;
            }

            //меняем параметры файла dfd
            Date = QDate::currentDate();
            Time = QTime::currentTime();

            setChanged(true);
            write();
        }
        else {// каналы уже были, копируем обычным образом
            copyChannelsFrom_plain(file, indexes);
            return;
        }
    }
    else {//заглушка: переписать
        copyChannelsFrom_plain(file, indexes);
        return;
        // берем данные из файла uff

        // данные могут быть комплексные, мы должны их изменить
    }

}

void DfdFileDescriptor::calculateMean(const QList<QPair<FileDescriptor *, int> > &channels)
{DD;
    populate();

    DfdChannel *ch = new DfdChannel(this, channelsCount());

    QList<Channel*> list;
    for (int i=0; i<channels.size(); ++i)
        list << channels.at(i).first->channel(channels.at(i).second);
    Channel *firstChannel = list.first();

    //ищем наименьшее число отсчетов
    int numInd = firstChannel->samplesCount();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->samplesCount() < numInd)
            numInd = list.at(i)->samplesCount();
    }

    // ищем формат данных для нового канала
    // если форматы разные, то формат будет линейный (амплитуды), не логарифмированный
    int format = firstChannel->yValuesFormat();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->yValuesFormat() != format) {
            format = DataHolder::YValuesAmplitudes;
            break;
        }
    }

    int units = firstChannel->units();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->units() != units) {
            units = DataHolder::UnitsUnknown;
            break;
        }
    }

    Averaging averaging(Averaging::Linear, list.size());

    foreach (Channel *ch, list) {
        if (ch->yValuesFormat() == DataHolder::YValuesComplex)
            averaging.average(ch->data()->yValuesComplex());
        else
            averaging.average(ch->data()->linears());
    }

    // обновляем сведения канала
    ch->setPopulated(true);
    QStringList l;
    foreach (Channel *c,list) {
        l << c->name();
    }
    ch->ChanName = "Среднее "+l.join(", ");
    l.clear();
    for (int i=0; i<channels.size(); ++i) {
        l << QString::number(channels.at(i).second+1);
    }
    ch->ChanDscr = "Среднее каналов "+l.join(",");

    ch->data()->setThreshold(firstChannel->data()->threshold());
    ch->data()->setYValuesUnits(units);
    if (format == DataHolder::YValuesComplex)
        ch->data()->setYValues(averaging.getComplex().mid(0, numInd));
    else if (format == DataHolder::YValuesAmplitudesInDB) {
        QVector<double> data = averaging.get().mid(0, numInd);
        ch->data()->setYValues(DataHolder::toLog(data, firstChannel->data()->threshold(), units),
                               DataHolder::YValuesFormat(format));
    }
    else
        ch->data()->setYValues(averaging.get().mid(0, numInd), DataHolder::YValuesFormat(format));

    if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        ch->data()->setXValues(firstChannel->xMin(), firstChannel->xStep(), numInd);
    }
    else {
        ch->data()->setXValues(firstChannel->data()->xValues().mid(0, numInd));
    }

    ch->ChanBlockSize = numInd;

    ch->IndType = this->channels.isEmpty()?3221225476:this->channels.first()->IndType;
    ch->YName = firstChannel->yName();
    //грязный хак
    if (DfdChannel *dfd = dynamic_cast<DfdChannel*>(firstChannel)) {
        ch->YNameOld = dfd->YNameOld;
        if (ch->YName == ch->YNameOld && format == DataHolder::YValuesAmplitudesInDB)
            ch->YName = "дБ";
    }
    if (XName.isEmpty()) XName = firstChannel->xName();
    ch->parent = this;
    ch->channelIndex = this->channelsCount();

    this->channels << ch;
}

void DfdFileDescriptor::calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize)
{
    populate();

    for (int i=0; i<channels.size(); ++i) {
        DfdChannel *ch = new DfdChannel(this, channelsCount());
        FileDescriptor *firstDescriptor = channels.at(i).first;
        Channel *firstChannel = firstDescriptor->channel(channels.at(i).second);

        int numInd = firstChannel->samplesCount();

        ch->data()->setThreshold(firstChannel->data()->threshold());
        ch->data()->setYValuesUnits(firstChannel->data()->yValuesUnits());

        int format = firstChannel->data()->yValuesFormat();
        if (format == DataHolder::YValuesComplex) {
            ch->data()->setYValues(movingAverage(firstChannel->data()->yValuesComplex(), windowSize));
        }
        else {
            QVector<double> values = movingAverage(firstChannel->data()->linears(), windowSize);
            if (format == DataHolder::YValuesAmplitudesInDB)
                format = DataHolder::YValuesAmplitudes;
            ch->data()->setYValues(values, DataHolder::YValuesFormat(format));
        }

        if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform)
            ch->data()->setXValues(firstChannel->xMin(), firstChannel->xStep(), numInd);
        else
            ch->data()->setXValues(firstChannel->data()->xValues());

        // обновляем сведения канала
        ch->setPopulated(true);
        ch->ChanName = firstChannel->name()+" сглаж.";

        ch->ChanDscr = "Скользящее среднее канала "+firstChannel->name();
        ch->ChanBlockSize = numInd;

        ch->IndType = this->channels.isEmpty()?3221225476:this->channels.first()->IndType;
        ch->YName = firstChannel->yName();
        //грязный хак
        if (DfdChannel *dfd = dynamic_cast<DfdChannel*>(firstChannel)) {
            ch->YNameOld = dfd->YNameOld;
            if (ch->YName == ch->YNameOld && format == DataHolder::YValuesAmplitudesInDB)
                ch->YName = "дБ";
        }
        if (XName.isEmpty()) XName = firstChannel->xName();
        ch->parent = this;
        ch->channelIndex = this->channelsCount();

        this->channels << ch;
    }
}

QString DfdFileDescriptor::calculateThirdOctave()
{DD;
    populate();

    QString thirdOctaveFileName = createUniqueFileName("", fileName(), "3oct", "dfd", false);

    DfdFileDescriptor *thirdOctDfd = DfdFileDescriptor::newThirdOctaveFile(thirdOctaveFileName);
    thirdOctDfd->DescriptionFormat = this->DescriptionFormat;

    if (this->dataDescription) {
        thirdOctDfd->dataDescription = new DataDescription(thirdOctDfd);
        thirdOctDfd->dataDescription->data = this->dataDescription->data;
    }

    int index=0;
    foreach (DfdChannel *ch, this->channels) {
        DfdChannel *newCh = new DfdChannel(thirdOctDfd,index++);

        auto result = thirdOctave(ch->data()->decibels(), ch->xMin(), ch->xStep());

        newCh->data()->setThreshold(ch->data()->threshold());
        newCh->data()->setYValuesUnits(ch->data()->yValuesUnits());
        newCh->data()->setXValues(result.first);
        newCh->data()->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);


        newCh->YName="дБ";
        newCh->YNameOld=ch->YNameOld;
        newCh->ChanAddress=ch->ChanAddress;
        newCh->ChanName=ch->ChanName;
        newCh->ChanBlockSize=result.first.size();
        newCh->IndType=3221225476;
        //BandWidth=1162346496
        newCh->InputType = ch->InputType;
        newCh->ChanDscr = ch->ChanDscr;

        newCh->setPopulated(true);

        thirdOctDfd->channels.append(newCh);

    }

    thirdOctDfd->xValues = thirdOctDfd->channels.last()->xValues();

//     добавляем нулевой канал с осью Х
    DfdChannel *ch = new DfdChannel(thirdOctDfd,index++);
    ch->ChanAddress.clear(); //
    ch->ChanName = "ось X"; //
    ch->YName="Гц";
    ch->YNameOld.clear();
    ch->InputType.clear();
    ch->ChanDscr.clear();
    ch->channelIndex = 0; // нумерация с 0
    ch->ChanBlockSize=thirdOctDfd->xValues.size();
    ch->IndType=3221225476;
    ch->data()->setThreshold(1.0);
    ch->data()->setXValues(thirdOctDfd->xValues);
    ch->data()->setYValues(thirdOctDfd->xValues, DataHolder::YValuesReals);
    ch->setPopulated(true);

    thirdOctDfd->channels.prepend(ch);

    for (int i=0; i<thirdOctDfd->channels.size(); ++i)
        thirdOctDfd->channels[i]->channelIndex = i;


    thirdOctDfd->XBegin = thirdOctDfd->xValues.first();
    thirdOctDfd->setSamplesCount(thirdOctDfd->channels.last()->data()->samplesCount());

    thirdOctDfd->setChanged(true);
    thirdOctDfd->setDataChanged(true);
    thirdOctDfd->write();
    thirdOctDfd->writeRawFile();
    delete thirdOctDfd;
    return thirdOctaveFileName;
}

bool DfdFileDescriptor::rewriteRawFile(const QVector<QPair<int,int> > &indexesVector)
{
    if (indexesVector.isEmpty()) return false;

    // переписываем файл данных во временный файл, которым затем подменим исходный raw
    QTemporaryFile tempFile;
    QFile rawFile(rawFileName);

    if (tempFile.open() && rawFile.open(QFile::ReadOnly)) {
        qDebug()<<"работаем через временный файл";
        //работаем через временный файл - читаем один канал и записываем его во временный файл
        if (!channels[0]->dataPositions.isEmpty()) {
            qDebug()<<"- используем dataPositions";
            for (int ind = 0; ind < indexesVector.size(); ++ind) {

                // пропускаем канал, предназначенный для удаления
                if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

                DfdChannel *ch = channels[indexesVector.at(ind).first];
                qint64 dataPos = tempFile.pos();
                //пишем канал целиком
                foreach (int pos, ch->dataPositions) {
                    rawFile.seek(pos);
                    QByteArray b = rawFile.read(ch->blockSizeInBytes());
                    tempFile.write(b);
                }
                tempFile.flush();

                // меняем размер блока и обновляем положение данных
                ch->dataPositions.clear();
                ch->dataPositions.append(dataPos);
                ch->ChanBlockSize = this->NumInd;
                this->BlockSize = 0;
                qDebug()<<"-- меняем BlockSize на 0";
            }
        }
        else {
            qDebug()<<"- dataPositions отсутствуют";
            if (BlockSize == 1) {
                qDebug()<<"-- BlockSize == 1";
                // trying to map file into memory
                unsigned char *ptr = rawFile.map(0, rawFile.size());
                if (ptr) {//достаточно памяти отобразить весь файл
                    qDebug()<<"-- работаем через ptr";
                    qDebug()<<"-- меняем BlockSize на 0";
                    unsigned char *ptrCurrent = ptr;
                    for (int ind = 0; ind < indexesVector.size(); ++ind) {
                        // пропускаем канал, предназначенный для удаления
                        if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

                        DfdChannel *ch = channels[indexesVector.at(ind).first];
                        qint64 dataPos = tempFile.pos();
                        int bytes = ch->IndType % 16;
                        for (int i=0; i<this->NumInd; ++i) {
                            ptrCurrent = ptr + (indexesVector.at(ind).first + i*channelsCount()) * bytes;
                            tempFile.write((char*)(ptrCurrent), bytes);
                        }
                        tempFile.flush();

                        // меняем размер блока и обновляем положение данных
                        ch->dataPositions.clear();
                        ch->dataPositions.append(dataPos);
                        ch->ChanBlockSize = this->NumInd;
                        this->BlockSize = 0;
                    }

                    rawFile.unmap(ptr);
                }
                else {
                    qDebug()<<"-- не меняем BlockSize";
                    // не удалось прочитать никакими оптимальными методами, читаем медленным классическим
                    int actuallyRead = 0;
                    const int chunkSizeBytes = channelsCount() * channels[0]->blockSizeInBytes();

                    while (1) {
                        QByteArray temp = rawFile.read(chunkSizeBytes);
                        actuallyRead = temp.size();
                        if (actuallyRead == 0)
                            break;
                        if (actuallyRead < chunkSizeBytes) {
                            temp.resize(chunkSizeBytes);
                            actuallyRead = 0;
                        }

                        for (int ind = 0; ind < indexesVector.size(); ++ind) {
                            // пропускаем канал, предназначенный для удаления
                            if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

                            tempFile.write(temp.mid(indexesVector.at(ind).first * channels[indexesVector.at(ind).first]->blockSizeInBytes(),
                                           channels[indexesVector.at(ind).first]->blockSizeInBytes()));
                        }

                        if (actuallyRead == 0)
                            break;
                    }
                }
            }
            else {
                qDebug()<<"-- не меняем BlockSize";
                // не удалось прочитать никакими оптимальными методами, читаем медленным классическим
                int actuallyRead = 0;
                const int chunkSizeBytes = channelsCount() * channels[0]->blockSizeInBytes();

                while (1) {
                    QByteArray temp = rawFile.read(chunkSizeBytes);
                    actuallyRead = temp.size();
                    if (actuallyRead == 0)
                        break;
                    if (actuallyRead < chunkSizeBytes) {
                        temp.resize(chunkSizeBytes);
                        actuallyRead = 0;
                    }

                    for (int ind = 0; ind < indexesVector.size(); ++ind) {
                        // пропускаем канал, предназначенный для удаления
                        if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

                        tempFile.write(temp.mid(indexesVector.at(ind).first * channels[indexesVector.at(ind).first]->blockSizeInBytes(),
                                       channels[indexesVector.at(ind).first]->blockSizeInBytes()));
                    }

                    if (actuallyRead == 0)
                        break;
                }
            }
        }

        //подменяем исходный файл raw временным
        rawFile.close();
        tempFile.close();
        if (rawFile.remove()) {
            tempFile.copy(rawFileName);
            return true;
        }
        else {
            qDebug()<<" Couldn't replace raw file with temp file.";
            return false;
        }
    }
    return false;
}

void DfdFileDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    if (indexes.isEmpty() || newIndexes.isEmpty()) return;

    // заполняем вектор индексов каналов, как они будут выглядеть после перемещения
    const int count = channelsCount();
    QVector<QPair<int,int> > indexesVector(count);
    for (int i=0; i<count; ++i)
        indexesVector[i] = qMakePair(i, 0); // 0 = keep, 1 = delete

    {int i=up?0:indexes.size()-1;
    while (1) {
        indexesVector.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }}

    bool tempFileSuccessful = rewriteRawFile(indexesVector);

    if (!tempFileSuccessful) populate();

    int i=up?0:indexes.size()-1;
    while (1) {
        channels.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }
    for (int i=0; i<channels.size(); ++i)
        channels[i]->channelIndex = i;

    setChanged(true);
    write();

    if (!tempFileSuccessful) {
        setDataChanged(true);
        writeRawFile();
    }

    qDebug()<<"done";
}

QVariant DfdFileDescriptor::channelHeader(int column) const
{
    if (channels.isEmpty()) return QVariant();
    return channels[0]->channelHeader(column);
}

int DfdFileDescriptor::columnsCount() const
{
    if (channels.isEmpty()) return 7;
    return channels[0]->columnsCount();
}

Channel *DfdFileDescriptor::channel(int index) const
{
    if (channels.size()>index) return channels[index];
    return 0;
}

bool DfdFileDescriptor::isSourceFile() const
{DD;
    return (DataType>=SourceData && DataType<=15);
}

bool DfdFileDescriptor::dataTypeEquals(FileDescriptor *other) const
{DD;
    DfdFileDescriptor *d = dynamic_cast<DfdFileDescriptor *>(other);
    if (!d) return false;
    return (this->DataType == d->DataType);
}

QString DfdFileDescriptor::fileFilters() const
{DD;
    return QString("Файлы dfd (*.dfd)");
}

void DfdFileDescriptor::setDateTime(QDateTime dt)
{
    this->Date = dt.date();
//    Date = Date.addYears(100);
    this->Time = dt.time();
    setChanged(true);
    write();
}

DfdChannel *DfdFileDescriptor::newChannel(int chanIndex)
{DD;
    DfdChannel *c = 0;
    switch (DataType) {
        case SourceData: c = new RawChannel(this, chanIndex); break;
        default: c = new DfdChannel(this, chanIndex); break;
    }
    channels << c;
    return c;
}

QString DfdFileDescriptor::dataDescriptorAsString() const
{DD;
    if (dataDescription) return dataDescription->toString();
    return "";
}

QString DfdFileDescriptor::createGUID()
{DD;
    QString result = QUuid::createUuid().toString().toUpper();
    if (result.at(24) == '-') result.remove(24,1);
    else result.remove(25,1);
    return result;
}

Process::Process()
{DD;

}

void Process::read(DfdSettings &dfd)
{DD;
    data = dfd.values("Process");
}

void Process::write(QTextStream &dfd)
{DD;
    dfd << "[Process]" << endl;
    foreach (const DescriptionEntry &entry, data) {
        dfd << entry.first << "=" << entry.second << endl;
    }
}

QString Process::value(const QString &key)
{DD;
    for (int i = 0; i<data.size(); ++i) {
        QPair<QString, QString> item = data.at(i);
        if (item.first == key) return item.second;
    }
    return QString();
}


void Source::read(DfdSettings &dfd)
{DD;
    sFile = dfd.value("Sources/sFile");
    if (!sFile.isEmpty()) {
        //K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
        File = sFile.section("[",0,0);
        QStringList procChansList = sFile.section("[",1).section("]",0,0).split(",");
        foreach (QString s, procChansList) ProcChansList << s.toInt();
        DFDGUID = "{"+sFile.section("{",1);
    }
    else {
        File = dfd.value("Source/File");
        DFDGUID = dfd.value("Source/DFDGUID");
        Date=QDate::fromString(dfd.value("Source/Date"),"dd.MM.yyyy");
        Time=QTime::fromString(dfd.value("Source/Time"),"hh:mm:ss");
    }
}

void Source::write(QTextStream &dfd)
{DD;
    if (!sFile.isEmpty()) {
        dfd << "[Sources]" << endl;
        dfd << "sFile=" << sFile << endl;
    }
    else {
        dfd << "[Source]" << endl;
        dfd << "File=" << File << endl;
        dfd << "DFDGUID=" << DFDGUID << endl;
        dfd << "Date=" << Date.toString("dd.MM.yyyy") << endl;
        dfd << "Time=" << Time.toString("hh:mm:ss") << endl;
    }
}

DfdChannel::DfdChannel(DfdFileDescriptor *parent, int channelIndex)
    : Channel(), IndType(0),
      ChanBlockSize(0),
      parent(parent),
      channelIndex(channelIndex),
      _populated(false)
{
    dataType = parent->DataType;
}

DfdChannel::DfdChannel(DfdChannel &other, DfdFileDescriptor *parent) : Channel(other)
{DD;
    // data is copied in the Channel construction

    if (parent)
        this->parent = parent;
    ChanAddress = other.ChanAddress; //
    ChanName = other.ChanName;
    IndType = other.IndType;
    ChanBlockSize = other.ChanBlockSize; //размер блока в отсчетах
    YName = other.YName;
    YNameOld = other.YNameOld;
    InputType = other.InputType;
    ChanDscr = other.ChanDscr;

    channelIndex = other.channelIndex;

    _populated = other._populated;
    dataType = other.dataType;
}

DfdChannel::DfdChannel(Channel &other, DfdFileDescriptor *parent) : Channel(other)
{DD;
    // data is copied in the Channel construction

    if (parent)
        this->parent = parent;
    ChanAddress = "";
    ChanName = other.name();
    ChanDscr = other.description();
    IndType = 3221225476;
    ChanBlockSize = other.samplesCount();

    if (DfdChannel *o=dynamic_cast<DfdChannel*>(&other)) {
        YName = o->YName;
        YNameOld = o->YNameOld;
    }
    else {
        YName = other.yName();
        YNameOld = other.yName();
        if (other.data()->yValuesFormat() == DataHolder::YValuesAmplitudesInDB) {
            YName = "дБ";
        }
    }

    _populated = true;
    dataType = dfdDataTypeFromDataType(other.type());
}

DfdChannel::~DfdChannel()
{
    //delete [] YValues;
}

void DfdChannel::read(DfdSettings &dfd, int numChans)
{DD;
    QString group = QString("Channel%1/").arg(channelIndex+1);

    ChanAddress = dfd.value(group+"ChanAddress");
    ChanName = dfd.value(group+"ChanName");
    IndType = dfd.value(group+"IndType").toUInt();
    ChanBlockSize = dfd.value(group+"ChanBlockSize").toInt();

    YName = dfd.value(group+"YName");
    YNameOld = dfd.value(group+"YNameOld");
    InputType = dfd.value(group+"InputType");
    ChanDscr = dfd.value(group+"ChanDscr");
    setCorrection(dfd.value(group+"Correction"));

    int NumInd;
    if (parent->BlockSize==0)
        NumInd = ChanBlockSize;
    else {
        NumInd = ChanBlockSize/parent->BlockSize;
        NumInd *= parent->samplesCount();
    }
    double XStep = double(NumInd) / double(parent->samplesCount());
    XStep *= parent->xStep();

    if (XStep == 0) {// abscissa, uneven spacing
        _data->setXValues(QVector<double>());
        _data->setSamplesCount(NumInd);
    }
    else {// abscissa, even spacing
        _data->setXValues(0.0, XStep, NumInd);
    }

    int yValueFormat = dataFormat();

    if (YName.toLower()=="db" || YName.toLower()=="дб")
        yValueFormat = DataHolder::YValuesAmplitudesInDB;
    //настройка для отдельных файлов
    if ((parent->DataType == OSpectr || parent->DataType == ToSpectr)
        && YName.toLower() != "db" && YName.toLower() != "дб")
        yValueFormat = DataHolder::YValuesReals;

    _data->setYValuesFormat(yValueFormat);

    // читаем позиции данных этого канала
    dataPositions.clear();
    if (parent->BlockSize == 0) {//без перекрытия, один блок данных
        dataPositions << channelIndex * blockSizeInBytes();
    }
    else {
        if (ChanBlockSize == 1) return; //не запоминаем положения блоков, если блок равен 1
        int i = 0; // номер блока
        qint64 rawSize = QFile(parent->rawFileName).size(); //DebugPrint(rawSize);
        while (1) {
            qint64 pos = (parent->BlockSize * numChans * i
                         + channelIndex * ChanBlockSize) * (IndType % 16);
            if (pos >= rawSize) break;
            dataPositions << pos;
            i++;
        }
    }
}

void DfdChannel::write(QTextStream &dfd, int index)
{DD;
    dfd << QString("[Channel%1]").arg(index == -1 ? channelIndex+1 : index+1) << endl;
    dfd << "ChanAddress=" << ChanAddress << endl;
//    QString correctedName = ChanName;
//    if (data()->hasCorrection()) correctedName.append(data()->correctionString());
    dfd << "ChanName=" << ChanName << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << ChanBlockSize << endl;
    dfd << "YName=" << YName << endl;
    if (!YNameOld.isEmpty())
        dfd << "YNameOld=" << YNameOld << endl;
    dfd << "InputType="<<InputType << endl;
    dfd << "ChanDscr="<<ChanDscr << endl;
    dfd << "Correction="<<correction() << endl;
}

QVariant DfdChannel::info(int column) const
{
    switch (column) {
        case 0: return ChanName; //name(); //avoiding conversion variant->string->variant
        case 1: {
            QString result = YName;
            if (!YNameOld.isEmpty()) result.append(QString(" (%1)").arg(YNameOld));
            return result;
        }
        case 2: return data()->yValuesFormatString();
        case 3: return ChanDscr;
        case 4: return m_correction; //correction();
        default: ;
    }
    return QVariant();
}

int DfdChannel::columnsCount() const
{
    return 5;
}

QVariant DfdChannel::channelHeader(int column) const
{
    switch (column) {
        case 0: return QString("Имя");
        case 1: return QString("Ед.изм.");
        case 2: return QString("Формат");
        case 3: return QString("Описание");
        case 4: return QString("Коррекция");
        default: return QVariant();
    }
    return QVariant();
}

void DfdChannel::populate()
{DD;
    // clear previous data;
    _data->clear();

    double thr = 1.0;
    if (YName.isEmpty() || YName.toLower() == "дб" || YName.toLower() == "db")
        thr = threshold(YNameOld);
    else
        thr = threshold(YName);
    if (type()==Descriptor::FrequencyResponseFunction) thr=1.0;
    _data->setThreshold(thr);

    int units = DataHolder::UnitsLinear;
    if (dataType == Spectr || dataType == XSpectr) units = DataHolder::UnitsQuadratic;
    _data->setYValuesUnits(units);

//    int yValueFormat = dataFormat();

//    if (YName.toLower()=="db" || YName.toLower()=="дб")
//        yValueFormat = DataHolder::YValuesAmplitudesInDB;

    QFile rawFile(parent->attachedFileName());

//    QTime time;
//    time.start();

    if (rawFile.open(QFile::ReadOnly)) {
        /* Сложная система оптимизаций чтения.
         * Самый распространенный формат временных реализаций - IndType=2 (quint16)
         * Или IndType = 3221225476 (32-bit float)
         * Методы чтения идут в порядке убывания оптимальности
         * - самый быстрый - когда мы знаем положение всех блоков данных
         *   и отображаем файл в память.
         * - медленнее - читать из файла в память и интерпретировать каждый отсчет
         *
         */

        QVector<double> YValues;
        const quint64 blockSizeBytes = blockSizeInBytes();
        const int channelsCount = parent->channelsCount();

        if (IndType == 2) {//целое без знака, quint16. Можем читать как угодно
            if (!dataPositions.isEmpty()) {
                // уже знаем позиции всех данных
                // читаем или через map, или оптимизированным способом

                // trying to map file into memory
                unsigned char *ptr = rawFile.map(0, rawFile.size());
                if (ptr) {//достаточно памяти отобразить весь файл
//                    qDebug()<<"IndType=2, mapped, by dataPositions";
                    unsigned char *maxPtr = ptr + rawFile.size();
                    unsigned char *ptrCurrent = ptr;

                    foreach (int pos, dataPositions) {
                        ptrCurrent = ptr + pos;
                        QVector<double> temp = convertFromUINT16(ptrCurrent, qMin(maxPtr-ptrCurrent, int(blockSizeBytes)), IndType);
                        YValues << temp;
                    }
                }
                else {//оптимизированно через qToLittleEndian
//                    qDebug()<<"IndType=2, optimised, by dataPositions";
                    foreach (int pos, dataPositions) {
                        rawFile.seek(pos);
                        QVector<double> temp;

                        QByteArray b = rawFile.read(blockSizeBytes);
                        uchar *ptr = reinterpret_cast<uchar*>(b.data());
                        int length = b.size();
                        temp = convertFromUINT16(ptr, length, IndType);

                        YValues << temp;
                    }
                }
            }
            else if (ChanBlockSize != 1) {//блоки достаточно длинные, чтобы мы могли читать в вектор
                // Этот блок кода никогда не будет вызван,
                // так как dataPositions заполняются всегда, если ChanBlockSize != 1

                if (parent->BlockSize == 0) {// без перекрытия, читаем подряд весь канал
//                    qDebug()<<"IndType=2, blockSize=0, skipping";
                    // сложная схема пропуска данных на тот случай, если в каналах разное число отсчетов
                    for (int i=0; i<channelsCount; ++i) {
                        if (i==channelIndex) {
                            QByteArray b = rawFile.read(blockSizeBytes);
                            uchar *ptr = reinterpret_cast<uchar*>(b.data());
                            int length = b.size();
                            YValues = convertFromUINT16(ptr, length, IndType);
                        }
                        else {
                            rawFile.skip(parent->dfdChannel(i)->blockSizeInBytes());
                        }
                    }
                }
                else {// с перекрытием, читаем классическим способом
//                    qDebug()<<"IndType=2, с перекрытием, skipping";
                    //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
                    // если каналы имеют разный размер блоков, этот метод даст сбой
                    int actuallyRead = 0;
                    const int chunkSize = channelsCount * ChanBlockSize;

                    QDataStream readStream(&rawFile);
                    readStream.setByteOrder(QDataStream::LittleEndian);

                    while (1) {
                        /*
                        * i-й отсчет n-го канала имеет номер
                        * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                        */
                        QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                        //распихиваем данные по каналам
                        actuallyRead /= channelsCount;
                        YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                        if (actuallyRead < ChanBlockSize)
                            break;
                    }
                }
            }
            else {//ChanBlockSize == 1
                // читаем через map, по-отсчетно

                // trying to map file into memory
                unsigned char *ptr = rawFile.map(0, rawFile.size());
                if (ptr) {//достаточно памяти отобразить весь файл
//                    qDebug()<<"IndType=2, mapped, ChanBlockSize=1";
                    //unsigned char *maxPtr = ptr + rawFile.size();
                    unsigned char *ptrCurrent = ptr;

                    YValues.resize(qMin(parent->NumInd, int(rawFile.size() / channelsCount / (IndType % 16))));
                    for (int i=0; i < YValues.size(); ++i) {
                        //
                        // i-й отсчет n-го канала имеет номер
                        // n + i*ChannelsCount
                        //
                        ptrCurrent = ptr + (channelIndex + i*channelsCount) * (IndType % 16);
                        YValues[i] = static_cast<double>(qFromLittleEndian<quint16>(ptrCurrent));
                    }
                }
                else {// не удалось прочитать никакими оптимальными методами, читаем медленным классическим
//                    qDebug()<<"IndType=2, by chunks, ChanBlockSize=1";
                    int actuallyRead = 0;
                    const int chunkSize = channelsCount; //ChanBlockSize = 1

                    QDataStream readStream(&rawFile);
                    readStream.setByteOrder(QDataStream::LittleEndian);

                    while (1) {
                        /*
                        * i-й отсчет n-го канала имеет номер
                        * n + i*ChannelsCount
                        */
                        QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                        //распихиваем данные по каналам
                        actuallyRead /= chunkSize;
                        YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                        if (actuallyRead < ChanBlockSize)
                            break;
                    }
                }
            }
        }
        else /*if (IndType == 3221225476) {//тип float, LittleEndian
            if (!dataPositions.isEmpty()) {
                // уже знаем позиции всех данных
                // читаем оптимизированным способом через QByteArray и QDataStream
                qDebug()<<"IndType=3221225476, optimised, by dataPositions";
                foreach (int pos, dataPositions) {
                    rawFile.seek(pos);
                    QVector<double> temp;

                    QByteArray b = rawFile.read(blockSizeBytes);

                    //добавляем к вектору b число элементов
                    QByteArray bLength(4,'0');
                    uchar *ptr = reinterpret_cast<uchar*>(bLength.data());
                    qToLittleEndian<quint32>(quint32(b.size() / (IndType % 16)), ptr);

                    b.prepend(bLength);

                    //читаем из вектора b величины float с помощью QDataStream
                    QDataStream stream(b);
                    stream.setByteOrder(QDataStream::LittleEndian);
                    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
                    stream >> temp;

                    YValues << temp;
                }
            }
            else {
                //читаем классическим способом через getChunk
                qDebug()<<"IndType=3221225476, not optimised, skipping";

                QDataStream readStream(&rawFile);
                readStream.setByteOrder(QDataStream::LittleEndian);
                if (IndType==0xC0000004)
                    readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
                // если каналы имеют разный размер блоков, этот метод даст сбой
                int actuallyRead = 0;
                const int chunkSize = channelsCount * ChanBlockSize;
                while (1) {
                    //
                    // i-й отсчет n-го канала имеет номер
                    // n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                    //
                    QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                    //распихиваем данные по каналам
                    actuallyRead /= channelsCount;
                    YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                    if (actuallyRead < ChanBlockSize)
                        break;
                }
            }
        }
        else*/ {//все остальные форматы,
            //читаем классическим способом через getChunk
            QDataStream readStream(&rawFile);
            readStream.setByteOrder(QDataStream::LittleEndian);
            if (IndType==0xC0000004)
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
            // если каналы имеют разный размер блоков, этот метод даст сбой
            int actuallyRead = 0;
            const int chunkSize = channelsCount * ChanBlockSize;
            while (1) {
                /*
                * i-й отсчет n-го канала имеет номер
                * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                */
                QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                //распихиваем данные по каналам
                actuallyRead /= channelsCount;
                YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                if (actuallyRead < ChanBlockSize)
                    break;
            }
        }

        YValues.resize(parent->NumInd);
        postprocess(YValues);
        _data->setYValues(YValues, DataHolder::YValuesFormat(_data->yValuesFormat()));
        setPopulated(true);

        if (parent->DataType == ToSpectr || parent->DataType == OSpectr) {//данные по оси Х хранятся первым каналом
            if (!parent->xValues.isEmpty()) {
                _data->setXValues(parent->xValues);
            }
            else {
                QDataStream readStream(&rawFile);
                readStream.setByteOrder(QDataStream::LittleEndian);
                if (IndType==0xC0000004)
                    readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
                rawFile.seek(0);
                readStream.setDevice(&rawFile);
                // читаем без перекрытия, предполагаем, что тип файла - третьоктава или октава
                QVector<double> temp = getChunkOfData<double>(readStream, ChanBlockSize, IndType);
                //checking if values are really frequency values, take four first values
                parent->xValues = temp;
                _data->setXValues(temp);
                //                if (temp.size()>=4) {
                //                    QVector<double> xv(4);
                //                    for (int k = 0; k<4; ++k)
                //                        xv[k] = pow(10.0, 0.1*k);
                //                    if (qAbs(temp[0]-xv[0])<1e-4
                //                        && qAbs(temp[1]-xv[1])<1e-4
                //                        && qAbs(temp[2]-xv[2])<1e-4
                //                        && qAbs(temp[3]-xv[3])<1e-4) {
                //                        setXValues(temp);
                //                    }
                //                    else if (parent->DataType == ToSpectr) {
                //                        QVector<double> xv(YValues.size());
                //                        for (int k = 0; k<YValues.size(); ++k)
                //                            xv[k] = pow(10.0, 0.1*k);
                //                        setXValues(xv);
                //                    }
                //                }
            }
        }
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->attachedFileName();
    }
//    qDebug()<<"reading finished"<<time.elapsed();
}

void DfdChannel::populateFloat()
{DD;
    int NI = samplesCount();

    if (floatValues.size() == NI) return;
    floatValues.clear();

    const int count = parent->channelsCount();

    QFile rawFile(parent->attachedFileName());

    if (rawFile.open(QFile::ReadOnly)) {
        QDataStream readStream(&rawFile);
        readStream.setByteOrder(QDataStream::LittleEndian);
        if (IndType==0xC0000004)
            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        int actuallyRead = 0;

        int chunkSize = ChanBlockSize * count;

        while (1) {
            if (QThread::currentThread()->isInterruptionRequested()) return;

            QVector<float> temp = getChunkOfData<float>(readStream, chunkSize, IndType, &actuallyRead);

            //распихиваем данные по каналам
            actuallyRead /= count;
            for (int i=0; i<count;++i) {
                if (i == channelIndex) {
                    floatValues << temp.mid(actuallyRead*i, actuallyRead);
                    break;
                }
            }
            if (actuallyRead < ChanBlockSize) {
                break;
            }
        }

        if (RawChannel *rawc = dynamic_cast<RawChannel*>(this)) {
            for (int k=0; k < floatValues.size(); ++k)
                floatValues[k] = floatValues[k]*rawc->coef1+rawc->coef2;
        }
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->attachedFileName();
    }
}

quint64 DfdChannel::blockSizeInBytes() const
{
    return quint64(ChanBlockSize * (IndType % 16));
}

FileDescriptor *DfdChannel::descriptor()
{
    return parent;
}

int DfdChannel::dataFormat() const
{
    switch (parent->DataType) {
        case SourceData:		// исходные данные
        case CuttedData:		// вырезка из исходных данных
        case FilterData:		// фильтрованные данные
            return DataHolder::YValuesReals;
        case Envelope:		// огибающая по Гильберту
            return DataHolder::YValuesAmplitudes;
        case PassAvrg:		// проходная - мат. ожидание
        case PassDev:		// проходная - СКЗ
        case PassAss:		// проходная - асимметрия
        case PassExc:		// проходная - эксцесс
            return DataHolder::YValuesReals;
        case GPhaPhase: 	// фаза Гильберта
            return DataHolder::YValuesPhases;
        case TrFltP: 		// следящий фильтр - мощность
            return DataHolder::YValuesAmplitudes;
        case TrFltF:		// следящий фильтр - частота
            return DataHolder::YValuesReals;
        case AutoCorr:		// автокорреляция
        case CrossCorr:		// взаимная корреляция
            return DataHolder::YValuesReals;
        //Histogram  =  64,		// гистограмма
        //EDF          =  65,		// эмпирическая функция распределения
        //Hist100    	=  66,		// гистограмма %
        //HistP      	=  67,		// плотность вероятности
        case Spectr:		// спектр мощности
        case SpcDens:		// плотность спектра мощности
        case SpcDev:		// спектр СКЗ
        case XSpectr:		// взаимный спектр
            return DataHolder::YValuesAmplitudes;
        case XPhase:		// взаимная фаза
            return DataHolder::YValuesPhases;
        case Coherence:	// когерентность
        case TransFunc:	// передаточная функция
            return DataHolder::YValuesAmplitudes;
        case XSpectrRe:	// действ. часть взаимного спектра
            return DataHolder::YValuesReals;
        case XSpectrIm:		// мнимая часть взаимного спектра
            return DataHolder::YValuesImags;
        case TrFuncRe:	// действ. часть передаточной функции
            return DataHolder::YValuesReals;
        case TrFuncIm:		// мнимая часть передаточной функции
            return DataHolder::YValuesImags;
        //DiNike     = 152,		// диаграмма Найквиста для взаимных спектров
        case Cepstr:		// кепстр
            return DataHolder::YValuesAmplitudes;
        //DiNikeP    = 154,       // диаграмма Найквиста для передаточной функции
        //GSpectr    = 155,		// спектр Гильберта
        case OSpectr:		// октавный спектр
        case ToSpectr:		// 1/3-октавный спектр
            return DataHolder::YValuesAmplitudesInDB;
        default:
            break;
    }
    return DataHolder::YValuesReals;
}

QString DfdChannel::legendName() const
{DD;
    QStringList l;
    l << (name().isEmpty()?ChanAddress:name());
    if (!correction().isEmpty()) l << correction();
    if (!parent->legend().isEmpty()) l << parent->legend();

    return l.join(" ");
}

void DfdChannel::setValue(double val, QDataStream &writeStream)
{
    double realValue = preprocess(val);

    switch (IndType) {
        case 0x00000001: {
            quint8 v = (quint8)realValue;
            writeStream << v;
            break;}
        case 0x80000001: {
            qint8 v = (qint8)realValue;
            writeStream << v;
            break;}
        case 0x00000002: {
            quint16 v = (quint16)realValue;
            writeStream << v;
            break;}
        case 0x80000002: {
            qint16 v = (qint16)realValue;
            writeStream << v;
            break;}
        case 0x00000004: {
            quint32 v = (quint32)realValue;
            writeStream << v;
            break;}
        case 0x80000004: {
            qint32 v = (qint32)realValue;
            writeStream << v;
            break;}
        case 0x80000008: {
            qint64 v = (qint64)realValue;
            writeStream << v;
            break;}
        case 0xC0000004: {
            float v = (float)realValue;
            writeStream << v;
            break;}
        case 0xC0000008:
        case 0xC000000A:
            writeStream << realValue;
            break;
        default: break;
    }
}

Descriptor::DataType DfdChannel::type() const
{DD;
    return dataTypefromDfdDataType(dataType);
}

Descriptor::OrdinateFormat DfdChannel::yFormat() const
{
//    switch (IndType) {
//        case 0xC0000004: return Descriptor::RealSingle;
//        default: break;
//    }
//    return Descriptor::RealDouble;
    return Descriptor::RealSingle;
}

QString DfdChannel::xName() const
{DD;
    if (parent) return parent->xName();
    return "";
}

QString DfdChannel::yName() const
{
//    if (YName.isEmpty())
//        return YNameOld;
//    if ((YName.toLower() == "дб" || YName.toLower() == "db") && !YNameOld.isEmpty())
//        return YNameOld;
    return YName;
}

void DfdChannel::setYName(const QString &yName)
{
    if (yName == YName) return;
//    if ((YName.toLower() == "дб" || YName.toLower() == "db") && !YNameOld.isEmpty()) {
//        YNameOld = yName;
//    }
//    else
        YName = yName;
}

void DfdChannel::setName(const QString &name)
{
    if (ChanName == name) return;
    ChanName = name;
}

RawChannel::RawChannel(RawChannel &other, DfdFileDescriptor *parent) : DfdChannel(other, parent)
{
    SensName = other.SensName;
    ADC0 = other.ADC0;
    ADCStep = other.ADCStep;
    AmplShift = other.AmplShift;
    AmplLevel = other.AmplLevel;
    Sens0Shift = other.Sens0Shift;
    SensSensitivity = other.SensSensitivity;
    BandWidth = other.BandWidth;
    coef1 = other.coef1;
    coef2 = other.coef2;
    coef3 = other.coef3;
    coef4 = other.coef4;
}

void RawChannel::read(DfdSettings &dfd, int numChans)
{DD;
    DfdChannel::read(dfd, numChans);
    QString group = QString("Channel%1/").arg(channelIndex+1);

    ADC0 = hextodouble( dfd.value(group+"ADC0")); //qDebug()<<"ADC0" << ch.adc0;
    ADCStep = hextodouble(dfd.value(group+"ADCStep")); //qDebug()<< "ADCStep"<< ch.adcStep;
    AmplShift = hextodouble(dfd.value(group+"AmplShift")); //qDebug()<<"AmplShift" <<ch.amplShift ;
    AmplLevel = hextodouble(dfd.value(group+"AmplLevel")); //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    Sens0Shift = hextodouble(dfd.value(group+"Sens0Shift")); //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    SensSensitivity = hextodouble(dfd.value(group+"SensSensitivity")); //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    BandWidth = hextofloat(dfd.value(group+"BandWidth")); //qDebug()<< "BandWidth"<< ch.bandwidth;
    SensName = dfd.value(group+"SensName");

    coef1 = ADCStep / AmplLevel / SensSensitivity;
    coef2 = (ADC0 / AmplLevel - AmplShift - Sens0Shift) / SensSensitivity;

    coef3 = SensSensitivity * AmplLevel / ADCStep;
    coef4 = ((Sens0Shift + AmplShift) * AmplLevel - ADC0) / ADCStep;
}

void RawChannel::write(QTextStream &dfd, int index)
{DD;
    DfdChannel::write(dfd, index);

    dfd << "ADC0=" <<  doubletohex(ADC0).toUpper() << endl;
    dfd << "ADCStep=" <<  doubletohex(ADCStep).toUpper() << endl; //qDebug()<< "ADCStep"<< ch.adcStep;
    dfd << "AmplShift=" << doubletohex(AmplShift).toUpper() << endl; //qDebug()<<"AmplShift" <<ch.amplShift ;
    dfd << "AmplLevel=" << doubletohex(AmplLevel).toUpper() << endl; //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    dfd << "Sens0Shift=" << doubletohex(Sens0Shift).toUpper() << endl; //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    dfd << "SensSensitivity=" << doubletohex(SensSensitivity).toUpper() << endl; //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    dfd << "BandWidth=" <<  floattohex(BandWidth).toUpper() << endl;
    dfd << "SensName=" <<  SensName << endl;
}

QVariant RawChannel::channelHeader(int column) const
{
    if (column >=0 && column < DfdChannel::columnsCount())
        return DfdChannel::channelHeader(column);
    switch (column) {
        case 5: return QString("Смещ."); //ADC0
        case 6: return QString("Множ."); //ADCStep
        case 7: return QString("Смещ.ус.(В)"); //AmplShift
        case 8: return QString("Ус.(дБ)"); //AmplLevel
        case 9: return QString("Смещ.датч.(мВ)"); //Sens0Shift
        case 10: return QString("Чувств."); //SensSensitivity
        case 11: return QString("Полоса"); //BandWidth
        case 12: return QString("Датчик"); //SensName
    }
    return QVariant();
}

QVariant RawChannel::info(int column) const
{
    if (column >= 0 && column < DfdChannel::columnsCount())
        return DfdChannel::info(column);
    switch (column) {
        case 5: return ADC0; //ADC0
        case 6: return ADCStep; //ADCStep
        case 7: return AmplShift; //AmplShift
        case 8: return AmplLevel; //AmplLevel
        case 9: return Sens0Shift; //Sens0Shift
        case 10: return SensSensitivity; //SensSensitivity
        case 11: return BandWidth; //BandWidth
        case 12: return SensName; //SensName
    }
    return QVariant();
}

int RawChannel::columnsCount() const
{
    return 13;
}

void RawChannel::postprocess(QVector<double> &v)
{
    for (int i=0; i<v.size(); ++i) v[i] = v[i]*coef1+coef2;
}

double RawChannel::postprocess(double v)
{
    //return ((v*ADCStep+ADC0)/AmplLevel - AmplShift - Sens0Shift)/SensSensitivity;
    return v*coef1+coef2;
}

double RawChannel::preprocess(double v)
{
    //return ((v * SensSensitivity + Sens0Shift + AmplShift) * AmplLevel - ADC0) / ADCStep;
    return v*coef3+coef4;
}

DataDescription::DataDescription(DfdFileDescriptor *parent) : parent(parent)
{DD;

}

void DataDescription::read(DfdSettings &dfd)
{DD;
    DescriptionList list = dfd.values("DataDescription");

    foreach (DescriptionEntry entry, list) {
        QString v = entry.second;
        if (v.startsWith("\"")) v.remove(0,1);
        if (v.endsWith("\"")) v.chop(1);
        v = v.trimmed();
        if (!v.isEmpty()) {
            if (entry.first == "Legend")
                parent->_legend = v;
            else
                data.append(DescriptionEntry(entry.first, v));
        }
    }
}

void DataDescription::write(QTextStream &dfd)
{DD;
    if (!data.isEmpty() || !parent->legend().isEmpty()) {
        dfd << "[DataDescription]" << endl;
        for (int i = 0; i<data.size(); ++i) {
            DescriptionEntry item = data.at(i);
            dfd << item.first << "=\"" << item.second << "\"" << endl;
        }
        if (!parent->legend().isEmpty())
            dfd << "Legend=" << "\"" << parent->legend() << "\"" << endl;
    }
}

QString DataDescription::toString() const
{DD;
    QStringList result;
    for (int i=0; i<data.size(); ++i) {
        DescriptionEntry item = data.at(i);
        result.append(/*item.first+"="+*/item.second);
    }
    return result.join("; ");
}


DfdDataType dfdDataTypeFromDataType(Descriptor::DataType type)
{DD;
    switch (type) {
        case Descriptor::TimeResponse : return SourceData;
        case Descriptor::AutoSpectrum : return XSpectr; //!
        case Descriptor::CrossSpectrum : return XSpectr;
        case Descriptor::FrequencyResponseFunction : return TransFunc;
        case Descriptor::Transmissibility : return TrFuncRe; //!
        case Descriptor::Coherence : return Coherence;
        case Descriptor::AutoCorrelation : return AutoCorr;
        case Descriptor::CrossCorrelation : return CrossCorr;
        case Descriptor::PowerSpectralDensity : return SpcDens;
        case Descriptor::EnergySpectralDensity : return SpcDev; //!
        case Descriptor::ProbabilityDensityFunction : return HistP;
        case Descriptor::Spectrum : return Spectr;
        default : return NotDef;
    }
    return NotDef;
}


Descriptor::DataType dataTypefromDfdDataType(DfdDataType type)
{DD;
    switch (type) {
        case SourceData:
        case CuttedData:
        case FilterData: return Descriptor::TimeResponse;
        case AutoCorr: return Descriptor::AutoCorrelation;
        case CrossCorr: return Descriptor::CrossCorrelation;
        case HistP: return Descriptor::ProbabilityDensityFunction;
        case Spectr: return Descriptor::Spectrum;
        case SpcDens: return Descriptor::PowerSpectralDensity;
        case SpcDev: return Descriptor::EnergySpectralDensity;
        case XSpectr: return Descriptor::CrossSpectrum;
        case Coherence: return Descriptor::Coherence;
        case TransFunc: return Descriptor::FrequencyResponseFunction;
        case TrFuncRe: return Descriptor::Transmissibility;
        default: return Descriptor::Unknown;
    }
    return Descriptor::Unknown;
}


QString DfdFileDescriptor::saveTimeSegment(double from, double to)
{
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (DataType != SourceData &&
        DataType != CuttedData &&
        DataType != FilterData) return QString();

//    populate();
    const int count = channelsCount();

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString newFileName = createUniqueFileName("", fileName(), suffix, "dfd", false);

    // 2 создаем новый файл
    DfdFileDescriptor *newDfd = new DfdFileDescriptor(newFileName);
    newDfd->fillPreliminary(Descriptor::TimeResponse);
    newDfd->DataType = CuttedData;
    newDfd->BlockSize = 0;
    newDfd->XName = this->XName;
    newDfd->XBegin = 0.0;
    newDfd->XStep = this->XStep;
    newDfd->_legend = this->_legend;
    newDfd->DescriptionFormat = this->DescriptionFormat;

    // [DataDescription]
    if (this->dataDescription) {
        newDfd->dataDescription = new DataDescription(newDfd);
        newDfd->dataDescription->data = dataDescription->data;
    }

    // [Sources]
    newDfd->source = new Source();
    QStringList procChansList;
    for (int i=1; i<=count; ++i) procChansList << QString::number(i);
    newDfd->source->sFile = fileName()+"["+procChansList.join(",")+"]"+DFDGUID;

    // 3 ищем границы данных по параметрам from и to
    int sampleStart = qRound((from/*-XBegin*/)/XStep);
    if (sampleStart<0) sampleStart = 0;
    int sampleEnd = qRound((to/*-XBegin*/)/XStep);
    if (sampleEnd>=samplesCount()) sampleEnd = samplesCount()-1;
    newDfd->setSamplesCount(sampleEnd - sampleStart + 1); //число отсчетов в новом файле

    // и заполняем параметр Process->BlockIn
    // [Process]
    newDfd->process = new Process();
    DescriptionList list;
    list.append({"pName", "Осциллограф"});
    list.append({"pTime","(0000000000000000)"});
    list.append({"ProcChansList", procChansList.join(",")});
    list.append({"BlockIn", QString::number(newDfd->samplesCount())});
    list.append({"TypeProc", "0"});
    list.append({"NAver", "1"});
    list.append({"Values", "измеряемые"});
    newDfd->process->data = list;

    // 4 сохраняем файл

    for (int i=0; i<count; ++i) {
        bool wasPopulated = channels[i]->populated();
        if (!wasPopulated) channels[i]->populate();

        DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
        ch->data()->setSegment(*(channels[i]->data()), sampleStart, sampleEnd);

        ch->setPopulated(true);
        ch->setName(channels[i]->name());

        ch->ChanDscr = channels[i]->ChanDscr;
        ch->ChanAddress = channels[i]->ChanAddress;

        ch->ChanBlockSize = sampleEnd - sampleStart + 1;
        ch->IndType = 3221225476;

        ch->YName = channels[i]->YName;
        ch->YNameOld = channels[i]->YNameOld;

        ch->setCorrection(channels[i]->correction());

        newDfd->channels << ch;
        if (!wasPopulated) {
            //clearing data
            channels[i]->data()->clear();
        }
    }

    newDfd->setSamplesCount(newDfd->channel(0)->samplesCount());
    newDfd->setChanged(true);
    newDfd->setDataChanged(true);
    newDfd->write();
    newDfd->writeRawFile();
    delete newDfd;

    // 5 возвращаем имя нового файла
    return newFileName;
}


int DfdFileDescriptor::samplesCount() const
{
    return NumInd;
}

void DfdFileDescriptor::setSamplesCount(int count)
{
    NumInd = count;
}



//DfdOctaveFileDescriptor::DfdOctaveFileDescriptor(const QString &fileName)
//    : DfdFileDescriptor(fileName)
//{DD;
//    firstChannel = 0;
//}

//DfdOctaveFileDescriptor::DfdOctaveFileDescriptor(const DfdFileDescriptor &d)
//    : DfdFileDescriptor(d)
//{DD;
//    firstChannel = 0;
//}

//DfdOctaveFileDescriptor::DfdOctaveFileDescriptor(const DfdOctaveFileDescriptor &d)
//    : DfdFileDescriptor(d)
//{DD;
//    firstChannel = 0;
//}


//void DfdOctaveFileDescriptor::read()
//{DD;
//    // прочитали каналы как есть
//    DfdFileDescriptor::read();


//}

//void DfdOctaveFileDescriptor::write()
//{
//    if (!changed()) return;

//    if (firstChannel) {
//        if (!channels.isEmpty() && channels.first() != firstChannel) {
//            channels.prepend(firstChannel);
//            NumChans = channels.size();
//            for (int i=0; i<channels.size(); ++i)
//                channels[i]->channelIndex = i;
//        }
//    }
//    // возвращаем старое название оси х
//    XName = "№№ полос";
//    DfdFileDescriptor::write();
//    //обратная еботнина
//    channels.removeFirst();
//    for (int i=0; i<channels.size(); ++i)
//        channels[i]->channelIndex = i;
//    NumChans = channels.size();
//}

//void DfdOctaveFileDescriptor::writeRawFile()
//{

//}

//void DfdOctaveFileDescriptor::populate()
//{
//}


int DfdChannel::index() const
{
    return channelIndex;
}


