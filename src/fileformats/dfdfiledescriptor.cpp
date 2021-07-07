#include "dfdfiledescriptor.h"
#include <QtCore>

#include <QtWidgets>
#include <QUuid>

#include "logging.h"
#include "algorithms.h"
#include "dfdsettings.h"
#include "averaging.h"

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
      BlockSize(0)
{DD;
}

DfdFileDescriptor::DfdFileDescriptor(const FileDescriptor &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)

{
    setDataDescription(other.dataDescription());
    dataDescription().put("source.file", other.fileName());
    dataDescription().put("source.guid", other.dataDescription().get("guid"));
    dataDescription().put("source.dateTime", other.dataDescription().get("dateTime"));

    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;
    else
        dataDescription().put("source.channels", stringify(indexes));

    fillPreliminary(&other);

//    qDebug()<<dataDescription().toStringList();


    //Поскольку other может содержать каналы с разным типом, размером и шагом,
    //данные берем из первого канала, который будем сохранять
    //предполагается, что все каналы из indexes имеют одинаковые параметры

    Channel *firstChannel = other.channel(indexes.constFirst());

    //копируем каналы и записываем данные
    QFile raw(rawFileName);
    if (!raw.open(QFile::WriteOnly)) {
        qDebug()<<"Unable to open file"<<rawFileName<<"to write";
        return;
    }
    QDataStream w(&raw);
    w.setByteOrder(QDataStream::LittleEndian);
    xChannel = firstChannel->data()->xValuesFormat()==DataHolder::XValuesNonUniform;

    //сначала канал для данных X
    if (xChannel) {
        w.setFloatingPointPrecision(QDataStream::SinglePrecision);
        QVector<double> vals = firstChannel->data()->xValues();
        DfdChannel ch(0,0);
        ch.IndType = 0xC0000004;
        for (double val: vals)
            ch.setValue(val, w);
    }
    //остальные каналы
    for (int i: indexes) {
        DfdChannel *c = new DfdChannel(*other.channel(i), this);

        Channel *sourceChannel = other.channel(i);
        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();
        c->write(w, sourceChannel->data()); //данные берутся из sourceChannel

        if (!populated)
            sourceChannel->clear();
    }

    //Сохраняем файл dfd
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug()<<"Unable to open file"<<fileName<<"to write";
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    QTextStream dfd(&file);
    dfd.setCodec(codec);
    writeDfd(dfd); //Записываем шапку файла и канал с осью X, если есть

    //Остальные каналы
    int index = xChannel ? 1 : 0;
    for (DfdChannel *c: channels) {
        c->write(dfd, index++);
    }
}

DfdFileDescriptor::~DfdFileDescriptor()
{DD;
    if (changed() || dataChanged())
        write();
    qDeleteAll(channels);
}

int octaveFormat(int DataType)
{
    switch(DataType) {
        case 156: return 1;
        case 157: return 3;
        case 158: return 2;
        case 159: return 6;
        case 160: return 12;
        case 161: return 24;
    }
    return 0;
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
    dataDescription().put("guid",dfd.value("DataFileDescriptor/DFDGUID"));
    DataType =  DfdDataType(dfd.value("DataFileDescriptor/DataType").toInt());
    dataDescription().put("function.type", dataTypefromDfdDataType(DataType));

    dataDescription().put("dateTime", dateTimeFromString(dfd.value("DataFileDescriptor/Date")
                                                         +" "
                                                         +dfd.value("DataFileDescriptor/Time")));

    int NumChans =  dfd.value("DataFileDescriptor/NumChans").toInt();
    uint NumInd = dfd.value("DataFileDescriptor/NumInd").toUInt();
    dataDescription().put("samples", NumInd);
    BlockSize = dfd.value("DataFileDescriptor/BlockSize").toInt();
    dataDescription().put("createdBy", dfd.value("DataFileDescriptor/CreatedBy"));
    QString XName = dfd.value("DataFileDescriptor/XName");
    dataDescription().put("xname", XName);
    if (XName == "№№ полос") xChannel = true;
    double XBegin = hextodouble(dfd.value("DataFileDescriptor/XBegin"));
    double XStep = hextodouble(dfd.value("DataFileDescriptor/XStep"));

    // [DataDescription]
    if (childGroups.contains("DataDescription")) {
        Description descr(this);
        descr.read(dfd);
        //qDebug()<<dataDescription().filter();
    }

    // [Source]
    if (childGroups.contains("Source") || childGroups.contains("Sources")) {
        Source source(this);
        source.read(dfd);
    }

    // [Process] - channel specific
    Process process(this);
    process.read(dfd, DataType);

    // [Channel#]
    for (int i = 0; i<NumChans; ++i) {
        DfdChannel *ch = newChannel(i);
        ch->read(dfd, NumChans, XBegin, XStep);

        //в эту функцию вставить следующий код: (function.format == "unknown")
//        if (yName.toLower()=="db" || yName.toLower()=="дб") return DataHolder::YValuesAmplitudesInDB;
//        else return DataHolder::YValuesReals;
    }

    // проверяем наличие канала со значениями оси X
    if (!channels.isEmpty() && DataType >= OSpectr) {
        QVector<double> xvalues;
        DfdChannel *c = channels.constFirst();
        xChannel = c->name()=="ось X";
        if (xChannel) {//нашли канал со значениями оси X
            c->populate();
            xvalues = c->data()->yValues(0);
        }
        else {
            xvalues = octaveStrips(octaveFormat(DataType), NumInd);
        }
        XName = "Гц";
        if (!xvalues.isEmpty())
            for (DfdChannel *ch: channels)
                ch->data()->setXValues(xvalues);

        if (xChannel) {
            //удаляем лишний канал
            delete channels.takeFirst();
            //проследить, чтобы удаленный канал не мешал чтению данных и удалению/перемещению каналов
        }
    }
    //удаляем информацию о каналах из информации о файле
    DataDescription d;
    for (auto i=dataDescription().data.begin(); i!=dataDescription().data.end(); ++i) {
        if (!i.key().startsWith("function.")) d.put(i.key(), i.value());
    }
    dataDescription() = d;
}

