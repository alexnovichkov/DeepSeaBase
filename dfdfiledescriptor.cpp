#include "dfdfiledescriptor.h"
#include <QtCore>

#include "converters.h"
#include <QtWidgets>
#include <QUuid>

#include "logging.h"

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

//PlotType plotTypeByDataType(DfdDataType dataType)
//{
//    if ((dataType>=0 && dataType<=31) || dataType == 153) return PlotTime;
//    if (dataType>=32 && dataType<=33) return PlotCorrelation;
//    if (dataType>=64 && dataType<=127) return PlotStatistics;
//    if (dataType>=128 && dataType<=151) return PlotSpectre;
//    if (dataType==156 || dataType==157) return PlotOctave;
//    if (dataType==152 || dataType==154) return PlotNykvist;
//    return PlotUnknown;
//}

bool readDfdFile(QIODevice &device, QSettings::SettingsMap &map)
{DD;
    char buf[2048];
    qint64 lineLength;
    QString group;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    while ((lineLength = device.readLine(buf, sizeof(buf))) != -1) {
        // the line is available in buf
        QByteArray b = QByteArray(buf, lineLength);
        if (!b.isEmpty()) {
            QString s = codec->toUnicode(b);
            s = s.trimmed();
            if (s.startsWith('[')) {
                s.chop(1);
                s.remove(0,1);
                group = s;
            }
            else {
                int ind = s.indexOf('=');
                if (ind>0) {
                    QString key = group+"/"+s.mid(0,ind);
                    QString val = s.mid(ind+1);
                    map.insert(key,val);
                }
            }
        }
    }
    return true;
}

bool writeDfdFile(QIODevice &/*device*/, const QSettings::SettingsMap &/*map*/)
{DD;
    return true;
}



DfdFileDescriptor::DfdFileDescriptor(const QString &fileName)
    : FileDescriptor(fileName),
      DataType(NotDef),
      NumChans(0),
      BlockSize(0),
      source(0),
      process(0),
      dataDescription(0)
{DD;
//    rawFileChanged = false;
//    changed = false;
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

void DfdFileDescriptor::read()
{DD;
    static const QSettings::Format dfdFormat = QSettings::registerFormat("DFD", readDfdFile, writeDfdFile);

    QSettings dfd(fileName(),dfdFormat);

    QStringList childGroups = dfd.childGroups();

    //[DataFileDescriptor]
    dfd.beginGroup("DataFileDescriptor");
    rawFileName = dfd.value("DataReference").toString();
    if (rawFileName.isEmpty())
        rawFileName = fileName().left(fileName().length()-4)+".raw";
    DFDGUID =   dfd.value("DFDGUID").toString();
    DataType =  DfdDataType(dfd.value("DataType").toInt());
    Date =      QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
    Time =      QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
    NumChans =  dfd.value("NumChans").toInt(); //DebugPrint(NumChans);
    setSamplesCount(dfd.value("NumInd").toUInt());      //DebugPrint(NumInd);
    BlockSize = dfd.value("BlockSize").toInt();   //DebugPrint(BlockSize);
    XName = dfd.value("XName").toString();  //DebugPrint(XName);
    setXBegin(hextodouble(dfd.value("XBegin").toString()));   //DebugPrint(XBegin);
    XStep = hextodouble(dfd.value("XStep").toString()); //DebugPrint(XStep);
    DescriptionFormat = dfd.value("DescriptionFormat").toString(); //DebugPrint(DescriptionFormat);
    CreatedBy = dfd.value("CreatedBy").toString(); //DebugPrint(CreatedBy);
    dfd.endGroup();

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
    for (quint32 i=0; i<NumChans; ++i) {
        DfdChannel *ch = newChannel(i);
        ch->read(dfd, i+1);
    }
}

void DfdFileDescriptor::write()
{DD;
    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");

    QFile file(fileName());
    if (!file.open(QFile::WriteOnly | QFile::Text)) return;
    QTextStream dfd(&file);
    dfd.setCodec(codec);

    // убираем перекрытие блоков, если пишем не сырые данные
    if (type() != Descriptor::TimeResponse)
        BlockSize = 0;

    /** [DataFileDescriptor]*/
    dfd << "[DataFileDescriptor]" << endl;
    dfd << "DFDGUID="<<DFDGUID << endl;
    dfd << "DataType="<<DataType << endl;
    dfd << "Date="<<Date.toString("dd.MM.yyyy") << endl;
    dfd << "Time="<<Time.toString("hh:mm:ss") << endl;
    dfd << "NumChans="<<NumChans << endl;
    dfd << "NumInd="<<samplesCount() << endl;
    dfd << "BlockSize="<<BlockSize << endl;
    dfd << "XName="<<xName() << endl;
    dfd << "XBegin="<<doubletohex(xBegin()).toUpper() << endl;
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

    /** Channels*/
    for (int i=0; i<channels.size(); ++i) {
        DfdChannel *ch = channels[i];
        ch->write(dfd, i+1);
    }
    setChanged(false);
}

void DfdFileDescriptor::writeRawFile()
{DD;
    //be sure all channels were read. May take too much memory
    for(int i = 0; i< channels.size(); ++i) {
        if (!channels[i]->populated()) channels[i]->populate();
    }


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

                for (quint32 val = 0; val < ch->samplesCount(); ++val) {
                    ch->setValue(ch->corrected?ch->YValues[val]-ch->oldCorrectionValue:ch->YValues[val], writeStream);
                }
            }
        }
        else {
            // пишем блоками размером BlockSize
            qDebug() << "Oops! Пытаемся писать с перекрытием?";
        }
    }
    else qDebug()<<"Cannot open file"<<rawFileName<<"to write";
    setDataChanged(false);
}

