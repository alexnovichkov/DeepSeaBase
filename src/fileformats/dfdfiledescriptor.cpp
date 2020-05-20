#include "dfdfiledescriptor.h"
#include <QtCore>

#include <QtWidgets>
#include <QUuid>

#include "logging.h"
#include "algorithms.h"
#include "dfdsettings.h"
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
        case 157: return QString("1/3-окт.спектр"); break;
        case 158: return QString("1/2-окт.спектр"); break;
        case 159: return QString("1/6-окт.спектр"); break;
        case 160: return QString("1/12-окт.спектр"); break;
        case 161: return QString("1/24-окт.спектр"); break;
        default: return QString("Неопр.");
    }
    return QString("Неопр.");
}


template <typename T>
QVector<T> convertFromUINT16(unsigned char *ptr, qint64 length, uint IndType)
{
    uint step = IndType % 16;
    QVector<T> temp(length / step, 0.0);

    int i=0;
    while (length) {
        temp[i++] = static_cast<T>(qFromLittleEndian<quint16>(ptr));
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
      XStep(0.0),
      source(0),
      process(0),
      dataDescription(0)
{DD;
//    rawFileChanged = false;
    //    changed = false;
}

// creates a copy of DfdDataDescriptor without copying data
DfdFileDescriptor::DfdFileDescriptor(const DfdFileDescriptor &d, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{
    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<d.channelsCount(); ++i) indexes << i;

    DFDGUID = createGUID();
    rawFileName = fileName.left(fileName.length()-3)+"raw";
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
    for (int i: indexes)
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

    for (int i: indexes) {
        DfdChannel *c = d.channels.at(i);
        if (RawChannel *rc = dynamic_cast<RawChannel*>(c))
            this->channels << new RawChannel(*rc, this);
        else
            this->channels << new DfdChannel(*c, this);
    }

    //перенумеровываем
    for (int i=0; i<channels.count(); ++i) {
        channels[i]->channelIndex = i;
    }

    _legend = d._legend;

    setChanged(true);
    write();

    //данные
    QFile rawFile(rawFileName);
    if (!rawFile.open(QFile::WriteOnly)) {
        qDebug()<<"Cannot open file"<<rawFileName<<"to write";
        return;
    }
    QDataStream writeStream(&rawFile);
    writeStream.setByteOrder(QDataStream::LittleEndian);

    int destIndex = 0;
    for (int i=0; i<d.channels.count(); ++i) {
        if (!indexes.contains(i)) continue;

        Channel *sourceChannel = d.channel(i);
        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        //кусок кода из writeRawFile
        DfdChannel *destChannel = channels[destIndex];

        if (destChannel->IndType==0xC0000004)
            writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        const int sc = sourceChannel->samplesCount();


        QVector<double> yValues = sourceChannel->data()->rawYValues();
        if (yValues.isEmpty() && !sourceChannel->data()->yValuesComplex().isEmpty()) {
            yValues = sourceChannel->data()->linears();
            destChannel->data()->setYValuesFormat(DataHolder::YValuesAmplitudes);
        }
        int scc = qMin(sc, yValues.size());
        for (int val = 0; val < scc; ++val) {
            destChannel->setValue(yValues[val], writeStream);
        }

        if (!populated) {
            sourceChannel->clear();
            destChannel->clear();
        }
        destIndex++;
    }
}

DfdFileDescriptor::DfdFileDescriptor(const FileDescriptor &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)

{
    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;

    DFDGUID = createGUID();
    rawFileName = fileName.left(fileName.length()-3)+"raw";

    bool unevenX = false;

    this->Date = QDate::currentDate();
    this->Time = QTime::currentTime();
    this->CreatedBy = "DeepSeaBase by Novichkov & sukin sons";

    this->BlockSize = 0; // всегда меняем размер блока новых файлов на 0,
                         // чтобы они записывались без перекрытия

    //Поскольку other может содержать каналы с разным типом, размером и шагом,
    //данные берем из первого канала, который будем сохранять
    //предполагается, что все каналы из indexes имеют одинаковые параметры

    Channel *firstChannel = other.channel(indexes.constFirst());
    Q_ASSERT_X(firstChannel, "Dfd constructor", "channel to copy is null");
    this->DataType = dfdDataTypeFromDataType(firstChannel->type());
    switch (firstChannel->octaveType()) {
        case 1: DataType = OSpectr; break;
        case 2: DataType = TwoOSpectr; break;
        case 3: DataType = ToSpectr; break;
        case 6: DataType = SixOSpectr; break;
        case 12: DataType = TwlOSpectr; break;
        case 24: DataType = TFOSpectr; break;
        default: break;
    }

    // меняем тип данных временной реализации на вырезку, чтобы DeepSea не пытался прочитать файл как сырые данные
    if (this->DataType == SourceData) this->DataType = CuttedData;
    this->NumInd = firstChannel->samplesCount();
    this->XName = firstChannel->xName();

    this->XBegin = firstChannel->data()->xMin();
    this->XStep = firstChannel->xStep();
    this->DescriptionFormat = "";

    source = new Source();
    source->Date=other.dateTime().date();
    source->Time=other.dateTime().time();
    source->DFDGUID = "Unknown";
    source->File = other.fileName();
    for (int i: indexes)
        source->ProcChansList << i;

    DescriptionList dd = other.dataDescriptor();
    dataDescription = 0;
    if (!dd.isEmpty()) {
        dataDescription = new DataDescription(this);
        dataDescription->data = dd;
    }
    process = 0;
    _legend = other.legend();

    //только описание каналов
    for (int i: indexes) {
        Channel *c = other.channel(i);
        this->channels << new DfdChannel(*c, this);
    }

    unevenX = !channels.isEmpty() && channels.constFirst()->data()->xValuesFormat()==DataHolder::XValuesNonUniform;

    if (unevenX) {
        //добавляем нулевой канал с осью Х
        DfdChannel *ch = new DfdChannel(this, 0);
        ch->ChanAddress.clear(); //
        ch->ChanName = "ось X"; //
        ch->YName="Гц";
        ch->YNameOld.clear();
        ch->InputType.clear();
        ch->ChanDscr.clear();
        ch->ChanBlockSize = channels.constFirst()->samplesCount();
        ch->IndType = 3221225476;
        ch->data()->setThreshold(1.0);
        ch->data()->setXValues(channels.constFirst()->xValues());

        channels.prepend(ch);
    }

    //сохраняем файл без данных
    //перенумеровываем
    for (int i=0; i<channels.count(); ++i) {
        channels[i]->channelIndex = i;
    }
    setChanged(true);
    write();

    //сохраняем данные
    QFile rawFile(rawFileName);
    if (!rawFile.open(QFile::WriteOnly)) {
        qDebug()<<"Cannot open file"<<rawFileName<<"to write";
        return;
    }
    QDataStream writeStream(&rawFile);
    writeStream.setByteOrder(QDataStream::LittleEndian);

    //записываем данные канала Х, если шаг неравномерный
    if (unevenX) {
        DfdChannel *destChannel = channels.constFirst();
        if (destChannel->IndType==0xC0000004)
            writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        QVector<double> yValues = destChannel->data()->xValues();
        for (int val = 0; val < yValues.size(); ++val) {
            destChannel->setValue(yValues[val], writeStream);
        }
    }

    int destIndex = unevenX ? 1 : 0; //начинаем со второго канала, если добавляли канал Х
    for (int i=0; i<other.channelsCount(); ++i) {
        if (!indexes.contains(i)) continue;

        Channel *sourceChannel = other.channel(i);
        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        //кусок кода из writeRawFile
        DfdChannel *destChannel = channels[destIndex];

        if (destChannel->IndType==0xC0000004)
            writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        const int sc = sourceChannel->samplesCount();


        QVector<double> yValues = sourceChannel->data()->rawYValues();
        if (yValues.isEmpty() && !sourceChannel->data()->yValuesComplex().isEmpty()) {
            yValues = sourceChannel->data()->linears();
            destChannel->data()->setYValuesFormat(DataHolder::YValuesAmplitudes);
        }
        int scc = qMin(sc, yValues.size());
        for (int val = 0; val < scc; ++val) {
            destChannel->setValue(yValues[val], writeStream);
        }

        if (!populated) {
            sourceChannel->clear();
            destChannel->clear();
        }
        destIndex++;
    }
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

    // проверяем неравномерность шкалы
    if (!channels.isEmpty()) {
        DfdChannel *firstChannel = channels.constFirst();
        if (firstChannel->data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
            firstChannel->populate();
            QVector<double> xvalues = firstChannel->data()->yValues();

            //для октавного и третьоктавного спектра значения полос могут
            //отсутствовать в файле. Проверяем и создаем, если надо
            if (DataType == OSpectr || DataType == ToSpectr) {
                XName = "Гц";
                // первым каналом записаны центральные частоты, сохраняем их как значения по X

                //определяем, действительно ли это значения фильтров
                if (!looksLikeOctave(xvalues, DataType)) {
                    xValues.clear();
                    if (DataType == ToSpectr) {
                        for (int i=0; i<NumInd; ++i) xValues << pow(10.0, 0.1*(i+1));
                    }
                    else if (DataType == OSpectr) {
                        for (int i=0; i<NumInd; ++i) xValues << pow(2.0, i+1);
                    }
                }
            }
            else {
                xValues = xvalues;
                firstChannel->data()->setXValues(xValues);
            }
        }
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

    /** [DataFileDescriptor]*/
    dfd << "[DataFileDescriptor]" << endl;
    dfd << "DFDGUID="<<DFDGUID << endl;
    dfd << "DataType="<<DataType << endl;
    dfd << "Date="<<Date.toString("dd.MM.yyyy") << endl;
    dfd << "Time="<<Time.toString("hh:mm:ss") << endl;
    dfd << "NumChans="<<channels.size() << endl;
    dfd << "NumInd="<<NumInd << endl;
    dfd << "BlockSize="<<BlockSize << endl;
    dfd << "XName="<<XName << endl;
    dfd << "XBegin="<<doubletohex(XBegin).toUpper() << endl;
    dfd << "XStep="<<doubletohex(XStep).toUpper() << endl;
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

    /** Channels*/
    int b = 0;

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
        QDataStream writeStream(&rawFile);
        writeStream.setByteOrder(QDataStream::LittleEndian);

        if (BlockSize == 0) {
            // пишем поканально
            for (int i = 0; i<channels.size(); ++i) {
                DfdChannel *ch = channels[i];

                if (ch->IndType==0xC0000004)
                    writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                const int sc = ch->samplesCount();

                QVector<double> yValues = ch->data()->rawYValues();
                if (yValues.isEmpty() && !ch->data()->yValuesComplex().isEmpty())
                    yValues = ch->data()->linears();
                int scc = qMin(sc, yValues.size());
                for (int val = 0; val < scc; ++val) {
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
                    else if (ch->IndType==0xC0000008)
                        writeStream.setFloatingPointPrecision(QDataStream::DoublePrecision);

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

    setSamplesCount(channels.constFirst()->samplesCount());
    BlockSize = 0;
//    XName = channels.constFirst()->xName();
    XBegin = channels.constFirst()->xMin();
    XStep = channels.constFirst()->xStep();
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

bool DfdFileDescriptor::copyTo(const QString &name)
{
    QString rawFile = name;
    QString suffix = QFileInfo(name).suffix();
    rawFile.replace(rawFile.length() - suffix.length(), suffix.length(), "raw");

    return FileDescriptor::copyTo(name) && QFile::copy(rawFileName, rawFile);
}

DfdFileDescriptor *DfdFileDescriptor::newThirdOctaveFile(const QString &fileName)
{
    DfdFileDescriptor *dfd = new DfdFileDescriptor(fileName);

    dfd->rawFileName = fileName.left(fileName.length()-4)+".raw";
    dfd->updateDateTimeGUID();
    dfd->DataType = ToSpectr;

    dfd->BlockSize = 0;
    dfd->XName="Гц";
    dfd->XStep = 0.0;

    dfd->process = new Process();
    dfd->process->data.append(qMakePair(QStringLiteral("pName"),QStringLiteral("1/3-октавный спектр")));
    dfd->process->data.append(qMakePair(QStringLiteral("pTime"),QStringLiteral("(0000000000000000)")));
    dfd->process->data.append(qMakePair(QStringLiteral("TypeProc"),QStringLiteral("1/3-октава")));
    dfd->process->data.append(qMakePair(QStringLiteral("Values"),QStringLiteral("измеряемые")));
    dfd->process->data.append(qMakePair(QStringLiteral("TypeScale"),QStringLiteral("в децибелах")));

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
//    write();
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

bool DfdFileDescriptor::fileExists() const
{
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

    // не удалось переписать файл raw, читаем весь файл и перезаписываем стандартным интерфейсом
    if (!tempFileSuccessful)
        populate();

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    //перенумеровываем каналы
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

//    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file);
    foreach (int index, indexes) {
        bool wasPopulated = file->channel(index)->populated();
        if (!wasPopulated) file->channel(index)->populate();
        channels.append(new DfdChannel(*file->channel(index), this));
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

void DfdFileDescriptor::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{DD;
//    //заглушка для релиза
//    copyChannelsFrom_plain(file, indexes);
//    return;

    const int count = channelsCount();
    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(sourceFile);

    //еще мы должны убедиться, что типы файлов совпадают, если текущий файл не пустой
    if (count > 0) {
        if (dfd) {
            //частный случай: мы можем записать данные из SourceData в CuttedData, преобразовав их в floats,
            //но не наоборот
            if ((dfd->DataType != DataType) &&
                (dfd->DataType > 15 ||
                 DataType > 15 ||
                 (dfd->DataType != SourceData && DataType == SourceData))) {
                qDebug()<<"Попытка скопировать каналы в файл с другим типом";
                return;
            }
        }
        else if (dataTypefromDfdDataType(DataType) != sourceFile->type()) {
            qDebug()<<"Попытка скопировать каналы в файл с другим типом";
            return;
        }
    }

    bool rewrited = false;

    //мы должны добавить каналы из file
    //и дописать к файлу данных новые данные

    if (count > 0) {
        //особое внимание нужно уделить тем файлам, у которых BlockSize != 0,
        //перезаписываем файл данных, чтобы не мучаться с блоками отсчетов
        if (BlockSize > 0) {
            QVector<QPair<int, int> > inds;
            for (int i=0; i<count; ++i) inds.append(qMakePair<int,int>(i,0)); // 0 = keep, 1 = delete
            rewrited = rewriteRawFile(inds);
        }

        if (BlockSize > 0 || !rewrited) {
            // не удалось переписать файл. Добавляем каналы стандартным способом
            copyChannelsFrom_plain(sourceFile, indexes);
            return;
        }
    }

    BlockSize = 0;

    //1. копируем каналы из file в this
    for (int i: indexes) {
        Channel *sourceChannel = sourceFile->channel(i);

        //заполняем канал данными
        bool wasPopulated = sourceChannel->populated();
        if (!wasPopulated) sourceChannel->populate();

        DfdChannel *newCh = 0;

        //особый случай - из RawChannel в RawChannel
        RawChannel *rawSource = dynamic_cast<RawChannel *>(sourceChannel);
        RawChannel *rawDest = 0;
        if (count > 0) {
            rawDest = dynamic_cast<RawChannel *>(channels[0]);
        }

        if (rawSource && rawDest)
            newCh = new RawChannel(*rawSource, this);
        else
            newCh = new DfdChannel(*sourceChannel, this);

        if (newCh->data()->xValuesFormat() == DataHolder::XValuesNonUniform)
            xValues = newCh->data()->xValues();

        //меняем параметры канала
        newCh->IndType = channels[0]->IndType; //характеристика отсчета
        newCh->ChanBlockSize = this->NumInd; //размер блока в отсчетах

        //меняем, если копировали из Source в Cutted
        newCh->dataType = DataType;

        //добавляем данные в файл raw напрямую, обходя функцию writeRawFile
        //функция ничего не делает, если BlockSize != 0
        newCh->setPopulated(true);
        newCh->appendDataTo(rawFileName);

        channels << newCh;
        if (!wasPopulated) {
            sourceChannel->clear();
            newCh->clear();
        }
    }

    //перенумеровываем каналы
    for (int i=0; i<channels.size(); ++i) {
        channels[i]->channelIndex = i;
    }

    //сохраняем описатель
    setChanged(true);
    write();
}

void DfdFileDescriptor::calculateMean(const QList<Channel*> &channels)
{DD;
    Channel *firstChannel = channels.constFirst();

    //ищем наименьшее число отсчетов
    int numInd = firstChannel->samplesCount();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->samplesCount() < numInd)
            numInd = channels.at(i)->samplesCount();
    }

    // ищем формат данных для нового канала
    // если форматы разные, то формат будет линейный (амплитуды), не логарифмированный
    auto format = firstChannel->data()->yValuesFormat();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->yValuesFormat() != format) {
            format = DataHolder::YValuesAmplitudes;
            break;
        }
    }

    int units = firstChannel->units();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->units() != units) {
            units = DataHolder::UnitsUnknown;
            break;
        }
    }

    Averaging averaging(Averaging::Linear, channels.size());

    foreach (Channel *ch, channels) {
        if (ch->data()->yValuesFormat() == DataHolder::YValuesComplex)
            averaging.average(ch->data()->yValuesComplex());
        else
            averaging.average(ch->data()->linears());
    }

    // обновляем сведения канала
    DfdChannel *ch = new DfdChannel(this, channelsCount());
    ch->setPopulated(true);
    ch->setChanged(true);
    ch->setDataChanged(true);

    QStringList l;
    foreach (Channel *c,channels) {
        l << c->name();
    }
    ch->ChanName = "Среднее "+l.join(", ");
    l.clear();
    for (int i=0; i<channels.size(); ++i) {
        l << QString::number(channels.at(i)->index()+1);
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

    ch->IndType = this->channels.isEmpty()?3221225476:this->channels.constFirst()->IndType;
    ch->YName = firstChannel->yName();
    //грязный хак
    if (DfdChannel *dfd = dynamic_cast<DfdChannel*>(firstChannel)) {
        ch->YNameOld = dfd->YNameOld;
        if (ch->YName == ch->YNameOld && format == DataHolder::YValuesAmplitudesInDB)
            ch->YName = "дБ";
    }
    if (XName.isEmpty()) XName = firstChannel->xName();

    this->channels << ch;

    setChanged(true);
    setDataChanged(true);
    write();
    writeRawFile();
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

        auto format = firstChannel->data()->yValuesFormat();
        if (format == DataHolder::YValuesComplex) {
            ch->data()->setYValues(movingAverage(firstChannel->data()->yValuesComplex(), windowSize));
        }
        else {
            QVector<double> values = movingAverage(firstChannel->data()->linears(), windowSize);
            if (format == DataHolder::YValuesAmplitudesInDB)
                format = DataHolder::YValuesAmplitudes;
            ch->data()->setYValues(values, format);
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

        ch->IndType = this->channels.isEmpty()?3221225476:this->channels.constFirst()->IndType;
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


    thirdOctDfd->XBegin = thirdOctDfd->xValues.constFirst();
    thirdOctDfd->setSamplesCount(thirdOctDfd->channels.last()->data()->samplesCount());

    thirdOctDfd->setChanged(true);
    thirdOctDfd->setDataChanged(true);
    thirdOctDfd->write();
    thirdOctDfd->writeRawFile();
    delete thirdOctDfd;
    return thirdOctaveFileName;
}

//bool DfdFileDescriptor::rewriteRawFile(const QVector<QPair<int,int> > &indexesVector)
//{
//    if (indexesVector.isEmpty()) return false;

//    // переписываем файл данных во временный файл, которым затем подменим исходный raw
//    QTemporaryFile tempFile;
//    QFile rawFile(rawFileName);

//    if (tempFile.open() && rawFile.open(QFile::ReadOnly)) {
//        qDebug()<<"работаем через временный файл";
//        //работаем через временный файл - читаем один канал и записываем его во временный файл
//        if (!channels[0]->dataPositions.isEmpty()) {
//            qDebug()<<"- используем dataPositions";
//            for (int ind = 0; ind < indexesVector.size(); ++ind) {

//                // пропускаем канал, предназначенный для удаления
//                if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

//                DfdChannel *ch = channels[indexesVector.at(ind).first];
//                qint64 dataPos = tempFile.pos();
//                //пишем канал целиком
//                foreach (int pos, ch->dataPositions) {
//                    rawFile.seek(pos);
//                    QByteArray b = rawFile.read(ch->blockSizeInBytes());
//                    tempFile.write(b);
//                }
//                tempFile.flush();

//                // меняем размер блока и обновляем положение данных
//                ch->dataPositions.clear();
//                ch->dataPositions.append(dataPos);
//                ch->ChanBlockSize = this->NumInd;
//                this->BlockSize = 0;
//                qDebug()<<"-- меняем BlockSize на 0";
//            }
//        }
//        else {
//            qDebug()<<"- dataPositions отсутствуют";
//            if (BlockSize == 1) {
//                qDebug()<<"-- BlockSize == 1";
//                // trying to map file into memory
//                unsigned char *ptr = rawFile.map(0, rawFile.size());
//                if (ptr) {//достаточно памяти отобразить весь файл
//                    qDebug()<<"-- работаем через ptr";
//                    qDebug()<<"-- меняем BlockSize на 0";
//                    unsigned char *ptrCurrent = ptr;

//                    for (int ind = 0; ind < indexesVector.size(); ++ind) {
//                        // пропускаем канал, предназначенный для удаления
//                        if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

//                        DfdChannel *ch = channels[indexesVector.at(ind).first];
//                        qint64 dataPos = tempFile.pos();
//                        const int bytes = ch->IndType % 16;
//                        for (int i=0; i<this->NumInd; ++i) {
//                            ptrCurrent = ptr + (indexesVector.at(ind).first + i*channelsCount()) * bytes;
//                            tempFile.write((char*)(ptrCurrent), bytes);
//                        }
//                        tempFile.flush();

//                        // меняем размер блока и обновляем положение данных
//                        ch->dataPositions.clear();
//                        ch->dataPositions.append(dataPos);
//                        ch->ChanBlockSize = this->NumInd;
//                        this->BlockSize = 0;
//                    }

//                    rawFile.unmap(ptr);
//                }
//                else {
//                    qDebug()<<"-- не меняем BlockSize";
//                    // не удалось прочитать никакими оптимальными методами, читаем медленным классическим
//                    int actuallyRead = 0;
//                    const int chunkSizeBytes = channelsCount() * channels[0]->blockSizeInBytes();

//                    while (1) {
//                        QByteArray temp = rawFile.read(chunkSizeBytes);
//                        actuallyRead = temp.size();
//                        if (actuallyRead == 0)
//                            break;
//                        if (actuallyRead < chunkSizeBytes) {
//                            temp.resize(chunkSizeBytes);
//                            actuallyRead = 0;
//                        }

//                        for (int ind = 0; ind < indexesVector.size(); ++ind) {
//                            // пропускаем канал, предназначенный для удаления
//                            if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

//                            tempFile.write(temp.mid(indexesVector.at(ind).first * channels[indexesVector.at(ind).first]->blockSizeInBytes(),
//                                           channels[indexesVector.at(ind).first]->blockSizeInBytes()));
//                        }

//                        if (actuallyRead == 0)
//                            break;
//                    }
//                }
//            }
//            else {
//                qDebug()<<"-- не меняем BlockSize";
//                // не удалось прочитать никакими оптимальными методами, читаем медленным классическим
//                int actuallyRead = 0;
//                const int chunkSizeBytes = channelsCount() * channels[0]->blockSizeInBytes();

//                while (1) {
//                    QByteArray temp = rawFile.read(chunkSizeBytes);
//                    actuallyRead = temp.size();
//                    if (actuallyRead == 0)
//                        break;
//                    if (actuallyRead < chunkSizeBytes) {
//                        temp.resize(chunkSizeBytes);
//                        actuallyRead = 0;
//                    }

//                    for (int ind = 0; ind < indexesVector.size(); ++ind) {
//                        // пропускаем канал, предназначенный для удаления
//                        if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

//                        tempFile.write(temp.mid(indexesVector.at(ind).first * channels[indexesVector.at(ind).first]->blockSizeInBytes(),
//                                       channels[indexesVector.at(ind).first]->blockSizeInBytes()));
//                    }

//                    if (actuallyRead == 0)
//                        break;
//                }
//            }
//        }

//        //подменяем исходный файл raw временным
//        rawFile.close();
//        tempFile.close();
//        if (rawFile.remove()) {
//            tempFile.copy(rawFileName);
//            return true;
//        }
//        else {
//            qDebug()<<" Couldn't replace raw file with temp file.";
//            return false;
//        }
//    }
//    return false;
//}

bool DfdFileDescriptor::rewriteRawFile(const QVector<QPair<int,int> > &indexesVector)
{
    if (indexesVector.isEmpty()) return false;

    // переписываем файл данных во временный файл, которым затем подменим исходный raw
    QTemporaryFile tempFile;
    QFile rawFile(rawFileName);

    if (tempFile.open() && rawFile.open(QFile::ReadOnly)) {
        //работаем через временный файл - читаем один канал и записываем его во временный файл
        if (!channels[0]->dataPositions.isEmpty()) {
//            qDebug()<<"- используем dataPositions";
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

                // меняем размер блока и обновляем положение данных,
                //если перезаписываем файл поверх самого себя
                ch->dataPositions.clear();
                ch->dataPositions.append(dataPos);
                ch->ChanBlockSize = NumInd;
            }
        }
        else {
//            qDebug()<<"- dataPositions отсутствуют";

            // trying to map file into memory
            unsigned char *ptr = rawFile.map(0, rawFile.size());
            if (ptr) {//достаточно памяти отобразить весь файл
//                qDebug()<<"-- работаем через ptr";

                for (int ind = 0; ind < indexesVector.size(); ++ind) {
                    // пропускаем канал, предназначенный для удаления
                    if (indexesVector.at(ind).second == 1) continue; // 0 = keep, 1 = delete

                    DfdChannel *ch = channels[indexesVector.at(ind).first];
                    qint64 dataPos = tempFile.pos();
                    const int bytes = ch->IndType % 16;
                    for (int i=0; i < NumInd; ++i) {
                        /*
                        * i-й отсчет n-го канала имеет номер
                        * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                        *
                        * если BlockSize=1 или ChanBlockSize=1, то
                        * n + i*ChannelsCount
                        */
                        unsigned char *ptrCurrent = ptr;
                        if (ch->ChanBlockSize == 1)
                            ptrCurrent = ptr + (indexesVector.at(ind).first + i*channelsCount()) * bytes;
                        else
                            ptrCurrent = ptr + (indexesVector.at(ind).first * ch->ChanBlockSize
                                                + (i/ch->ChanBlockSize) * ch->ChanBlockSize * channelsCount()
                                                + (i % ch->ChanBlockSize)) * bytes;
                        tempFile.write((char*)(ptrCurrent), bytes);
                    }
                    tempFile.flush();

                    // меняем размер блока и обновляем положение данных
                    ch->dataPositions.clear();
                    ch->dataPositions.append(dataPos);
                    ch->ChanBlockSize = NumInd;
                }
                rawFile.unmap(ptr);
            }
            else {
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
                        quint64 size = channels[indexesVector.at(ind).first]->blockSizeInBytes();
                        tempFile.write(temp.mid(indexesVector.at(ind).first * size, size));
                    }

                    if (actuallyRead == 0)
                        break;
                }
                for (int ind = 0; ind < channels.size(); ++ind) {
                    DfdChannel *ch = channels[ind];
                    ch->ChanBlockSize = NumInd;
                    ch->dataPositions.clear();
                    ch->dataPositions.append(ind * ch->blockSizeInBytes());
                }
            }
        }

        rawFile.close();
        tempFile.close();

        // удаляем файл данных, если мы перезаписывали его
        if (rawFile.remove()) {
            if (tempFile.copy(rawFileName)) {
                BlockSize = 0;
                return true;
            }
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
    if (d)
        return (this->DataType == d->DataType);
    return (dataTypefromDfdDataType(DataType) == other->type());
}

bool DfdFileDescriptor::canTakeChannelsFrom(FileDescriptor *other) const
{
    //типы совпадают строго -> принимаем
    const bool canTake = FileDescriptor::canTakeChannelsFrom(other);
    if (canTake) return true;

    DfdFileDescriptor *d = dynamic_cast<DfdFileDescriptor *>(other);
    if (!d) return canTake;

    if (xStep() != other->xStep()) return false;

    if (d->DataType == SourceData && DataType > 0 && DataType < 16) {
        return true;
    }

    return false;
}

QStringList DfdFileDescriptor::fileFilters()
{DD;
    return QStringList()<< "Файлы dfd (*.dfd)" << "Файлы dfd (*.DFD)";
}

bool DfdFileDescriptor::setDateTime(QDateTime dt)
{
    if (Date == dt.date() && Time == dt.time()) return false;

    Date = dt.date();
    Time = dt.time();
    setChanged(true);
    return true;
//    write();
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

QStringList DfdFileDescriptor::suffixes()
{
    return QStringList()<<"*.dfd"<<"*.DFD";
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
      channelIndex(channelIndex)
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
    IndType = 0xc0000004; // float
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
        _data->setXValues(parent->XBegin, XStep, NumInd);
    }

    auto yValueFormat = dataFormat();

    if (YName.toLower()=="db" || YName.toLower()=="дб")
        yValueFormat = DataHolder::YValuesAmplitudesInDB;
    //настройка для октавных спектров - если единица измерения не дБ, то считаем как амплитуды
    if ((parent->DataType >= OSpectr) && YName.toLower() != "db" && YName.toLower() != "дб")
        yValueFormat = DataHolder::YValuesAmplitudes;

    _data->setYValuesFormat(yValueFormat);

    double thr = 1.0;
    QString thrString = dfd.value(group+"threshold");
    if (thrString.isEmpty()) {
        if (YName.isEmpty() || YName.toLower() == "дб" || YName.toLower() == "db")
            thr = threshold(YNameOld);
        else
            thr = threshold(YName);
        if (type()==Descriptor::FrequencyResponseFunction) thr=1.0;
    }
    else {
        thr = thrString.toDouble();
    }
    _data->setThreshold(thr);

    int units = DataHolder::UnitsLinear;
    QString unitsStr = dfd.value(group+"units");
    if (unitsStr.isEmpty()) {
        if (dataType == Spectr || dataType == XSpectr) units = DataHolder::UnitsQuadratic;
    }
    else {
        if (unitsStr == "linear") units = DataHolder::UnitsLinear;
        else if (unitsStr == "quadratic") units = DataHolder::UnitsQuadratic;
        else if (unitsStr == "dimensionless") units = DataHolder::UnitsDimensionless;
        else if (unitsStr == "unknown") units = DataHolder::UnitsUnknown;
    }
    _data->setYValuesUnits(units);

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

    dfd << "threshold=" << QString::number(data()->threshold()) << endl;
    switch (data()->yValuesUnits()) {
        case DataHolder::UnitsLinear: dfd << "units=" << "linear" << endl; break;
        case DataHolder::UnitsQuadratic: dfd << "units=" << "quadratic" << endl; break;
        case DataHolder::UnitsDimensionless: dfd << "units=" << "dimensionless" << endl; break;
        case DataHolder::UnitsUnknown: dfd << "units=" << "unknown" << endl; break;
    }

}

QVariant DfdChannel::info(int column, bool edit) const
{
    switch (column) {
        case 0: return ChanName; //name(); //avoiding conversion variant->string->variant
        case 1: {
            QString result = YName;
            if (edit) return result;
            if (!YNameOld.isEmpty() && YNameOld != YName)
                result.append(QString(" (%1)").arg(YNameOld));
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

//void DfdChannel::populate()
//{DD;
//    // clear previous data;
//    _data->clear();

//    double thr = 1.0;
//    if (YName.isEmpty() || YName.toLower() == "дб" || YName.toLower() == "db")
//        thr = threshold(YNameOld);
//    else
//        thr = threshold(YName);
//    if (type()==Descriptor::FrequencyResponseFunction) thr=1.0;
//    _data->setThreshold(thr);

//    int units = DataHolder::UnitsLinear;
//    if (dataType == Spectr || dataType == XSpectr) units = DataHolder::UnitsQuadratic;
//    _data->setYValuesUnits(units);

////    int yValueFormat = dataFormat();

////    if (YName.toLower()=="db" || YName.toLower()=="дб")
////        yValueFormat = DataHolder::YValuesAmplitudesInDB;

//    QFile rawFile(parent->rawFileName);

//    QTime time;
//    time.start();

//    if (rawFile.open(QFile::ReadOnly)) {
//        /* Сложная система оптимизаций чтения.
//         * Самый распространенный формат временных реализаций - IndType=2 (quint16)
//         * Или IndType = 3221225476 (32-bit float)
//         * Методы чтения идут в порядке убывания оптимальности
//         * - самый быстрый - когда мы знаем положение всех блоков данных
//         *   и отображаем файл в память.
//         * - медленнее - читать из файла в память и интерпретировать каждый отсчет
//         *
//         */

//        QVector<double> YValues;
//        const quint64 blockSizeBytes = blockSizeInBytes();
//        const int channelsCount = parent->channelsCount();

//        if (IndType == 2) {//целое без знака, quint16
//            if (!dataPositions.isEmpty()) {
//                // уже знаем позиции всех данных
//                // читаем или через map, или оптимизированным способом

//                // map file into memory
//                unsigned char *ptr = rawFile.map(0, rawFile.size());
//                if (ptr) {//достаточно памяти отобразить весь файл
//                    qDebug()<<"IndType=2, mapped, by dataPositions";
//                    unsigned char *maxPtr = ptr + rawFile.size();
//                    unsigned char *ptrCurrent = ptr;

//                    foreach (int pos, dataPositions) {
//                        ptrCurrent = ptr + pos;
//                        QVector<double> temp = convertFromUINT16<double>(ptrCurrent, qMin(maxPtr-ptrCurrent, int(blockSizeBytes)), IndType);
//                        YValues << temp;
//                    }
//                }
//                else {//оптимизированно через qToLittleEndian
//                    qDebug()<<"IndType=2, optimised, by dataPositions";
//                    foreach (int pos, dataPositions) {
//                        rawFile.seek(pos);
//                        QVector<double> temp;

//                        QByteArray b = rawFile.read(blockSizeBytes);
//                        uchar *ptr = reinterpret_cast<uchar*>(b.data());
//                        int length = b.size();
//                        temp = convertFromUINT16<double>(ptr, length, IndType);

//                        YValues << temp;
//                    }
//                }
//            } /// end !dataPositions.isEmpty()

//            else if (ChanBlockSize != 1) {//блоки достаточно длинные, чтобы мы могли читать в вектор
//                // Этот блок кода никогда не будет вызван,
//                // так как dataPositions заполняются всегда, если ChanBlockSize != 1

//                if (parent->BlockSize == 0) {// без перекрытия, читаем подряд весь канал
//                    qDebug()<<"IndType=2, blockSize=0, skipping";
//                    // сложная схема пропуска данных на тот случай, если в каналах разное число отсчетов
//                    for (int i=0; i<channelsCount; ++i) {
//                        if (i==channelIndex) {
//                            QByteArray b = rawFile.read(blockSizeBytes);
//                            uchar *ptr = reinterpret_cast<uchar*>(b.data());
//                            int length = b.size();
//                            YValues = convertFromUINT16<double>(ptr, length, IndType);
//                        }
//                        else {
//                            rawFile.skip(parent->dfdChannel(i)->blockSizeInBytes());
//                        }
//                    }
//                } /// end parent->BlockSize == 0
//                else {// с перекрытием, читаем классическим способом
//                    qDebug()<<"IndType=2, с перекрытием, skipping";
//                    //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
//                    // если каналы имеют разный размер блоков, этот метод даст сбой
//                    int actuallyRead = 0;
//                    const int chunkSize = channelsCount * ChanBlockSize;

//                    QDataStream readStream(&rawFile);
//                    readStream.setByteOrder(QDataStream::LittleEndian);

//                    while (1) {
//                        /*
//                        * i-й отсчет n-го канала имеет номер
//                        * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
//                        */
//                        QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

//                        //распихиваем данные по каналам
//                        actuallyRead /= channelsCount;
//                        YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

//                        if (actuallyRead < ChanBlockSize)
//                            break;
//                    }
//                } /// end parent->BlockSize != 0
//            } /// end ChanBlockSize != 1
//            else {//ChanBlockSize == 1
//                // trying to map file into memory
//                unsigned char *ptr = rawFile.map(0, rawFile.size());
//                if (ptr) {//достаточно памяти отобразить весь файл
//                    unsigned char *ptrCurrent = ptr;
//                    qDebug()<<"IndType="<<IndType<<", ChanBlockSize == 1, mapped";
//                    YValues.resize(qMin(parent->NumInd, int(rawFile.size() / channelsCount / (IndType % 16))));
//                    for (int i=0; i < YValues.size(); ++i) {
//                        //
//                        // i-й отсчет n-го канала имеет номер
//                        // n + i*ChannelsCount
//                        //
//                        ptrCurrent = ptr + (channelIndex + i*channelsCount) * (IndType % 16);
//                        YValues[i] = static_cast<double>(qFromLittleEndian<quint16>(ptrCurrent));
//                    }
//                }
//                else {// не удалось прочитать никакими оптимальными методами, читаем медленным классическим
//                    int actuallyRead = 0;
//                    const int chunkSize = channelsCount; //ChanBlockSize = 1

//                    QDataStream readStream(&rawFile);
//                    readStream.setByteOrder(QDataStream::LittleEndian);

//                    while (1) {
//                        /*
//                        * i-й отсчет n-го канала имеет номер
//                        * n + i*ChannelsCount
//                        */
//                        QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

//                        //распихиваем данные по каналам
//                        actuallyRead /= chunkSize;
//                        YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

//                        if (actuallyRead < ChanBlockSize)
//                            break;
//                    }
//                }
//            }/// end ChanBlockSize == 1
//        } /// end IndType == 2
//        else {//все остальные форматы,
//            //читаем классическим способом через getChunk
//            qDebug()<<"IndType="<<IndType;
//            QDataStream readStream(&rawFile);
//            readStream.setByteOrder(QDataStream::LittleEndian);
//            if (IndType==0xC0000004)
//                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

//            //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
//            // если каналы имеют разный размер блоков, этот метод даст сбой
//            int actuallyRead = 0;
//            const int chunkSize = channelsCount * ChanBlockSize;
//            while (1) {
//                /*
//                * i-й отсчет n-го канала имеет номер
//                * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
//                */
//                QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

//                //распихиваем данные по каналам
//                actuallyRead /= channelsCount;
//                YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

//                if (actuallyRead < ChanBlockSize)
//                    break;
//            }
//        } /// end IndType != 2

//        YValues.resize(parent->NumInd);
//        postprocess(YValues);
//        _data->setYValues(YValues, DataHolder::YValuesFormat(_data->yValuesFormat()));
//        setPopulated(true);

//        if (parent->DataType == ToSpectr || parent->DataType == OSpectr) {//данные по оси Х хранятся первым каналом
//            if (!parent->xValues.isEmpty()) {
//                _data->setXValues(parent->xValues);
//            }
//            else {
//                QDataStream readStream(&rawFile);
//                readStream.setByteOrder(QDataStream::LittleEndian);
//                if (IndType==0xC0000004)
//                    readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
//                rawFile.seek(0);
//                readStream.setDevice(&rawFile);
//                // читаем без перекрытия, предполагаем, что тип файла - третьоктава или октава
//                QVector<double> temp = getChunkOfData<double>(readStream, ChanBlockSize, IndType);
//                //checking if values are really frequency values, take four first values
//                parent->xValues = temp;
//                _data->setXValues(temp);
//                //                if (temp.size()>=4) {
//                //                    QVector<double> xv(4);
//                //                    for (int k = 0; k<4; ++k)
//                //                        xv[k] = pow(10.0, 0.1*k);
//                //                    if (qAbs(temp[0]-xv[0])<1e-4
//                //                        && qAbs(temp[1]-xv[1])<1e-4
//                //                        && qAbs(temp[2]-xv[2])<1e-4
//                //                        && qAbs(temp[3]-xv[3])<1e-4) {
//                //                        setXValues(temp);
//                //                    }
//                //                    else if (parent->DataType == ToSpectr) {
//                //                        QVector<double> xv(YValues.size());
//                //                        for (int k = 0; k<YValues.size(); ++k)
//                //                            xv[k] = pow(10.0, 0.1*k);
//                //                        setXValues(xv);
//                //                    }
//                //                }
//            }
//        }
//    }
//    else {
//        qDebug()<<"Cannot read raw file"<<parent->rawFileName;
//    }
//    qDebug()<<"reading finished"<<time.elapsed();
//}

void DfdChannel::populate()
{DD;
//    QElapsedTimer timer;  timer.start();
    // clear previous data;
    _data->clear();

    QFile rawFile(parent->rawFileName);

    if (rawFile.open(QFile::ReadOnly)) {
        QVector<double> YValues;
        const quint64 blockSizeBytes = blockSizeInBytes();
        const int channelsCount = parent->channelsCount();

        // map file into memory
        unsigned char *ptr = rawFile.map(0, rawFile.size());
        if (ptr) {//достаточно памяти отобразить весь файл
            unsigned char *maxPtr = ptr + rawFile.size();
            unsigned char *ptrCurrent = ptr;
            if (!dataPositions.isEmpty()) {
                //qDebug()<<"IndType="<<IndType<<", mapped, by dataPositions";
                foreach (int pos, dataPositions) {
                    ptrCurrent = ptr + pos;
                    QVector<double> temp = convertFrom<double>(ptrCurrent, qMin(maxPtr-ptrCurrent, int(blockSizeBytes)), IndType);
                    YValues << temp;
                }
            } ///!dataPositions.isEmpty()
            else {
                //qDebug()<<"IndType="<<IndType<<"blocksize"<<parent->BlockSize<<", mapped, skipping";
                /*
                * i-й отсчет n-го канала имеет номер
                * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                *
                * если BlockSize=1 или ChanBlockSize=1, то
                * n + i*ChannelsCount
                */
                YValues.resize(parent->NumInd);
                for (int i=0; i < YValues.size(); ++i) {
                    if (ChanBlockSize == 1)
                        ptrCurrent = ptr + (channelIndex + i*channelsCount) * (IndType % 16);
                    else
                        ptrCurrent = ptr + (channelIndex*ChanBlockSize
                                            + (i/ChanBlockSize)*ChanBlockSize*channelsCount
                                            + (i % ChanBlockSize)) * (IndType % 16);
                    YValues[i] = convertFrom<double>(ptrCurrent, IndType);
                }
            } /// dataPositions.isEmpty()
        } /// mapped
        else {
            //читаем классическим способом через getChunk
            //qDebug()<<"IndType="<<IndType<<"BlockSize="<<parent->BlockSize<<"not mapped";
            QDataStream readStream(&rawFile);
            readStream.setByteOrder(QDataStream::LittleEndian);
            if (IndType==0xC0000004)
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
            // если каналы имеют разный размер блоков, этот метод даст сбой
            int actuallyRead = 0;
            const int chunkSize = channelsCount * ChanBlockSize;
            while (1) {
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
        _data->setYValues(YValues, _data->yValuesFormat());
        setPopulated(true);
        rawFile.close();

        if (!parent->xValues.isEmpty()) {
            _data->setXValues(parent->xValues);
        }
        else {
            if (_data->xValuesFormat() == DataHolder::XValuesNonUniform) {
                if (channelIndex == 0) {
                    //это канал со значениями X, мы только что его прочитали
                    parent->xValues = YValues;
                    _data->setXValues(YValues);
                }
                else {//unlikely to run
                    QDataStream readStream(&rawFile);
                    readStream.setByteOrder(QDataStream::LittleEndian);
                    if (IndType==0xC0000004)
                        readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
                    rawFile.seek(0);
                    readStream.setDevice(&rawFile);
                    // читаем без перекрытия, предполагаем, что тип файла - третьоктава или октава
                    QVector<double> temp = getChunkOfData<double>(readStream, ChanBlockSize, IndType);
                    parent->xValues = temp;
                    _data->setXValues(temp);
                }
            }
        }
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->rawFileName;
    }
//    qDebug()<<"reading finished"<<timer.elapsed();
}

quint64 DfdChannel::blockSizeInBytes() const
{
    return quint64(ChanBlockSize * (IndType % 16));
}

FileDescriptor *DfdChannel::descriptor()
{
    return parent;
}

DataHolder::YValuesFormat DfdChannel::dataFormat() const
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
            return DataHolder::YValuesReals;
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
        case TwoOSpectr:
        case SixOSpectr:
        case TwlOSpectr:
        case TFOSpectr:
            return DataHolder::YValuesAmplitudesInDB;
        default:
            break;
    }
    return DataHolder::YValuesReals;
}

void DfdChannel::appendDataTo(const QString &rawFileName)
{
    if (!populated()) {
        qDebug()<<"Попытка записать канал без данных";
        return;
    }
    if (parent->BlockSize != 0) {
        qDebug()<<"Попытка записать канал в файл с перекрытием";
        return;
    }

    QFile rawFile(rawFileName);
    if (rawFile.open(QFile::Append)) {
        QDataStream writeStream(&rawFile);
        writeStream.setByteOrder(QDataStream::LittleEndian);

        if (IndType==0xC0000004)
            writeStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        const int sc = parent->NumInd;

        QVector<double> yValues = data()->rawYValues();
        if (yValues.isEmpty() && !data()->yValuesComplex().isEmpty()) {
            //комплексные данные, dfd не умеет их переваривать, конвертируем в линейные
            yValues = data()->linears();
            data()->setYValuesFormat(DataHolder::YValuesAmplitudes);
        }
        yValues.resize(sc);
        for (int val = 0; val < sc; ++val) {
            setValue(yValues[val], writeStream);
        }
    }
    else {
        qDebug()<<"Не могу открыть файл для дозаписи:"<<rawFileName;
        return;
    }
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

int DfdChannel::octaveType() const
{
    switch (dataType) {
        case ToSpectr: return 3;
        case OSpectr: return 1;
        case TwoOSpectr: return 2;
        case SixOSpectr: return 6;
        case TwlOSpectr: return 12;
        case TFOSpectr: return 24;
        default: return 0;
    }
    return 0;
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

QString DfdChannel::zName() const
{
    return "";
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
//qDebug()<<parent->fileName();
    ADC0 = hextodouble( dfd.value(group+"ADC0")); //qDebug()<<"ADC0" << ADC0;
    ADCStep = hextodouble(dfd.value(group+"ADCStep")); //qDebug()<< "ADCStep"<< ADCStep;
    AmplShift = hextodouble(dfd.value(group+"AmplShift")); //qDebug()<<"AmplShift" <<AmplShift ;
    AmplLevel = hextodouble(dfd.value(group+"AmplLevel")); //qDebug()<< "AmplLevel"<<AmplLevel ;
    Sens0Shift = hextodouble(dfd.value(group+"Sens0Shift")); //qDebug()<< "Sens0Shift"<<Sens0Shift ;
    SensSensitivity = hextodouble(dfd.value(group+"SensSensitivity")); //qDebug()<< "SensSensitivity"<<SensSensitivity;
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

QVariant RawChannel::info(int column, bool edit) const
{
    if (column >= 0 && column < DfdChannel::columnsCount())
        return DfdChannel::info(column, edit);
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
    return DfdChannel::columnsCount() + 8;
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
        case XSpectr:
        case XSpectrRe:
        case XSpectrIm:
        case XPhase:
            return Descriptor::CrossSpectrum;
        case Coherence: return Descriptor::Coherence;
        case TransFunc:
        case TrFuncIm:
            return Descriptor::FrequencyResponseFunction;
        case TrFuncRe: return Descriptor::Transmissibility;
        case ToSpectr:
        case OSpectr:
        case TwoOSpectr:
        case SixOSpectr:
        case TwlOSpectr:
        case TFOSpectr: return Descriptor::Spectrum;
        default: return Descriptor::Unknown;
    }

    return Descriptor::Unknown;
}


QString DfdFileDescriptor::saveTimeSegment(double from, double to)
{
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (DataType >= Envelope) return QString();

//    populate();
    const int count = channelsCount();

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString newFileName = createUniqueFileName("", fileName(), suffix, "dfd", false);

    // 2 создаем новый файл
    DfdFileDescriptor *newDfd = DfdFileDescriptor::newFile(newFileName, CuttedData);
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
    list.append(qMakePair(QStringLiteral("pName"), QStringLiteral("Осциллограф")));
    list.append(qMakePair(QStringLiteral("pTime"),QStringLiteral("(0000000000000000)")));
    list.append(qMakePair(QStringLiteral("ProcChansList"), procChansList.join(",")));
    list.append(qMakePair(QStringLiteral("BlockIn"), QString::number(newDfd->samplesCount())));
    list.append(qMakePair(QStringLiteral("TypeProc"), QStringLiteral("0")));
    list.append(qMakePair(QStringLiteral("NAver"), QStringLiteral("1")));
    list.append(qMakePair(QStringLiteral("Values"), QStringLiteral("измеряемые")));
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
        ch->appendDataTo(newDfd->rawFileName);

        newDfd->channels << ch;
        if (!wasPopulated) {
            //clearing data
            channels[i]->data()->clear();
        }
    }

    newDfd->setSamplesCount(newDfd->channel(0)->samplesCount());
    newDfd->setChanged(true);
    newDfd->write();
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

int DfdChannel::index() const
{
    return channelIndex;
}