void DfdFileDescriptor::writeDfd(QTextStream &dfdStream)
{
    /** [DataFileDescriptor]*/
    dfdStream << "[DataFileDescriptor]" << endl;
    dfdStream << "DFDGUID="<<dataDescription().get("guid").toString() << endl;
    dfdStream << "DataType="<<DataType << endl;
    dfdStream << "Date="<<dataDescription().get("dateTime").toDateTime().toString("dd.MM.yyyy")
        << endl;
    dfdStream << "Time="<<dataDescription().get("dateTime").toDateTime().toString("hh:mm:ss")
        << endl;
    dfdStream << "NumChans="<< (xChannel ? channels.size()+1 : channels.size()) << endl;
    int numInd = 0;
    double xBegin = 0.0;
    double xStep = 0.0;
    if (!channels.isEmpty()) {
        numInd = channels.first()->data()->samplesCount();
        xBegin = channels.first()->data()->xMin();
        xStep = channels.first()->data()->xStep();
    }
    dfdStream << "NumInd="<< numInd << endl;
    dfdStream << "BlockSize="<<BlockSize << endl;
    dfdStream << "XName="<< xName() << endl;
    dfdStream << "XBegin=" << doubletohex(xBegin).toUpper() << endl;
    dfdStream << "XStep=" << doubletohex(xStep).toUpper() << endl;
    dfdStream << "DescriptionFormat=" << dataDescription().get("description.format").toString()
        << endl;
    dfdStream << "CreatedBy="<<dataDescription().get("createdBy").toString()
        << endl;

    /** [DataDescription] */
    Description descr(this);
    descr.write(dfdStream);

    /** [Source] */
    Source source(this);
    source.write(dfdStream);

    /** [Process] */
    if (!channels.isEmpty()) {
        Process process(&(channels[0]->dataDescription()));
        process.write(dfdStream);
    }


    if (xChannel) {
        //добавляем нулевой канал с осью Х
        DfdChannel ch(0, 0);
        ch.setName("ось X");
        ch.setYName("Гц");
        ch.ChanBlockSize = channels.constFirst()->data()->samplesCount();
        ch.IndType = 3221225476;
        ch.data()->setThreshold(1.0);
        ch.data()->setXValues(channels.constFirst()->data()->xValues());
        ch.data()->setYValues(channels.constFirst()->data()->xValues(), DataHolder::YValuesReals);
        ch.write(dfdStream, 0);
    }
}

void DfdFileDescriptor::write()
{DD;
    if (changed()) {
        QTextCodec *codec = QTextCodec::codecForName("Windows-1251");

        QFile file(fileName());
        if (!file.open(QFile::WriteOnly | QFile::Text)) return;
        QTextStream dfd(&file);
        dfd.setCodec(codec);

        writeDfd(dfd);

        int b = xChannel ? 1 : 0;
        for (int i=0; i<channels.size(); ++i)
            channels[i]->write(dfd, i+b);

        setChanged(false);
    }
    if (dataChanged()) {
        //всегда меняем на 0
        BlockSize = 0;
        //переписываем с помощью временного файла
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            QDataStream w(&tempFile);
            w.setByteOrder(QDataStream::LittleEndian);
            //сперва нулевой канал оси X
            w.setFloatingPointPrecision(QDataStream::SinglePrecision);
            if (xChannel) {
                QVector<double> vals = channels.first()->data()->xValues();
                DfdChannel ch(0,0);
                ch.IndType = 0xC0000004;
                for (double val: vals)
                    ch.setValue(val, w);
            }
            //остальные каналы
            for (DfdChannel *c: channels) {
                bool populated = c->populated();
                if (!populated) c->populate();
                quint64 dataPos = w.device()->pos();
                c->write(w, c->data());

                c->dataPositions.clear();
                c->dataPositions.append(dataPos);
                c->ChanBlockSize = c->data()->samplesCount();

                if (!populated)
                    c->clear();
            }
        }
        tempFile.close();
        // удаляем файл данных, если мы перезаписывали его
        if (QFile::remove(rawFileName) || !QFile::exists(rawFileName)) {
            if (!tempFile.copy(rawFileName)) {
                qDebug()<<"Failed to replace"<<rawFileName<<"with temp file";
                return;
            }
        }

        setDataChanged(false);
    }
}

void DfdFileDescriptor::fillPreliminary(const FileDescriptor *file)
{DD;
    FileDescriptor::fillPreliminary(file);
    const DfdFileDescriptor *dfd = dynamic_cast<const DfdFileDescriptor*>(file);
    if (dfd) {
        DataType = dfd->DataType;
    }
    else {
        ///TODO: переписать определение типа данных файла DFD
        DataType = dfdDataTypeFromDataType(*file->channel(0));
    }

    rawFileName = fileName().left(fileName().length()-4)+".raw";


    // time data tweak, so deepseabase doesn't take the file as raw time data
    //так как мы вызываем эту функцию только из новых файлов,
    //все сведения из файлов rawChannel нам не нужны
    if (DataType == SourceData) DataType = CuttedData;
    if (DataType >= OSpectr && DataType <= TFOSpectr) xChannel = true;

    BlockSize = 0; // всегда меняем размер блока новых файлов на 0,
                   // чтобы они записывались без перекрытия
}

DfdFileDescriptor *DfdFileDescriptor::newFile(const QString &fileName, DfdDataType type)
{
    DfdFileDescriptor *dfd = new DfdFileDescriptor(fileName);
    dfd->DataType = type;
    dfd->rawFileName = fileName.left(fileName.length()-4)+".raw";
    dfd->updateDateTimeGUID();
    dfd->BlockSize = 0;
    return dfd;
}