void DfdFileDescriptor::populate()
{DD;
    for(int i = 0; i < channels.size(); ++i) {
        if (!channels[i]->populated()) channels[i]->populate();
    }
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
}

void DfdFileDescriptor::fillRest()
{DD;
    if (channels.isEmpty()) return;

    setSamplesCount(channels.first()->samplesCount());
    BlockSize = 0;
    XName = channels.first()->xName();  //DebugPrint(XName);
    setXBegin(channels.first()->xBegin());   //DebugPrint(XBegin);
    XStep = channels.first()->XStep;
}

QStringList DfdFileDescriptor::info() const
{DD;
    QStringList  list;
    list << QFileInfo(fileName()).completeBaseName() //QString("Файл") 1
         << QDateTime(Date, Time).toString("dd.MM.yy hh:mm:ss") // QString("Дата") 2
         << dataTypeDescription(DataType) // QString("Тип") 3
         << QString::number(samplesCount() * xStep()) // QString("Размер") 4
         << xName() // QString("Ось Х") 5
         << QString::number(xStep()) // QString("Шаг") 6
         << QString::number(channelsCount()) // QString("Каналы")); 7
         << dataDescriptorAsString()
         << legend();
    return list;
}

QString DfdFileDescriptor::dateTime() const
{DD;
    return QDateTime(Date, Time).toString("dd.MM.yy hh:mm:ss");
}

Descriptor::DataType DfdFileDescriptor::type() const
{DD;
    return dataTypefromDfdDataType(DataType);
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

        dataDescription->data = data;
        setChanged(true);
        write();
    }
}

void DfdFileDescriptor::setLegend(const QString &legend)
{DD;
    if (legend == _legend) return;
    _legend = legend;
    setChanged(true);
    if (!dataDescription)
        dataDescription = new DataDescription(this);
    write();
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
    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    setChanged(true);
    setDataChanged(true);
    NumChans = channels.size();

    for (quint32 i=0; i<NumChans; ++i) {
        channels[i]->channelIndex = i;
    }

    write();
    writeRawFile();
}