bool DfdFileDescriptor::copyTo(const QString &name)
{
    QString rawFile = name;
    QString suffix = QFileInfo(name).suffix();
    rawFile.replace(rawFile.length() - suffix.length(), suffix.length(), "raw");

    return FileDescriptor::copyTo(name) && QFile::copy(rawFileName, rawFile);
}

Descriptor::DataType DfdFileDescriptor::type() const
{DD;
    return dataTypefromDfdDataType(DataType);
}

QString DfdFileDescriptor::typeDisplay() const
{
    //return dataTypeDescription(DataType);
    switch (DataType) {
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

qint64 DfdFileDescriptor::fileSize() const
{
    return QFileInfo(rawFileName).size();
}

bool DfdFileDescriptor::fileExists() const
{
    return (FileDescriptor::fileExists() && QFileInfo(rawFileName).exists());
}

void DfdFileDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    if (channelsToDelete.isEmpty()) return;

    //всегда меняем на 0
    BlockSize = 0;
    //переписываем с помощью временного файла
    QTemporaryFile tempFile;
    if (tempFile.open()) {
        QDataStream w(&tempFile);
        w.setByteOrder(QDataStream::LittleEndian);
        //сперва нулевой канал оси X
        w.setFloatingPointPrecision(QDataStream::SinglePrecision);
        if (xChannel) {
            QVector<double> vals = channels.first()->data()->xValues();
            DfdChannel ch(0,0);
            ch.IndType = 0xC0000004;
            for (double val: vals)
                ch.setValue(val, w);
        }
        //остальные каналы
        for (int i=0; i<channels.count(); ++i) {
            if (channelsToDelete.contains(i)) continue;
            DfdChannel *c = channels[i];
            bool populated = c->populated();
            if (!populated) c->populate();
            quint64 dataPos = w.device()->pos();
            c->write(w, c->data());

            c->dataPositions.clear();
            c->dataPositions.append(dataPos);
            c->ChanBlockSize = c->data()->samplesCount();

            if (!populated) c->clear();
        }
    }
    tempFile.close();
    // удаляем файл данных, если мы перезаписывали его
    if (QFile::remove(rawFileName)) {
        if (!tempFile.copy(rawFileName)) {
            qDebug()<<"Failed to replace"<<rawFileName<<"with temp file";
            return;
        }
    }

    setDataChanged(false);

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    //перенумеровываем каналы
    for (int i=0; i<channels.size(); ++i) {
        channels[i]->channelIndex = i;
    }

    //осталось пересохранить файл dfd
    setChanged(true);
    write();
}


void DfdFileDescriptor::copyChannelsFrom_plain(FileDescriptor *file, const QVector<int> &indexes)
{DD;
    //заполняем данными файл, куда будем копировать каналы
    //читаем все каналы, чтобы сохранить файл полностью
    populate();

    for (int index: indexes) {
        bool wasPopulated = file->channel(index)->populated();
        if (!wasPopulated) file->channel(index)->populate();
        new DfdChannel(*file->channel(index), this);
        if (!wasPopulated) file->channel(index)->clear();
    }

    for (int i=0; i<channels.size(); ++i) {
        channels[i]->channelIndex = i;
    }

    //меняем параметры файла dfd
    setDateTime(QDateTime::currentDateTime());

    setChanged(true);
    setDataChanged(true);
    write();
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

//        if (newCh->data()->xValuesFormat() == DataHolder::XValuesNonUniform)
//            xValues = newCh->data()->xValues();

        //меняем параметры канала
        newCh->IndType = channels[0]->IndType; //характеристика отсчета
        newCh->ChanBlockSize = newCh->data()->samplesCount(); //размер блока в отсчетах

        //меняем, если копировали из Source в Cutted
        newCh->dataType = DataType;

        //добавляем данные в файл raw напрямую, обходя функцию writeRawFile
        //функция ничего не делает, если BlockSize != 0
        newCh->setPopulated(true);
        newCh->appendDataTo(rawFileName);

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

void DfdFileDescriptor::addChannelWithData(DataHolder *data, const DataDescription &description)
{
    // обновляем сведения канала
    DfdChannel *ch = new DfdChannel(this, channelsCount());
    ch->setPopulated(true);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setData(data);
    ch->dataDescription() = description;

    ch->ChanBlockSize = data->samplesCount();
    ch->IndType = channelsCount()==1 ? 3221225476 : channels.constFirst()->IndType;
    if (description.get("yname") == description.get("ynameold") && data->yValuesFormat() == DataHolder::YValuesAmplitudesInDB)
        ch->dataDescription().put("yname", "дБ");

    int octave = description.get("function.octaveFormat").toInt();
    switch (octave) {
        case 1: DataType = OSpectr; break;
        case 3: DataType = ToSpectr; break;	// 1/3-октавный спектр
        case 2: DataType = TwoOSpectr; break;     // 1/2-октавный спектр
        case 6: DataType = SixOSpectr; break;    // 1/6-октавный спектр
        case 12: DataType = TwlOSpectr; break;     // 1/12-октавный спектр
        case 24: DataType = TFOSpectr; break;      // 1/24-октавный спектр
        default: break;
    }

//    //третьоктава или октава: канал только один и данные по x неравномерны
//    if (channelsCount()==1 && ch->data()->xValuesFormat()==DataHolder::XValuesNonUniform) {
//        xValues = ch->data()->xValues();
//        //     добавляем нулевой канал с осью Х
//        DfdChannel *ch0 = new DfdChannel(this, channelsCount());
//        ch0->ChanAddress.clear(); //
//        ch0->ChanName = "ось X"; //
//        ch0->YName="Гц";
//        ch0->YNameOld.clear();
//        ch0->InputType.clear();
//        ch0->ChanDscr.clear();
//        ch0->channelIndex = 0; // нумерация с 0
//        ch0->ChanBlockSize = xValues.size();
//        ch0->IndType = channelsCount()==2 ? 3221225476 : this->channels.constFirst()->IndType;
//        ch0->data()->setThreshold(1.0);
//        ch0->data()->setXValues(xValues);
//        ch0->data()->setYValues(xValues, DataHolder::YValuesReals);
//        ch0->setPopulated(true);

//        channels.prepend(ch0);
//        channels.takeLast(); //нулевой канал автоматически был добавлен в конец. Убираем его в начало

//        for (int i=0; i<channelsCount(); ++i)
//            channels[i]->channelIndex = i;
//    }

    if ((channelsCount()==2 && ch->data()->xValuesFormat()==DataHolder::XValuesNonUniform)
        || channelsCount()==1) {
        //заполняем всю остальную информацию
        BlockSize = 0;
//        NumInd = data->samplesCount();
//        XBegin = data->xMin();
//        XStep = data->xStep();
    }
}


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
                for (qint64 pos: ch->dataPositions) {
                    rawFile.seek(pos);
                    QByteArray b = rawFile.read(ch->blockSizeInBytes());
                    tempFile.write(b);
                }
                tempFile.flush();

                // меняем размер блока и обновляем положение данных,
                //если перезаписываем файл поверх самого себя
                ch->dataPositions.clear();
                ch->dataPositions.append(dataPos);
                ch->ChanBlockSize = ch->data()->samplesCount();
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
                    for (int i=0; i < ch->data()->samplesCount(); ++i) {
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
                    ch->ChanBlockSize = ch->data()->samplesCount();;
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
                    ch->ChanBlockSize = ch->data()->samplesCount();
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
    if (!tempFileSuccessful) {
        setDataChanged(true);
    }
    write();
}

Channel *DfdFileDescriptor::channel(int index) const
{
    if (channels.size()>index) return channels[index];
    return nullptr;
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
    DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(other);
    if (!dfd)
        return (dataTypefromDfdDataType(DataType) == other->type()) && qFuzzyIsNull(this->xStep() - other->xStep());
    else {
        if (DataType == dfd->DataType) return true;

        if (xStep() != other->xStep()) return false;

        if (dfd->DataType == SourceData && DataType > 0 && DataType < 16) {
            return true;
        }
    }
    return false;
}

bool DfdFileDescriptor::canTakeAnyChannels() const
{
    return false;
}

QStringList DfdFileDescriptor::fileFilters()
{DD;
    return QStringList()<< "Файлы dfd (*.dfd)" << "Файлы dfd (*.DFD)";
}

DfdChannel *DfdFileDescriptor::newChannel(int chanIndex)
{DD;
    DfdChannel *c = 0;
    switch (DataType) {
        case SourceData: c = new RawChannel(this, chanIndex); break;
        default: c = new DfdChannel(this, chanIndex); break;
    }
    return c;
}

QStringList DfdFileDescriptor::suffixes()
{
    return QStringList()<<"*.dfd"<<"*.DFD";
}

void Process::read(DfdSettings &dfd, DfdDataType dataType)
{DD;
    auto process = dfd.values("Process");
    QString typeScale;
    for (const auto &val : process) {
        ///TODO: реализовать с учетом function
        if (val.first == "TypeAver") {
            QString averaging = val.second;
            if (val.second == "линейное") averaging = "linear";
            else if (val.second == "экспоненциальное") averaging = "exponential";
            else if (val.second == "хранение максимума") averaging = "peak hold";
            else if (val.second == "без усреднения") averaging = "no";
            parent->dataDescription().put("function.averaging", averaging);
        }
        else if (val.first == "NAver") {
            parent->dataDescription().put("function.averagingCount", val.second.toInt());
        }
        else if (val.first == "Wind") {
            QString window = val.second;
            if (val.second == "Хеннинга") window = "Hann";
            else if (val.second == "Хемминга") window = "Hamming";
            else if (val.second == "Натолл") window = "Nuttall";
            else if (val.second == "Гаусс") window = "Gauss";
            else if (val.second == "Прямоуг.") window = "square";
            else if (val.second == "Бартлетта") window = "Bartlett";
            parent->dataDescription().put("function.window", window);
        }
        else if (val.first == "BlockIn") {
            parent->dataDescription().put("function.blockSize", val.second.toInt());
        }
        else if (val.first == "ProcChansList") {
            parent->dataDescription().put("function.channels", val.second);
        }
        else if (val.first == "TypeScale") {
            typeScale = val.second;
        }
        else if (val.first == "pBaseChan") {
            parent->dataDescription().put("function.referenceName", val.second.section(',', 2,2));
            parent->dataDescription().put("function.referenceDescription", val.second);
        }
        else
            parent->dataDescription().put("function."+val.first, val.second);
    }
    auto format = dataFormat(dataType, typeScale);
    QString fS = "unknown";
    switch (format) {
        case DataHolder::YValuesComplex: fS = "complex"; break;
        case DataHolder::YValuesAmplitudes: fS = "amplitude"; break;
        case DataHolder::YValuesAmplitudesInDB: fS = "amplitudeDb"; break;
        case DataHolder::YValuesPhases: fS = "phase"; break;
        case DataHolder::YValuesReals: fS = "real"; break;
        case DataHolder::YValuesImags: fS = "imaginary"; break;
        default: break;
    }
    parent->dataDescription().put("function.format", fS);
    int octave = 0;
    switch (dataType) {
        case ToSpectr: octave = 3; break;
        case OSpectr: octave = 1; break;
        case TwoOSpectr: octave = 2; break;
        case SixOSpectr: octave = 6; break;
        case TwlOSpectr: octave = 12; break;
        case TFOSpectr: octave = 24; break;
        default: break;
    }
    parent->dataDescription().put("function.octaveFormat", octave);
}

void Process::write(QTextStream &dfd)
{DD;
    DataDescription *d = nullptr;
    if (data) d = data;
    else if (parent) d = &parent->dataDescription();

    if (!d) return;

    dfd << "[Process]" << endl;
    ///TODO: реализовать с учетом function
    QString ref = d->data.value("function.referenceName").toString();

    for (auto [key, v] : asKeyValueRange(d->data)) {
        QString val = v.toString();
        if (key == "function.averaging") {
            if (val == "linear") dfd << "TypeAver="<<"линейное"<<endl;
            else if (val == "exponential") dfd << "TypeAver="<<"линейное"<<endl;
            else if (val == "peak hold") dfd << "TypeAver="<<"хранение максимума"<<endl;
            else if (val == "energetic") dfd << "TypeAver="<<"энергетическое"<<endl;
            else if (val == "no") dfd << "TypeAver="<<"без усреднения"<<endl;
        }
        else if (key == "function.averagingCount") {
            dfd << "NAver=" << val << endl;
        }
        else if (key == "function.window") {
            QString window = val;
            if (val.toLower() == "hann" || val.toLower() == "hanning") window = "Хеннинга";
            else if (val.toLower() == "hamming") window = "Хемминга";
            else if (val.toLower() == "nuttall") window = "Натолл";
            else if (val.toLower() == "gauss") window = "Гаусс";
            else if (val.toLower() == "square") window = "Прямоуг.";
            else if (val.toLower() == "bartlett") window = "Бартлетта";
            dfd << "Wind=" << window << endl;
        }
        else if (key == "function.blockSize")
            dfd << "BlockIn=" << val << endl;
        else if (key == "function.channels")
            dfd << "ProcChansList=" << val << endl;
        else if (key == "function.referenceDescription") {
            ref = val;
            if (!ref.isEmpty()) dfd << "pBaseChan=" << ref << endl;
        }
        else if (key.startsWith("function."))
            dfd << key.section('.',1) << "=" << val << endl;
    }

//    for (auto it = d->data.constBegin(); it != d->data.constEnd(); ++it) {
//        QString key = it.key();
//        QString val = it.value().toString();

//    }
}

void Source::read(DfdSettings &dfd)
{DD;
    QString sFile = dfd.value("Sources/sFile");
    if (!sFile.isEmpty()) {
        //K:\Лопасть_В3_бш_20кГц.DFD[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19],{7FD333E3-9A20-2A3E-A9443EC17B134848}
        parent->dataDescription().put("source.file", sFile.section("[",0,0));
        QString channels = sFile.section("[",1).section("]",0,0);
        parent->dataDescription().put("source.channels", channels);
        parent->dataDescription().put("source.guid", sFile.section("{",1).section("}",0,0));
    }
    else {
        parent->dataDescription().put("source.file", dfd.value("Source/File"));
        QString DFDGUID = dfd.value("Source/DFDGUID");
        if (DFDGUID.startsWith("{")) DFDGUID.remove(0,1);
        if (DFDGUID.endsWith("}")) DFDGUID.chop(1);
        parent->dataDescription().put("source.guid", DFDGUID);
        parent->dataDescription().put("source.dateTime",
                                      dateTimeFromString(dfd.value("Source/Date")+" "+dfd.value("Source/Time")));
    }
}

void Source::write(QTextStream &dfd)
{DD;

    if (QString channels = parent->dataDescription().get("source.channels").toString();
        !channels.isEmpty()) {
        dfd << "[Sources]" << endl;
        dfd << "sFile=";
        dfd << parent->dataDescription().get("source.file").toString()
            << "["<<channels<<"],{"
            << parent->dataDescription().get("source.guid").toString() <<"}";
    }

    dfd << "[Source]" << endl;
    dfd << "File=" << parent->dataDescription().get("source.file").toString() << endl;
    dfd << "DFDGUID=" << parent->dataDescription().get("source.guid").toString() << endl;
    QDateTime dt = parent->dataDescription().get("source.dateTime").toDateTime();
    dfd << "Date=" << dt.toString("dd.MM.yyyy") << endl;
    dfd << "Time=" << dt.toString("hh:mm:ss") << endl;
}

DfdChannel::DfdChannel(DfdFileDescriptor *parent, int channelIndex)
    : Channel(), IndType(0),
      ChanBlockSize(0),
      parent(parent),
      channelIndex(channelIndex)
{
    if (parent) {
        dataType = parent->DataType;
        parent->channels << this;

        //копируем из файла описание функции
        for (auto it = parent->dataDescription().data.begin();
             it != parent->dataDescription().data.end(); ++it) {
            if (it.key().startsWith("function.")) {
                dataDescription().put(it.key(), it.value());
                //it = parent->dataDescription().data.erase(it);
            }
        }
        dataDescription().put("xname", parent->dataDescription().get("xname"));
        dataDescription().put("samples", parent->dataDescription().get("xname"));
    }
}

DfdChannel::DfdChannel(DfdChannel &other, DfdFileDescriptor *parent) : Channel(other)
{DD;
    // data is copied in the Channel construction

    this->parent = parent;
    parent->channels << this;

    setDataDescription(other.dataDescription());
    IndType = other.IndType;
    ChanBlockSize = other.ChanBlockSize; //размер блока в отсчетах
    channelIndex = parent->channels.size()-1;
    dataType = other.dataType;
}

DfdChannel::DfdChannel(Channel &other, DfdFileDescriptor *parent) : Channel(other)
{DD;
    // data is copied in the Channel construction

    this->parent = parent;
    parent->channels << this;

    setDataDescription(other.dataDescription());
    qDebug()<<dataDescription().toStringList();

    QString precision = dataDescription().get("function.precision").toString();
    if (precision == "uint8") IndType = 0x1;
    else if (precision == "int8") IndType = 0x80000001;
    else if (precision == "uint16") IndType = 0x2;
    else if (precision == "int16") IndType = 0x80000002;
    else if (precision == "uint32") IndType = 0x4;
    else if (precision == "int32") IndType = 0x80000004;
    else if (precision == "uint64") IndType = 0x8;
    else if (precision == "int64") IndType = 0x80000008;
    else if (precision == "float") IndType = 0xc0000004;
    else if (precision == "double") IndType = 0xc0000008;

    ChanBlockSize = other.data()->samplesCount();

//    if (other.data()->yValuesFormat() == DataHolder::YValuesAmplitudesInDB) {
//        YName = "дБ";
//    }

    dataType = dfdDataTypeFromDataType(other);
}

DfdChannel::~DfdChannel()
{
    //delete [] YValues;
}

void DfdChannel::read(DfdSettings &dfd, int numChans, double xBegin, double xStep)
{DD;
    QString group = QString("Channel%1/").arg(channelIndex+1);

    dataDescription().put("sensorID", dfd.value(group+"ChanAddress"));
    dataDescription().put("name", dfd.value(group+"ChanName"));
    IndType = dfd.value(group+"IndType").toUInt();
    QString precision = "float";
    switch (IndType) {
        case 0x1: precision = "uint8"; break;
        case 0x80000001: precision = "int8"; break;
        case 0x2: precision = "uint16"; break;
        case 0x80000002: precision = "int16"; break;
        case 0x4: precision = "uint32"; break;
        case 0x80000004: precision = "int32"; break;
        case 0x8: precision = "uint64"; break;
        case 0x80000008: precision = "int64"; break;
        case 0xc0000004: precision = "float"; break;
        case 0xc0000008: precision = "double"; break;
        case 0xc000000a: precision = "double"; break;
    }
    dataDescription().put("function.precision", precision);
    ChanBlockSize = dfd.value(group+"ChanBlockSize").toInt();
    QString YName = dfd.value(group+"YName");
    dataDescription().put("yname", YName);
    dataDescription().put("ynameold", dfd.value(group+"YNameOld"));
    dataDescription().put("inputType", dfd.value(group+"InputType"));
    dataDescription().put("description", dfd.value(group+"ChanDscr"));
    dataDescription().put("correction", dfd.value(group+"Correction"));

    //dfd не понимает многоблочные данные
    data()->setZValues(0,0,1);
    dataDescription().put("blocks", 1);

    if (qFuzzyIsNull(xStep)) {// abscissa, uneven spacing
        _data->setXValues(QVector<double>());
        _data->setSamplesCount(parent->dataDescription().get("samples").toInt());
    }
    else {// abscissa, even spacing
        _data->setXValues(xBegin, xStep, parent->dataDescription().get("samples").toInt());
    }

    auto yValueFormat = DataHolder::formatFromString(dataDescription().get("function.format").toString());
    if (yValueFormat == DataHolder::YValuesUnknown) {
        //по умолчанию считаем, что данные линейные, если отсутствует запись [Process].TypeScale
        yValueFormat = dataFormat(parent->DataType, "линейная");
    }

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
            thr = threshold(dataDescription().get("ynameold").toString());
        else
            thr = threshold(YName);
        if (type()==Descriptor::FrequencyResponseFunction) thr=1.0;
    }
    else {
        thr = thrString.toDouble();
    }
    _data->setThreshold(thr);
    dataDescription().put("logref", thr);

    auto units = DataHolder::UnitsLinear;
    QString unitsStr = dfd.value(group+"units");
    if (unitsStr.isEmpty()) {
        if (dataType == Spectr || dataType == XSpectr) units = DataHolder::UnitsQuadratic;
    }
    else {
        units = DataHolder::unitsFromString(unitsStr);
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
        qint64 rawSize = QFile(parent->rawFileName).size();
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
    dfd << "ChanAddress=" << dataDescription().get("sensorID").toString() << endl;
//    QString correctedName = ChanName;
//    if (data()->hasCorrection()) correctedName.append(data()->correctionString());
    dfd << "ChanName=" << dataDescription().get("name").toString() << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << ChanBlockSize << endl;
    dfd << "YName=" << dataDescription().get("yname").toString() << endl;
    dfd << "YNameOld=" << dataDescription().get("ynameold").toString() << endl;
    dfd << "InputType="<< dataDescription().get("inputType").toString() << endl;
    dfd << "ChanDscr="<<dataDescription().get("description").toString() << endl;
    dfd << "Correction="<<correction() << endl;

    dfd << "threshold=" << QString::number(data()->threshold()) << endl;
    dfd << "units=" << DataHolder::unitsToString(data()->yValuesUnits()) << endl;
}

void DfdChannel::write(QDataStream &s, DataHolder *d)
{
    s.setByteOrder(QDataStream::LittleEndian);

    if (IndType==0xC0000004)
        s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //dfd не понимает многоблочные файлы
    QVector<double> yValues = d->rawYValues(0);
    if (yValues.isEmpty() && !d->yValuesComplex(0).isEmpty())
        yValues = d->linears(0);

    for (int val = 0; val < d->samplesCount() && val < yValues.size(); ++val) {
        setValue(yValues[val], s);
    }
}

QVariant DfdChannel::info(int column, bool edit) const
{
    switch (column) {
        case 0: return dataDescription().get("name");
        case 1: {
            QString result = yName();
            if (edit) return result;
            QString yNameO = yNameOld();
            if (!yNameO.isEmpty() && yNameO != result)
                result.append(QString(" (%1)").arg(yNameO));
            return result;
        }
        case 2: return data()->yValuesFormatString();
        case 3: return dataDescription().get("description");
        case 4: return dataDescription().get("correction");
        default: break;
    }

    return Channel::info(column, edit);
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
                qDebug()<<"IndType="<<IndType<<", mapped, by dataPositions";
                for (qint64 pos: dataPositions) {
                    ptrCurrent = ptr + pos;
                    QVector<double> temp = convertFrom<double>(ptrCurrent, qMin(quint64(maxPtr-ptrCurrent), blockSizeBytes), IndType);
                    YValues << temp;
                }
            } ///!dataPositions.isEmpty()
            else {
                qDebug()<<"IndType="<<IndType<<"blocksize"<<parent->BlockSize<<", mapped, skipping";
                /*
                * i-й отсчет n-го канала имеет номер
                * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
                *
                * если BlockSize=1 или ChanBlockSize=1, то
                * n + i*ChannelsCount
                */
                YValues.resize(data()->samplesCount());
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

            QDataStream readStream(&rawFile);
            readStream.setByteOrder(QDataStream::LittleEndian);
            if (IndType==0xC0000004)
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            qint64 actuallyRead = 0;

            if (parent->BlockSize > 0)  {
                //с перекрытием, сначала читаем блок данных в ChanBlockSize отчетов для всех каналов
                // если каналы имеют разный размер блоков, этот метод даст сбой
                qDebug()<<"IndType="<<IndType<<"BlockSize="<<parent->BlockSize<<"not mapped, ovlap";
                const quint64 chunkSize = channelsCount * ChanBlockSize;
                while (1) {
                    QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                    //распихиваем данные по каналам
                    actuallyRead /= channelsCount;
                    YValues << temp.mid(actuallyRead*channelIndex, actuallyRead);

                    if (actuallyRead < ChanBlockSize)
                        break;
                }
            }
            else {
                //без перекрытия, читаем данные всего канала (blockSize = 0, chanBlockSize != 0)
                qDebug()<<"IndType="<<IndType<<"BlockSize="<<parent->BlockSize<<"not mapped, without ovlap";
                readStream.device()->seek(channelIndex * blockSizeBytes);
                const quint64 chunkSize = ChanBlockSize;
                QVector<double> temp = getChunkOfData<double>(readStream, chunkSize, IndType, &actuallyRead);

                //распихиваем данные по каналам
                YValues = temp;
            }
        }

        YValues.resize(data()->samplesCount());
        postprocess(YValues);
        _data->setYValues(YValues, _data->yValuesFormat());
        setPopulated(true);
        rawFile.close();

//        if (!parent->xValues.isEmpty()) {
//            _data->setXValues(parent->xValues);
//        }
//        else {
//            if (_data->xValuesFormat() == DataHolder::XValuesNonUniform) {
//                if (channelIndex == 0) {
//                    //это канал со значениями X, мы только что его прочитали
//                    parent->xValues = YValues;
//                    _data->setXValues(YValues);
//                }
//                else {//unlikely to run
//                    QDataStream readStream(&rawFile);
//                    readStream.setByteOrder(QDataStream::LittleEndian);
//                    if (IndType==0xC0000004)
//                        readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
//                    rawFile.seek(0);
//                    readStream.setDevice(&rawFile);
//                    // читаем без перекрытия, предполагаем, что тип файла - третьоктава или октава
//                    QVector<double> temp = getChunkOfData<double>(readStream, ChanBlockSize, IndType);
//                    parent->xValues = temp;
//                    _data->setXValues(temp);
//                }
//            }
//        }
    }
    else {
        qDebug()<<"Cannot read raw file"<<parent->rawFileName;
    }
//    qDebug()<<"reading finished"<<timer.elapsed();
}

quint64 DfdChannel::blockSizeInBytes() const
{
    quint64 res = ChanBlockSize;
    return res * (IndType % 16);
}

FileDescriptor *DfdChannel::descriptor() const
{
    return parent;
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

        const int sc = data()->samplesCount();

        //dfd не понимает многоблочные файлы
        QVector<double> yValues = data()->rawYValues(0);
        if (yValues.isEmpty() && !data()->yValuesComplex(0).isEmpty()) {
            //комплексные данные, dfd не умеет их переваривать, конвертируем в линейные
            yValues = data()->linears(0);
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

QString DfdChannel::yNameOld() const
{
    return dataDescription().get("ynameold").toString();
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

void RawChannel::read(DfdSettings &dfd, int numChans, double xBegin, double xStep)
{DD;
    DfdChannel::read(dfd, numChans, xBegin, xStep);
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

Description::Description(DfdFileDescriptor *parent) : parent(parent)
{DD;

}

void Description::read(DfdSettings &dfd)
{DD;
    DescriptionList list = dfd.values("DataDescription");

    for (const auto &entry: list) {
        QString v = entry.second;
        if (v.startsWith("\"")) v.remove(0,1);
        if (v.endsWith("\"")) v.chop(1);
        v = v.trimmed();
        if (!v.isEmpty()) {
            if (entry.first == "Legend")
                parent->dataDescription().put("legend", v);
            else
                parent->dataDescription().put("description."+entry.first, v);
        }
    }
}

void Description::write(QTextStream &dfd)
{DD;
    dfd << "[DataDescription]" << endl;
    const DataDescription &descr = parent->dataDescription();
    for (auto it = descr.data.constBegin(); it != descr.data.constEnd(); ++it) {
        if (QString key = it.key(); key.startsWith("description")) {
            dfd << key.section('.',1) << "=\"" << it.value().toString() << "\"" << endl;
        }
    }
    dfd << "Legend=" << "\"" << parent->legend() << "\"" << endl;
}

QString Description::toString() const
{DD;
    QStringList result;
    for (int i=0; i<data.size(); ++i) {
        DescriptionEntry item = data.at(i);
        result.append(/*item.first+"="+*/item.second);
    }
    return result.join("; ");
}


DfdDataType dfdDataTypeFromDataType(const Channel &ch)
{DD;
    Descriptor::DataType d = ch.type();
    switch (d) {
        case Descriptor::TimeResponse : return CuttedData;
        case Descriptor::AutoSpectrum : return Spectr;
        case Descriptor::CrossSpectrum : {
            switch (ch.data()->yValuesFormat()) {
                case DataHolder::YValuesReals: return XSpectrRe;
                case DataHolder::YValuesImags: return XSpectrIm;
                case DataHolder::YValuesComplex:
                case DataHolder::YValuesAmplitudes:
                case DataHolder::YValuesAmplitudesInDB: return XSpectr;
                case DataHolder::YValuesPhases: return XPhase;
                default: return NotDef;
            }
        }
        case Descriptor::FrequencyResponseFunction : {
            switch (ch.data()->yValuesFormat()) {
                case DataHolder::YValuesReals: return TrFuncRe;
                case DataHolder::YValuesImags: return TrFuncIm;
                case DataHolder::YValuesComplex:
                case DataHolder::YValuesAmplitudes:
                case DataHolder::YValuesAmplitudesInDB:
                    return TransFunc;
                default: return NotDef;
            }
        }
        case Descriptor::Transmissibility : return TrFuncRe; //!
        case Descriptor::Coherence : return Coherence;
        case Descriptor::AutoCorrelation : return AutoCorr;
        case Descriptor::CrossCorrelation : return CrossCorr;
        case Descriptor::PowerSpectralDensity : return SpcDens;
        case Descriptor::EnergySpectralDensity : return NotDef; //не знает
        case Descriptor::ProbabilityDensityFunction : return HistP;
        case Descriptor::Spectrum : {
            switch (ch.data()->yValuesFormat()) {
                case DataHolder::YValuesComplex: return SpcDev;
                case DataHolder::YValuesAmplitudes:
                case DataHolder::YValuesAmplitudesInDB: {
                    switch (ch.octaveType()) {
                        case 1: return OSpectr;
                        case 2: return TwoOSpectr;
                        case 3: return ToSpectr;
                        case 6: return SixOSpectr;
                        case 12: return TwlOSpectr;
                        case 24: return TFOSpectr;
                        default: return SpcDev;
                    }
                }
                default: return NotDef;
            }
        }
        case Descriptor::FiniteImpulseResponseFilter: return FilterData;
        case Descriptor::CumulativeFrequencyDistribution: return EDF;
        default : return NotDef;
    }
    return NotDef;
}


Descriptor::DataType dataTypefromDfdDataType(DfdDataType type)
{DD;
    switch (type) {
        case SourceData:
        case CuttedData: return Descriptor::TimeResponse;
        case FilterData: return Descriptor::FiniteImpulseResponseFilter;
        case Envelope: return Descriptor::TimeResponse;
        case AutoCorr: return Descriptor::AutoCorrelation;
        case CrossCorr: return Descriptor::CrossCorrelation;
        case HistP: return Descriptor::ProbabilityDensityFunction;
        case Spectr: return Descriptor::AutoSpectrum;
        case SpcDens: return Descriptor::PowerSpectralDensity;
        case SpcDev: return Descriptor::Spectrum;
        case XSpectr:
        case XSpectrRe:
        case XSpectrIm:
        case XPhase:      return Descriptor::CrossSpectrum;
        case Coherence: return Descriptor::Coherence;
        case TransFunc:
        case TrFuncIm:
        case TrFuncRe: return Descriptor::FrequencyResponseFunction;
        case ToSpectr:
        case OSpectr:
        case TwoOSpectr:
        case SixOSpectr:
        case TwlOSpectr:
        case TFOSpectr: return Descriptor::Spectrum;
        case EDF: return Descriptor::CumulativeFrequencyDistribution;
        default: return Descriptor::Unknown;
    }

    return Descriptor::Unknown;
}


int DfdChannel::index() const
{
    return channelIndex;
}




bool DfdFileDescriptor::rename(const QString &newName, const QString &newPath)
{
    bool result = FileDescriptor::rename(newName, newPath);
    if (!result) return false;

    QString newRawName = changeFileExt(fileName(), "raw");

    result &= QFile::rename(rawFileName, newRawName);
    if (result) rawFileName = newRawName;
    return result;
}

DataHolder::YValuesFormat dataFormat(DfdDataType dataType, const QString& typeScale)
{
    switch (dataType) {
        case SourceData:		// исходные данные
        case CuttedData:		// вырезка из исходных данных
        case FilterData:		// фильтрованные данные
            return DataHolder::YValuesReals;
        case Envelope:		// огибающая по Гильберту
            return DataHolder::YValuesReals;
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
        case Histogram: 	// гистограмма
        case EDF:      		// эмпирическая функция распределения
        case Hist100:    		// гистограмма %
        case HistP:      		// плотность вероятности
            return DataHolder::YValuesReals;
        case Spectr: 		// спектр мощности
        case SpcDens:		// плотность спектра мощности
        case SpcDev:		// спектр СКЗ
        case XSpectr:		// взаимный спектр
        case TransFunc:	// передаточная функция
        case Cepstr:		// кепстр
        case OSpectr:		// октавный спектр
        case ToSpectr:		// 1/3-октавный спектр
        case TwoOSpectr:
        case SixOSpectr:
        case TwlOSpectr:
        case TFOSpectr:
        case GSpectr:  		// спектр Гильберта
        {    if (typeScale == "линейная" || typeScale == "уровень СКЗ (лин.)")
                return DataHolder::YValuesAmplitudes;
            else if (typeScale == "в децибелах" || typeScale == "уровень СКЗ (в дБ)")
                return DataHolder::YValuesAmplitudesInDB;
            else if (typeScale == "частота (Гц)") return DataHolder::YValuesReals;
            break;
        }
        case XPhase:		// взаимная фаза
            return DataHolder::YValuesPhases;
        case Coherence:	// когерентность
            return DataHolder::YValuesReals;
        case XSpectrRe:	// действ. часть взаимного спектра
            return DataHolder::YValuesReals;
        case XSpectrIm:		// мнимая часть взаимного спектра
            return DataHolder::YValuesImags;
        case TrFuncRe:	// действ. часть передаточной функции
            return DataHolder::YValuesReals;
        case TrFuncIm:		// мнимая часть передаточной функции
            return DataHolder::YValuesImags;
        //DiNike     = 152,		// диаграмма Найквиста для взаимных спектров
        //DiNikeP    = 154,       // диаграмма Найквиста для передаточной функции
        //GSpectr    = 155,		// спектр Гильберта

        default:
            break;
    }
    return DataHolder::YValuesUnknown;
}