void DfdFileDescriptor::copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy)
{DD;
    //заполняем данными файл, куда будем копирвоать каналы
    //читаем все каналы, чтобы сохранить файл полностью
    for(int i = 0; i < channels.size(); ++i) {
        if (!channels[i]->populated())
            channels[i]->populate();
    }
    QList<FileDescriptor*> records;
    for (int i=0; i<channelsToCopy.size(); ++i)
        if (!records.contains(channelsToCopy.at(i).first)) records << channelsToCopy.at(i).first;

    foreach (FileDescriptor *newDfd, records) {
        DfdFileDescriptor *dfd = static_cast<DfdFileDescriptor *>(newDfd);
        QList<int> channelsIndexes = filterIndexes(newDfd, channelsToCopy);

        for(int i = 0; i < newDfd->channelsCount(); ++i) {
            if (!newDfd->channel(i)->populated() && channelsIndexes.contains(i))
                newDfd->channel(i)->populate();
        }

        //добавляем в файл dfd копируемые каналы из dfdRecord
        foreach (int index, channelsIndexes) {
            if (dfd) {
                channels.append(new DfdChannel(*dfd->channels[index]));
            }
            else {
                channels.append(new DfdChannel(*newDfd->channel(index)));
            }
        }
    }
    NumChans = channels.count();

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

void DfdFileDescriptor::calculateMean(const QMultiHash<FileDescriptor *, int> &channels)
{DD;
    for(int i = 0; i< this->channels.size(); ++i) {
        if (!this->channels[i]->populated()) this->channels[i]->populate();
    }

    DfdChannel *ch = new DfdChannel(this, channelsCount());

    FileDescriptor *firstDescriptor = channels.keys().first();
    Channel *firstChannel = firstDescriptor->channel(channels.value(firstDescriptor));

    if (ch) {
        QList<Channel*> list;
        QHashIterator<FileDescriptor *, int> it(channels);
        while (it.hasNext()) {
            it.next();
            list << it.key()->channel(it.value());
        }

        // считаем данные для этого канала
        // если ось = дБ, сначала переводим значение в линейное

        //ищем наименьшее число отсчетов
        quint32 numInd = list.first()->samplesCount();
        for (int i=1; i<list.size(); ++i) {
            if (list.at(i)->samplesCount() < numInd)
                numInd = list.at(i)->samplesCount();
        }

        ch->YValues = QVector<double>(numInd, 0.0);

        ch->yMin = 1.0e100;
        ch->yMax = -1.0e100;

        for (quint32 i=0; i<numInd; ++i) {
            double sum = 0.0;
            for (int file = 0; file < list.size(); ++file) {
                double temp = list[file]->yValues()[i];
                if (list[file]->yName() == "дБ" || list[file]->yName() == "dB")
                    temp = pow(10.0, (temp/10.0));
                sum += temp;
            }
            sum /= list.size();
            if (list[0]->yName() == "дБ" || list[0]->yName() == "dB")
                sum = 10.0 * log10(sum);
            if (sum < ch->yMin) ch->yMin = sum;
            if (sum > ch->yMax) ch->yMax = sum;
            ch->YValues[i] = sum;
        }

        // обновляем сведения канала
        ch->setPopulated(true);
        QStringList l;
        foreach (Channel *c,list) {
            l << c->name();
        }

        ch->ChanName = "Среднее "+l.join(", ");

        QList<int> ll;
        l.clear();
        it.toFront();
        while (it.hasNext()) {
            it.next();
            ll << it.value()+1;
        }
        qSort(ll);
        foreach(int n,ll) l << QString::number(n);
        ch->ChanDscr = "Среднее каналов "+l.join(",");

        ch->ChanBlockSize = numInd;
        ch->NumInd = numInd;
        ch->IndType = this->channels.isEmpty()?3221225476:this->channels.first()->IndType;

        ch->YName = firstChannel->yName();
        ch->XName = firstChannel->xName();
        ch->XStep = firstChannel->xStep();

        ch->xMin = firstChannel->xBegin();
        ch->xMax = ch->xMin + ch->XStep * numInd;
        ch->XMaxInitial = ch->xMax;
        ch->YMinInitial = ch->yMin;
        ch->YMaxInitial = ch->yMax;

        ch->parent = this;

        this->channels << ch;
        this->NumChans++;
    }
}

void DfdFileDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    populate();
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
    writeRawFile();
}

QStringList DfdFileDescriptor::getHeadersForChannel(int channel)
{DD;
    return channels[channel]->getInfoHeaders();
}

Channel *DfdFileDescriptor::channel(int index)
{
    if (channels.size()>index) return channels[index];
    return 0;
}

bool DfdFileDescriptor::allUnplotted() const
{DD;
    foreach (DfdChannel *c, channels) {
        if (c->checkState() == Qt::Checked) return false;
    }
    return true;
}

bool DfdFileDescriptor::isSourceFile() const
{DD;
    return (DataType>=SourceData && DataType<=15);
}

bool DfdFileDescriptor::dataTypeEquals(FileDescriptor *other)
{DD;
    DfdFileDescriptor *d = dynamic_cast<DfdFileDescriptor *>(other);
    if (!d) return false;
    return (this->DataType == d->DataType);
}

QString DfdFileDescriptor::fileFilters() const
{DD;
    return QString("Файлы dfd (*.dfd)");
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
    QString result;
    result = QUuid::createUuid().toString().toUpper();
    if (result.at(24) == '-') result.remove(24,1);
    else result.remove(25,1);
    return result;
}

Process::Process()
{DD;

}

void Process::read(QSettings &dfd)
{DD;
    dfd.beginGroup("Process");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
        data.append(QPair<QString, QString>(key, v));
    }

    dfd.endGroup();
}

void Process::write(QTextStream &dfd)
{DD;
    dfd << "[Process]" << endl;
    for (int i = 0; i<data.size(); ++i) {
        QPair<QString, QString> item = data.at(i);
        dfd << item.first << "=" << item.second << endl;
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


void Source::read(QSettings &dfd)
{DD;
    dfd.beginGroup("Sources");
    sFile = dfd.value("sFile").toString();
    if (!sFile.isEmpty()) {
        //K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
        File = sFile.section("[",0,0);
        QStringList procChansList = sFile.section("[",1).section("]",0,0).split(",");
        foreach (QString s, procChansList) ProcChansList << s.toInt();
        DFDGUID = "{"+sFile.section("{",1);
        dfd.endGroup();
    }
    else {
        dfd.beginGroup("Source");
        File = dfd.value("File").toString();
        DFDGUID = dfd.value("DFDGUID").toString();
        Date=QDate::fromString(dfd.value("Date").toString(),"dd.MM.yyyy");
        Time=QTime::fromString(dfd.value("Time").toString(),"hh:mm:ss");
        dfd.endGroup();
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
      xMin(0.0),
      xMax(0.0),
      yMin(0.0),
      yMax(0.0),
      XStep(0.0),
      XMaxInitial(0.0),
      YMinInitial(0.0),
      YMaxInitial(0.0),
      //YValues(0),
      parent(parent),
      channelIndex(channelIndex),
      _populated(false)
{
    XName = parent->XName;
    dataType = parent->DataType;
    NumInd = 0;
    corrected = false;
    oldCorrectionValue = 0.0;
}

DfdChannel::DfdChannel(const DfdChannel &other)
{DD;
    ChanAddress = other.ChanAddress; //
    ChanName = other.ChanName;
    IndType = other.IndType;
    ChanBlockSize = other.ChanBlockSize; //размер блока в отсчетах
    YName = other.YName;
    YNameOld = other.YNameOld;
    InputType = other.InputType;
    ChanDscr = other.ChanDscr;
    NumInd = other.NumInd;

    XName = other.XName;
    xMin = other.xMin;
    xMax = other.xMax;
    yMin = other.yMin;
    yMax = other.yMax;
    XStep = other.XStep;
    XMaxInitial = other.XMaxInitial; // initial xMax value to display
    YMinInitial = other.YMinInitial; // initial yMin value to display
    YMaxInitial = other.YMaxInitial; // initial yMax value to display

    YValues = other.YValues;
//    YValues = new double[samplesCount()];
//    memcpy(static_cast<void *>(YValues), static_cast<void *>(other.YValues),
//           samplesCount()*sizeof(double));

    parent = other.parent;
    channelIndex = other.channelIndex;

    _populated = other._populated;
    dataType = other.dataType;
    corrected = other.corrected;
    oldCorrectionValue = other.oldCorrectionValue;
}

DfdChannel::DfdChannel(Channel &other)
{
    ChanAddress = "";
    ChanName = other.name();
    ChanDscr = other.description();
    IndType = 3221225476;
    ChanBlockSize = other.samplesCount();
    NumInd = other.samplesCount();

    YName = other.yName();
    XName = other.xName();

    xMin = other.xBegin();
    XStep = other.xStep();
    xMax = xMin + XStep * NumInd;
    yMin = other.yMinInitial();
    yMax = other.yMaxInitial();
    XMaxInitial = xMax;
    YMinInitial = yMin;
    YMaxInitial = yMax;

    YValues = other.yValues();
//    YValues = new double[NumInd];
//    memcpy(static_cast<void *>(YValues), static_cast<const void *>(other.yValues()),
//           NumInd*sizeof(double));

    _populated = true;
    dataType = dfdDataTypeFromDataType(other.type());
    corrected = false;
    oldCorrectionValue = 0.0;
}

DfdChannel::~DfdChannel()
{DD;
    //delete [] YValues;
}

void DfdChannel::read(QSettings &dfd, int chanIndex)
{DD;
    dfd.beginGroup(QString("Channel%1").arg(chanIndex));
    ChanAddress = dfd.value("ChanAddress").toString();
    ChanName = dfd.value("ChanName").toString();
    IndType = dfd.value("IndType").toUInt();
    ChanBlockSize = dfd.value("ChanBlockSize").toInt();

    YName = dfd.value("YName").toString();
    YNameOld = dfd.value("YNameOld").toString();
    InputType = dfd.value("InputType").toString();
    ChanDscr = dfd.value("ChanDscr").toString();
    dfd.endGroup();
    if (parent->BlockSize==0)
        NumInd = ChanBlockSize;
    else {
        NumInd = ChanBlockSize/parent->BlockSize;
        NumInd *= parent->samplesCount();
    }
    XStep = double(samplesCount()/parent->samplesCount());
    XStep *= parent->xStep();
}

void DfdChannel::write(QTextStream &dfd, int chanIndex)
{DD;
    dfd << QString("[Channel%1]").arg(chanIndex) << endl;
    dfd << "ChanAddress=" << ChanAddress << endl;
    if (corrected) {
        setName(nameBeforeCorrection);
    }
    dfd << "ChanName=" << ChanName << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << ChanBlockSize << endl;
    dfd << "YName=" << YName << endl;
    if (!YNameOld.isEmpty())
        dfd << "YNameOld=" << YNameOld << endl;
    dfd << "InputType="<<InputType << endl;
    dfd << "ChanDscr="<<ChanDscr << endl;
}

QStringList DfdChannel::getInfoHeaders()
{DD;
    return QStringList()
            //<< QString("Канал") //ChanAddress
            << QString("       Имя") //ChanName
            //<< QString("Вход") //InputType
            << QString("Ед.изм.") //YName
            << QString("Описание") //ChanDscr
               ;
}

QStringList DfdChannel::getInfoData()
{DD;
    return QStringList()
          //  << ChanAddress
            << ChanName
          //  << InputType
            << YName
            << ChanDscr
               ;
}

//void DfdChannel::populate()
//{DD;
//    // clear previous data;
//    YValues.clear();

//    QFile rawFile(parent->attachedFileName());
//    if (rawFile.open(QFile::ReadOnly)) {

//        // число отсчетов в канале
//        quint32 NI = samplesCount();

//        yMin = 1.0e100;
//        yMax = -1.0e100;

//        QDataStream readStream(&rawFile);
//        readStream.setByteOrder(QDataStream::LittleEndian);
//        if (IndType==0xC0000004)
//            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

//        while (NI>0) {
//            //read NumChans chunks
//            //так как размеры блока разных каналов могут не совпадать, приходится учитывать все каналы
//            for (quint32 i=0; i<parent->NumChans; ++i) {
//                if (i==channelIndex) {

//                    //QByteArray rawBlock = rawFile.read(parent->channels.at(i)->blockSizeInBytes());
//                    QVector<double> temp = getValue(readStream);

//                    for (int i=0; i<temp.size(); ++i) {
//                        if (temp[i] < yMin) yMin = temp[i];
//                        if (temp[i] > yMax) yMax = temp[i];
//                    }

//                    YValues << temp;
//                    NI -= ChanBlockSize;
//                }
//                else {
//                    readStream.skipRawData(parent->channels.at(i)->blockSizeInBytes());
//                }
//            }
//        }
//        xMin = parent->xBegin();
//        xMax = xMin + XStep * YValues.size();
//        XMaxInitial = xMax;
//        YMinInitial = yMin;
//        YMaxInitial = yMax;

//        setPopulated(true);
//    }
//    else {
//        qDebug()<<"Cannot read raw file"<<parent->attachedFileName();
//    }
//}

void DfdChannel::populate()
{DD;
    // clear previous data;
    YValues.clear();

    QFile rawFile(parent->attachedFileName());
    if (rawFile.open(QFile::ReadOnly)) {

        // число отсчетов в канале
        quint32 NI = samplesCount();

        yMin = 1.0e100;
        yMax = -1.0e100;

        QDataStream readStream(&rawFile);
        readStream.setByteOrder(QDataStream::LittleEndian);
        if (IndType==0xC0000004)
            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        if (parent->BlockSize == 0) {// без перекрытия, читаем подряд весь канал
            for (quint32 i=0; i<parent->NumChans; ++i) {
                if (i==channelIndex) {
                    QVector<double> temp = getValue(readStream, ChanBlockSize);

                    postprocess(temp);

                    for (int i=0; i<temp.size(); ++i) {
                        if (temp[i] < yMin) yMin = temp[i];
                        if (temp[i] > yMax) yMax = temp[i];
                    }

                    YValues = temp;
                }
                else {
                    readStream.skipRawData(parent->channels.at(i)->blockSizeInBytes());
                }
            }
            xMin = parent->xBegin();
            xMax = xMin + XStep * YValues.size();
            XMaxInitial = xMax;
            YMinInitial = yMin;
            YMaxInitial = yMax;

            setPopulated(true);
        }
        else {//с перекрытием, сначала читаем блок данных в 2048 отчетов для всех каналов
            // если каналы имеют разный размер блоков, этот метод даст сбой
            quint32 actuallyRead = 0;
            while (1) {
                quint32 chunkSize = parent->channelsCount() * ChanBlockSize;

                QVector<double> temp = getValue(readStream, chunkSize, &actuallyRead);

                //распихиваем данные по каналам
                actuallyRead /= parent->channelsCount();
                for (int i=0; i<parent->channelsCount();++i) {
                    //if (!parent->channel(i)->populated()) {
                    if (i == channelIndex) {
                        parent->channels[i]->YValues << temp.mid(actuallyRead*i, actuallyRead);
                    }
                }
                if (actuallyRead < ChanBlockSize) {
                    qDebug()<<parent->fileName()<<"read"<<actuallyRead <<"of"<< ChanBlockSize;
                    break;
                }
            }
            for (int i=0; i<parent->channelsCount();++i) {
                //if (!parent->channel(i)->populated()) {
                if (i == channelIndex) {
                    parent->channels[i]->postprocess(parent->channels[i]->YValues);
                    parent->channels[i]->xMin = parent->xBegin();
                    parent->channels[i]->xMax = parent->channels[i]->xMin
                                                + parent->channels[i]->XStep
                                                * parent->channels[i]->YValues.size();
                    parent->channels[i]->XMaxInitial = parent->channels[i]->xMax;
                    parent->channels[i]->YMinInitial = parent->channels[i]->yMin;
                    parent->channels[i]->YMaxInitial = parent->channels[i]->yMax;

                    parent->channels[i]->setPopulated(true);
                }
            }
        }
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->attachedFileName();
    }
}

void DfdChannel::clear()
{DD;
    YValues.clear();
    setPopulated(false);
}

quint32 DfdChannel::blockSizeInBytes() const
{
    return ChanBlockSize * (IndType % 16);
}

QString DfdChannel::legendName() const
{DD;
    QString result = parent->legend();
    if (!result.isEmpty()) result.prepend(" ");

    return (ChanName.isEmpty()?ChanAddress:ChanName) + result;
}

template <typename T>
QVector<double> readChunk(QDataStream &readStream, quint32 blockSize, quint32 *actuallyRead)
{
    QVector<double> result(blockSize);
    T v;

    if (actuallyRead) *actuallyRead = 0;

    for (quint32 i=0; i<blockSize; ++i) {
        if (readStream.atEnd()) {
            break;
        }

        readStream >> v;
        result[i] = double(v);
        if (actuallyRead) (*actuallyRead)++;
    }

    return result;
}

QVector<double> DfdChannel::getValue(QDataStream &readStream, quint32 chunkSize, quint32 *actuallyRead)
{
    QVector<double> result;

    switch (IndType) {
        case 0x00000001: {
            result = readChunk<quint8>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000001: {
            result = readChunk<qint8>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000002: {
            result = readChunk<quint16>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000002: {
            result = readChunk<qint16>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000004: {
            result = readChunk<quint32>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000004: {
            result = readChunk<qint32>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000008: {
            result = readChunk<qint64>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000004: {
            result = readChunk<float>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000008:
        case 0xC000000A:
            result = readChunk<double>(readStream, chunkSize, actuallyRead);
            break;
        default: break;
    }

    return result;
}

//double DfdChannel::getValue(QDataStream &readStream)
//{
//    double realValue = 0.0;
//    switch (IndType) {
//        case 0x00000001: {
//            quint8 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x80000001: {
//            qint8 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x00000002: {
//            quint16 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x80000002: {
//            qint16 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x00000004: {
//            quint32 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x80000004: {
//            qint32 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0x80000008: {
//            qint64 v;
//            readStream >> v;
//            realValue = (double)v;
//            break;}
//        case 0xC0000004: {
//            float v;
//            readStream >> v;
//            realValue = v;
//            break;}
//        case 0xC0000008:
//        case 0xC000000A:
//            readStream >> realValue;
//            break;
//        default: break;
//    }

//    return postprocess(realValue);
//}

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
    switch (IndType) {
        case 0xC0000004: return Descriptor::RealSingle;
        default: break;
    }
    return Descriptor::RealDouble;
}

//QString DfdChannel::typeDescription() const
//{

//}

QString DfdChannel::xName() const
{DD;
    return XName;
}

double DfdChannel::xBegin() const
{DD;
    return xMin;
}

quint32 DfdChannel::samplesCount() const
{DD;
    return NumInd;//YValues.size();
}

void DfdChannel::addCorrection(double correctionValue, bool writeToFile)
{
    populate();
    for (uint j = 0; j < YValues.size(); ++j)
        YValues[j] += correctionValue;

    oldCorrectionValue = correctionValue;

    if (!corrected) {
        nameBeforeCorrection = name();
        corrected = true;
    }

    setName(nameBeforeCorrection + QString(correctionValue>=0?"+":"")
                +QString::number(correctionValue));

    if (writeToFile) corrected = false;

    YMaxInitial += correctionValue;
    YMinInitial += correctionValue;
}

void RawChannel::read(QSettings &dfd, int chanIndex)
{DD;
    DfdChannel::read(dfd,chanIndex);
    dfd.beginGroup(QString("Channel%1").arg(chanIndex));
    ADC0 = hextodouble( dfd.value("ADC0").toString()); //qDebug()<<"ADC0" << ch.adc0;
    ADCStep = hextodouble(dfd.value("ADCStep").toString()); //qDebug()<< "ADCStep"<< ch.adcStep;
    AmplShift = hextodouble(dfd.value("AmplShift").toString()); //qDebug()<<"AmplShift" <<ch.amplShift ;
    AmplLevel = hextodouble(dfd.value("AmplLevel").toString()); //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    Sens0Shift = hextodouble(dfd.value("Sens0Shift").toString()); //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    SensSensitivity = hextodouble(dfd.value("SensSensitivity").toString()); //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    BandWidth = hextofloat(dfd.value("BandWidth").toString()); //qDebug()<< "BandWidth"<< ch.bandwidth;
    SensName = dfd.value("SensName").toString();
    dfd.endGroup();

    coef1 = ADCStep / AmplLevel / SensSensitivity;
    coef2 = (ADC0 / AmplLevel - AmplShift - Sens0Shift) / SensSensitivity;

    coef3 = SensSensitivity * AmplLevel / ADCStep;
    coef4 = ((Sens0Shift + AmplShift) * AmplLevel - ADC0) / ADCStep;
}

void RawChannel::write(QTextStream &dfd, int chanIndex)
{DD;
    DfdChannel::write(dfd,chanIndex);

    dfd << "ADC0=" <<  doubletohex(ADC0).toUpper() << endl;
    dfd << "ADCStep=" <<  doubletohex(ADCStep).toUpper() << endl; //qDebug()<< "ADCStep"<< ch.adcStep;
    dfd << "AmplShift=" << doubletohex(AmplShift).toUpper() << endl; //qDebug()<<"AmplShift" <<ch.amplShift ;
    dfd << "AmplLevel=" << doubletohex(AmplLevel).toUpper() << endl; //qDebug()<< "AmplLevel"<<ch.amplLevel ;
    dfd << "Sens0Shift=" << doubletohex(Sens0Shift).toUpper() << endl; //qDebug()<< "Sens0Shift"<<ch.sens0Shift ;
    dfd << "SensSensitivity=" << doubletohex(SensSensitivity).toUpper() << endl; //qDebug()<< "SensSensitivity"<<ch.sensSensitivity ;
    dfd << "BandWidth=" <<  floattohex(BandWidth).toUpper() << endl;
    dfd << "SensName=" <<  SensName << endl;
}

QStringList RawChannel::getInfoHeaders()
{DD;
    QStringList result = DfdChannel::getInfoHeaders();
    result.append(QStringList()
                  << QString("Смещ.") //ADC0
                  << QString("Множ.") //ADCStep
                  << QString("Смещ.ус.(В)") //AmplShift
                  << QString("Ус.(дБ)") //AmplLevel
                  << QString("Смещ.датч.(мВ)") //Sens0Shift
                  << QString("Чувств.") //SensSensitivity
                  << QString("Полоса") //BandWidth
                  << QString("Датчик") //SensName
                  );
    return result;
}

QStringList RawChannel::getInfoData()
{DD;
    // пересчитываем усиление из В в дБ
    double ampllevel = AmplLevel;
    if (ampllevel <= 0.0) ampllevel = 1.0;
    QStringList result = DfdChannel::getInfoData();
    result.append(QStringList()
                  << QString::number(ADC0)
                  << QString::number(ADCStep)
                  << QString::number(AmplShift)
                  << QString::number(qRound(20*log(ampllevel)))
                  << QString::number(Sens0Shift)
                  << QString::number(SensSensitivity)
                  << QString::number(BandWidth)
                  << SensName
                  );
    return result;
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

void RawChannel::populate()
{DD;
    DfdChannel::populate();

    // rescale initial min and max values with first 200 values
    XMaxInitial = parent->xBegin() + XStep*200;
    YMinInitial = 0.0;
    YMaxInitial = 0.0;

    if (YValues.isEmpty()) return;
    const int steps = qMin(samplesCount(), 200u);
    for (int i=0; i<steps; ++i) {
        double val = YValues[i];
        if (val > YMaxInitial) YMaxInitial = val;
        if (val < YMinInitial) YMinInitial = val;
    }
}


DataDescription::DataDescription(DfdFileDescriptor *parent) : parent(parent)
{DD;

}

void DataDescription::read(QSettings &dfd)
{DD;
    dfd.beginGroup("DataDescription");
    QStringList keys = dfd.childKeys();
    foreach (QString key, keys) {
        QString v = dfd.value(key).toString();
        if (v.startsWith("\"")) v.remove(0,1);
        if (v.endsWith("\"")) v.chop(1);
        v = v.trimmed();
        if (!v.isEmpty()) {
            if (key == "Legend")
                parent->_legend = v;
            else
                data.append(DescriptionEntry(key, v));
        }
    }
    dfd.endGroup();
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
