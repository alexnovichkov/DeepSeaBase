#include "ufffile.h"

#include <QMessageBox>
#include "logging.h"
#include "app.h"
#include "algorithms.h"
#include "dataholder.h"
#include "averaging.h"

QList<AbstractField*> fields = {
    new DelimiterField,
    new Float13_5Field,
    new Float15_7Field,
    new Float20_12Field,
    new Float25_17Field,
    new Integer4Field,
    new Integer5Field,
    new Integer6Field,
    new Integer10Field,
    new Integer12Field,
    new String80Field,
    new String10Field,
    new String10aField,
    new String20Field,
    new TimeDateField,
    new TimeDate80Field,
    new EmptyField
};

UffFileDescriptor::UffFileDescriptor(const QString &fileName) : FileDescriptor(fileName)
{DD;

}

UffFileDescriptor::UffFileDescriptor(const FileDescriptor &other, const QString &fileName,
                                     QVector<int> indexes)
    : FileDescriptor(fileName)
{
    setDataDescription(other.dataDescription());
    dataDescription().put("source.file", other.fileName());
    dataDescription().put("source.guid", other.dataDescription().get("guid"));
    dataDescription().put("source.dateTime", other.dataDescription().get("dateTime"));
    updateDateTimeGUID();

    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;
    dataDescription().put("source.channels", stringify(indexes));

    const int count = other.channelsCount();

    int referenceChannelNumber = -1; //номер опорного канала ("сила")
    QString referenceChannelName;

    //ищем силу и номер канала
    for (int i=0; i<count; ++i) {
        Channel *ch = other.channel(i);
        if (ch->xName().toLower()=="сила") {
            referenceChannelNumber = i;
            referenceChannelName = ch->name();
            break;
        }
    }

    //сохраняем файл, попутно подсасывая данные из other
    QFile uff(fileName);
    if (!uff.open(QFile::WriteOnly | QFile::Text)) {
        qDebug()<<"Couldn't open file"<<fileName<<"to write";
        return;
    }

    QTextStream stream(&uff);
    UffHeader h(dataDescription());
    h.write(stream);
    UffUnits u;
    u.write(stream);

    //заполнение каналов

    int id=1;
    for (int i=0; i<count; ++i) {
        if (!indexes.contains(i)) continue;

        Channel *ch = other.channel(i);
        bool populated = ch->populated();
        if (!populated) ch->populate();

        Function *f = new Function(*ch, this);
//        f->type58[15].value = i+1;
//        f->type58[10].value = QString("Record %1").arg(i+1);

        //заполнение инфы об опорном канале
        if (referenceChannelNumber>=0) {
            f->type58[18].value = referenceChannelName;
            f->type58[19].value = referenceChannelNumber+1;
        }

        f->type58[8].value = h.type151[10].value;

        f->write(stream, id);

        //clearing
        if (!populated) {
            ch->clear();
            f->clear();
        }
    }
}

UffFileDescriptor::~UffFileDescriptor()
{DD;
    if (changed() || dataChanged())
        write();

    qDeleteAll(channels);
}

void UffFileDescriptor::readWithStreams()
{
    qDebug()<<"Reading"<<fileName()<<"with streams";
    QFile uff(fileName());
    if (!uff.exists()) {
        qDebug()<<"Такого файла не существует";
        return;
    }

    if (uff.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&uff);

        UffHeader h;
        h.read(stream);
        setDataDescription(h.toDataDescription());
        UffUnits u;
        u.read(stream);

        while (!stream.atEnd()) {
            Function *f = new Function(this);
            f->read(stream, -1);
        }
    }
    else {
        qDebug()<<"Не удалось открыть файл"<<fileName();
    }
}

bool UffFileDescriptor::readWithMmap()
{
    qDebug()<<"Reading"<<fileName()<<"with mmap";
    QFile uff(fileName());
    if (!uff.exists()) return false;

    if (uff.open(QFile::ReadOnly)) {
        unsigned char *mapped = uff.map(0, uff.size());
        if (!mapped) return false;

        char *pos = reinterpret_cast<char*>(mapped);
        //char *beginning = pos;
        //char *end = pos+uff.size()-1;

        const qint64 size = uff.size();

        qint64 offset = 0;

        UffHeader h;
        h.read(pos, offset);
        setDataDescription(h.toDataDescription());
        UffUnits u;
        u.read(pos, offset);

        while (offset < uff.size()) {
            Function *f = new Function(this);
            f->read(pos, offset, size);
        }
    }
    return true;
}

void UffFileDescriptor::read()
{DD;
    //проверяем формат файлов uff:
    //если false - старый формат, удаляем файл и создаем заново
    int newUffFormat = App->getSetting("newUffFormat", 0).toInt();

    if (QFile::exists(fileName()+"~") && newUffFormat == 2) {
        // в папке с записью есть двоичный файл с описанием записи
        QFile uff(fileName()+"~");
        if (uff.open(QFile::ReadOnly)) {
            QDataStream stream(&uff);

            stream >> dataDescription();

            while (!stream.atEnd()) {
                Function *f = new Function(this);
                f->read(stream);
            }
        }
    }
    else
    {
        if (!readWithMmap())
            readWithStreams();

        //теперь уплощаем файл - группируем многоблочные каналы
        for (int i=channels.size()-1; i>=0; --i) {
            if (channels.at(i)->type58[16].value.toInt() > 1) {
                //это часть канала, а не целый канал
                if (i>0) {//TODO: оптимизировать
                    channels.at(i-1)->dataPositions.append(channels.at(i)->dataPositions);
                    channels.at(i-1)->dataEnds.append(channels.at(i)->dataEnds);
                    channels.at(i-1)->zValues.append(channels.at(i)->zValues);
                }
                delete channels.takeAt(i);
            }
        }

        for (Function *f: qAsConst(channels)) {
            f->readRest();
        }

        removeTempFile();
        QFile buff(fileName()+"~");
        if (buff.open(QFile::WriteOnly)) {
            QDataStream stream(&buff);

            stream << dataDescription();

            for (Function *f: channels) {
                stream << f->header;
                stream << f->type58;
                stream << f->dataPositions;
                stream << f->dataEnds;
                stream << f->zValues;
            }
        }
        App->setSetting("newUffFormat", 2);
    }
}

void UffFileDescriptor::write()
{DD;
    if (!changed() && !dataChanged()) return;

    QTemporaryFile tempFile;

    if (tempFile.open()) {
        QTextStream stream(&tempFile);

        UffHeader h(dataDescription());
        h.write(stream);
        UffUnits u;
        u.write(stream);

        int id=1;
        for (Function *c: channels) {
            bool populated = c->populated();
            if (!populated) c->populate();
            c->write(stream, id);
            if (!populated) c->clear();
            c->setChanged(false);
            c->setDataChanged(false);
        }

        tempFile.close();
        removeTempFile();
        QFile::remove(fileName());
        QFile::copy(tempFile.fileName(), fileName());

        setChanged(false);
        setDataChanged(false);
    }
    else {
        qDebug()<<"Couldn't open file"<<fileName()<<"to write";
    }
}

QString makeStringFromPair(const QPair<QString, QString> &pair)
{
    QString result = pair.second;
    if (!pair.first.isEmpty()) {
        result = pair.first+"="+result;
    }
    result.truncate(80);
    return result;
}

void UffFileDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    QTemporaryFile temp;
    if (!temp.open()) {
        qDebug()<<"Couldn't open file"<<fileName()<<"to write";
        return;
    }

    QTextStream stream (&temp);
    UffHeader h(dataDescription());
    h.write(stream);
    UffUnits u;
    u.write(stream);

    int id=1;
    for (int i = 0; i < channels.size(); ++i) {
        if (!channelsToDelete.contains(i)) continue;

        Function *c = channels.at(i);
        bool populated = c->populated();
        if (!populated) c->populate();

        c->write(stream, id);
        if (!populated) c->clear();

        c->setChanged(false);
        c->setDataChanged(false);
    }

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    temp.close();
    removeTempFile();
    QFile::remove(fileName());
    QFile::copy(temp.fileName(), fileName());

    setChanged(false);
    setDataChanged(false);
}

void UffFileDescriptor::removeTempFile()
{
    if (QFile::exists(fileName()+"~")) QFile::remove(fileName()+"~");
}

void UffFileDescriptor::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{DD;
    int referenceChannelNumber = -1; //номер опорного канала ("сила")
    QString referenceChannelName;

    QFile uff(fileName());
    if (!uff.open(QFile::Append | QFile::Text)) {
        qDebug()<<"Couldn't open file to write";
        return;
    }

    dataDescription().put("source.channels", stringify(indexes));

    QTextStream stream(&uff);

    //добавляем каналы
    int id=0;
    for (Function *f: qAsConst(channels)) {
        id += f->data()->blocksCount();
    }

    for (int i: indexes) {
        Channel *f = sourceFile->channel(i);
        bool populated = f->populated();
        if (!populated) f->populate();

        Function *ch = new Function(*f, this);

        //заполнение инфы об опорном канале
        if (referenceChannelNumber>=0) {
            ch->type58[18].value = referenceChannelName;
            ch->type58[19].value = referenceChannelNumber+1;
        }

        ch->type58[8].value = dataDescription().get("dateTime");

        ch->write(stream, id);

        //clearing
        if (!populated) {
            f->clear();
            ch->clear();
        }
    }

    removeTempFile();
}

void UffFileDescriptor::addChannelWithData(DataHolder *data, const QJsonObject &description)
{
    // обновляем сведения канала
    Function *ch = new Function(this);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setPopulated(true);
    ch->setData(data);

    int octave = description.value("function").toObject().value("octaveFormat").toInt();
    ch->header.type1858[5].value = octave;

    ch->setName(description.value("name").toString());
    ch->type58[8].value = QDateTime::currentDateTime();
    ch->setDescription(description.value("description").toString());


    if (data->yValuesFormat() == DataHolder::YValuesAmplitudesInDB) {
        // записываем в файлы uff только линейные данные
        auto d = DataHolder::fromLog(data->rawYValues(-1), data->threshold(), data->yValuesUnits());
        ch->data()->setYValues(d, DataHolder::YValuesAmplitudes);
    }

    ch->type58[14].value = description.value("type").toInt();

    ch->type58[25].value = (data->yValuesFormat() == DataHolder::YValuesComplex ? 6 : 4);
    ch->type58[26].value = ch->data()->samplesCount();
    ch->type58[27].value = data->xValuesFormat()==DataHolder::XValuesUniform ? 1 : 0;
    ch->type58[28].value = data->xMin();
    ch->type58[29].value = data->xStep();

    ch->type58[32].value = abscissaType(description.value("xName").toString());
    ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt());
    ch->type58[37].value = description.value("xName");

    ch->type58[44].value = description.value("yName");
}

void UffFileDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    // заполняем вектор индексов каналов, как они будут выглядеть после перемещения
    const int count = channelsCount();
    QVector<int> indexesVector(count);
    for (int i=0; i<count; ++i) indexesVector[i] = i;

    {int i=up?0:indexes.size()-1;
    while (1) {
        indexesVector.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }}

    QTemporaryFile uff;
    if (!uff.open()) {
        qDebug()<<"Couldn't open file to write";
        return;
    }

    QTextStream stream(&uff);
    UffHeader h(dataDescription());
    h.write(stream);
    UffUnits u;
    u.write(stream);

    int id=1;
    for (int i=0; i<indexesVector.count(); ++i) {
        Function *f = channels.at(indexesVector.at(i));
        bool populated = f->populated();
        if (!populated) f->populate();

        f->write(stream, id);

        //clearing
        if (!populated) f->clear();
    }
    uff.close();

    int i=up?0:indexes.size()-1;
    while (1) {
        channels.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }

    QFile::remove(fileName());
    uff.copy(fileName());
    removeTempFile();
}

int UffFileDescriptor::channelsCount() const
{DD;
    return channels.size();
}

Channel *UffFileDescriptor::channel(int index) const
{
    if (channels.size()>index)
        return channels[index];
    return 0;
}

bool UffFileDescriptor::operator ==(const FileDescriptor &descriptor)
{DD;
    return this->fileName() == descriptor.fileName();
}

QStringList UffFileDescriptor::fileFilters()
{DD;
    return QStringList()<< "Файлы uff (*.uff)";
}

QStringList UffFileDescriptor::suffixes()
{
    return QStringList()<<"*.uff";
}


UffHeader::UffHeader()
{DD;
    setType151(type151);
}

UffHeader::UffHeader(const DataDescription &data)
{
    setType151(type151);
    type151[4].value = data.get("source.file");
    //type151[6].value = "NONE";
    type151[8].value = data.get("legend");
    type151[10].value = data.get("dateTime");
    type151[12].value = data.get("dateTime");
    type151[14].value = data.get("createdBy");
    type151[16].value = data.get("fileCreationTime");
}

void UffHeader::read(QTextStream &stream)
{DD;
    for (int i=0; i<20; ++i) {
        fields[type151[i].type]->read(type151[i].value, stream);
//        qDebug() << i << type151[i].value;
    }

}

void UffHeader::read(char *pos, qint64 &offset)
{
    for (int i=0; i<20; ++i) {
        //qDebug()<<"pos at"<<offset;
        offset += fields[type151[i].type]->read(type151[i].value, pos, offset);
//        qDebug() << i << type151[i].value;
    }
}

void UffHeader::write(QTextStream &stream)
{DD;
    type151[16].value = QDateTime::currentDateTime();

    for (int i=0; i<20; ++i) {
        fields[type151[i].type]->print(type151[i].value, stream);
    }
}

QString UffHeader::info() const
{DD;
    return type151[4].value.toString()+" "+type151[6].value.toString();
}

DataDescription UffHeader::toDataDescription() const
{
    DataDescription data;
    data.put("source.file", type151[4].value);
    //type151[6].value = "NONE";
    data.put("legend", type151[8].value);
    data.put("dateTime", type151[10].value);
    data.put("createdBy",type151[14].value);
    data.put("fileCreationTime", type151[16].value);

    return data;
}


UffUnits::UffUnits()
{DD;
    setType164(type164);
}

void UffUnits::read(QTextStream &stream)
{DD;
    for (int i=0; i<14; ++i) {
        fields[type164[i].type]->read(type164[i].value, stream);
    }
}

void UffUnits::read(char *pos, qint64 &offset)
{
    for (int i=0; i<14; ++i) {
        //qDebug()<<"pos at"<<offset;
        offset += fields[type164[i].type]->read(type164[i].value, pos, offset);
    }
}

void UffUnits::write(QTextStream &stream)
{DD;
    for (int i=0; i<14; ++i) {
        fields[type164[i].type]->print(type164[i].value, stream);
    }
}


FunctionHeader::FunctionHeader()
{DD;
    setType1858(type1858);
    valid = true;
}

void FunctionHeader::read(QTextStream &stream)
{
    for (int i=0; i<4; ++i) {
        fields[type1858[i].type]->read(type1858[i].value, stream);
    }
    if (type1858[2].value.toInt()==1858) {
        for (int i=4; i<48; ++i) {
            fields[type1858[i].type]->read(type1858[i].value, stream);
        }
    }
    else {
        valid = false;
        type1858[2].value = 1858;
    }
}

void FunctionHeader::read(char *data, qint64 &offset)
{DD;
    for (int i=0; i<4; ++i) {
        offset += fields[type1858[i].type]->read(type1858[i].value, data, offset);
    }
    if (type1858[2].value.toInt()==1858) {
        for (int i=4; i<48; ++i) {
            offset += fields[type1858[i].type]->read(type1858[i].value, data, offset);
        }
    }
    else {
        valid = false;
        type1858[2].value = 1858;
    }
}

void FunctionHeader::write(QTextStream &stream)
{DD;
    for (int i=0; i<48; ++i) {
        fields[type1858[i].type]->print(type1858[i].value, stream);
    }
}

QDataStream &operator>>(QDataStream &stream, FunctionHeader &header)
{
    stream >> header.type1858;
    stream >> header.valid;
    return stream;
}


Function::Function(UffFileDescriptor *parent) : Channel(),
    parent(parent)
{DD;
    setType58(type58);
    parent->channels << this;
}



Function::Function(Channel &other, UffFileDescriptor *parent) : Channel(other), parent(parent)
{DD;
    parent->channels << this;
    header.type1858[5].value = other.octaveType();
    ///TODO: заполнение поля 1858 данными обработки: окно, взвешивание и т.д.

    setType58(type58);

    if (!other.name().isEmpty()) type58[4].value = other.name();
    if (!other.description().isEmpty()) type58[6].value = other.description();
    type58[8].value = QDateTime::currentDateTime();
    type58[14].value = other.type();

    if (other.data()->yValuesFormat()== DataHolder::YValuesComplex)
        type58[25].value = 6; // 5 - complex, single precision
    else
        type58[25].value = 4; // 2 - real, single precision

    type58[26].value = other.data()->samplesCount();
    type58[27].value = other.data()->xValuesFormat() == DataHolder::XValuesNonUniform ? 0 : 1;
    type58[28].value = other.data()->xMin();
    type58[29].value = other.data()->xStep();
//    type58[30].value = other.datzValue();

    type58[32].value = abscissaType(other.xName());
    type58[36].value = abscissaTypeDescription(type58[32].value.toInt());
    if (!other.xName().isEmpty()) type58[37].value = other.xName();

    type58[39].value = abscissaType(other.yName());
    type58[43].value = abscissaTypeDescription(type58[39].value.toInt());
    if (!other.yName().isEmpty()) type58[44].value = other.yName();

    type58[53].value = abscissaType(other.zName());
    type58[57].value = abscissaTypeDescription(type58[53].value.toInt());
    if (!other.zName().isEmpty()) type58[58].value = other.zName();

    dataPositions.clear(); dataEnds.clear();
}

Function::Function(Function &other, UffFileDescriptor *parent) : Channel(other), parent(parent)
{DD;
    parent->channels << this;
    header = other.header;

    type58 = other.type58;
    dataPositions.clear(); dataEnds.clear();
}

Function::~Function()
{

}

void Function::read(QTextStream &stream, qint64 pos)
{DD;
    if (pos != -1) stream.seek(pos);

    dataPositions.clear(); dataEnds.clear();
    zValues.clear();

    header.read(stream);
    int i=0;
    if (!header.valid) {
        i=4;
        type58[2].value=58;
    }

    for (; i<60; ++i) {
        fields[type58[i].type]->read(type58[i].value, stream);
    }
    //первое положение данных
    dataPositions << stream.pos();
    zValues << type58[30].value.toDouble();

    if (pos == -1) {
        QString s;
        do {
            s = stream.readLine().trimmed();
            if (stream.atEnd()) s= "-1";
        }
        while (s != "-1");
    }
    dataEnds << stream.pos() - 6-2-2;

    //readRest();
}

void Function::read(char *data, qint64 &offset, int size)
{DD;
    dataPositions.clear();  dataEnds.clear();
    zValues.clear();

    header.read(data, offset);
    int i=0;
    if (!header.valid) {
        i=4;
        type58[2].value=58;
    }

    for (; i<60; ++i) {
        offset += fields[type58[i].type]->read(type58[i].value, data, offset);
    }
    //первое положение данных
    dataPositions << qint64(offset);
    zValues << type58[30].value.toDouble();

    for (; offset <= size-6; ++offset) {
        if (*(data+offset  ) == ' ' &&
            *(data+offset+1) == ' ' &&
            *(data+offset+2) == ' ' &&
            *(data+offset+3) == ' ' &&
            *(data+offset+4) == '-' &&
            *(data+offset+5) == '1') break;
    }
    dataEnds << qint64(offset);
    //qDebug()<<"found data delimiter at"<<offset;
    if (offset == size-6) {
        qDebug()<<"Reached end of file, no trailing \"    -1\" found";
    }
    offset+=6;

    while (*(data+offset) == '\n' || *(data+offset) == '\r') {
        offset++;
    }
    //qDebug()<<"data end at"<<offset;
}

void Function::read(QDataStream &stream)
{
    stream >> header;
    stream >> type58;
    stream >> dataPositions;
    stream >> dataEnds;
    stream >> zValues;

    readRest();
}

void Function::readRest()
{
    if (type58[27].value.toInt() == 1) {// abscissa, even spacing
        _data->setXValues(type58[28].value.toDouble(),
                type58[29].value.toDouble(),
                type58[26].value.toInt());
    }
    else {// abscissa, uneven spacing
        _data->setSamplesCount(type58[26].value.toInt());
        populate();
    }

    double thr = threshold(yName());
    if (type()==Descriptor::FrequencyResponseFunction) thr = 1.0;
    _data->setThreshold(thr);

    DataHolder::YValuesFormat yValueFormat = DataHolder::YValuesReals;

    int ftype = type58[14].value.toInt();
    switch (ftype) {
        case 0: yValueFormat = DataHolder::YValuesReals; break; // 0 - General or Unknown
        case 1: yValueFormat = DataHolder::YValuesReals; break; // 1 - Time Response
        case 2: yValueFormat = DataHolder::YValuesAmplitudes; break; // 2 - Auto Spectrum
        case 3: yValueFormat = DataHolder::YValuesAmplitudes; break; // 3 - Cross Spectrum
        case 4: yValueFormat = DataHolder::YValuesAmplitudes; break; // 4 - Frequency Response Function
        case 5: yValueFormat = DataHolder::YValuesReals; break; // 5 - Transmissibility
        case 6: yValueFormat = DataHolder::YValuesReals; break; // 6 - Coherence
        case 7: yValueFormat = DataHolder::YValuesReals; break; // 7 - Auto Correlation
        case 8: yValueFormat = DataHolder::YValuesReals; break; // 8 - Cross Correlation
        case 9: yValueFormat = DataHolder::YValuesAmplitudes; break; // 9 - Power Spectral Density (PSD)
        case 10: yValueFormat = DataHolder::YValuesAmplitudes; break; // 10 - Energy Spectral Density (ESD)
        case 11: yValueFormat = DataHolder::YValuesAmplitudes; break; // 11 - Probability Density Function
        case 12: yValueFormat = DataHolder::YValuesAmplitudes; break; // 12 - Spectrum
        case 13: yValueFormat = DataHolder::YValuesAmplitudes; break; // 13 - Cumulative Frequency Distribution
        case 14: yValueFormat = DataHolder::YValuesReals; break; // 14 - Peaks Valley
        case 15: yValueFormat = DataHolder::YValuesReals; break; // 15 - Stress/Cycles
        case 16: yValueFormat = DataHolder::YValuesReals; break; // 16 - Strain/Cycles
        case 17: yValueFormat = DataHolder::YValuesReals; break; // 17 - Orbit
        case 18: yValueFormat = DataHolder::YValuesReals; break; // 18 - Mode Indicator Function
        case 19: yValueFormat = DataHolder::YValuesReals; break; // 19 - Force Pattern
        case 20: yValueFormat = DataHolder::YValuesReals; break; // 20 - Partial Power
        case 21: yValueFormat = DataHolder::YValuesReals; break; // 21 - Partial Coherence
        case 22: yValueFormat = DataHolder::YValuesReals; break; // 22 - Eigenvalue
        case 23: yValueFormat = DataHolder::YValuesReals; break; // 23 - Eigenvector
        case 24: yValueFormat = DataHolder::YValuesAmplitudes; break; // 24 - Shock Response Spectrum
        case 25: yValueFormat = DataHolder::YValuesReals; break; // 25 - Finite Impulse Response Filter
        case 26: yValueFormat = DataHolder::YValuesReals; break; // 26 - Multiple Coherence
        case 27: yValueFormat = DataHolder::YValuesReals; break; // 27 - Order Function
        default: break;
    }
    if (type58[25].value.toInt() >= 5) yValueFormat = DataHolder::YValuesComplex;

    if (yName()=="dB" || yName()=="дБ" || type58[43].value.toString()=="Уровень")
        yValueFormat = DataHolder::YValuesAmplitudesInDB;

    if (type58[43].value.toString()=="Phase")
        yValueFormat = DataHolder::YValuesPhases;

    _data->setYValuesFormat(yValueFormat);

    int units = DataHolder::UnitsLinear;
    if (ftype == 9/*PSD*/ ||
        ftype == 10/*ESD*/ ||
        ftype == 2 /*auto power spectrum*/ ||
        ftype == 3 /*cross spectrum*/) units = DataHolder::UnitsQuadratic;
//    else if (ftype == 3 /*cross spectrum*/) {
//        //14 normalization method, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
//        //                         3=Units squared seconds per Hz (ESD)
//        if (header.type1858[14].value.toInt() > 0) units = DataHolder::UnitsQuadratic;
//    }
    _data->setYValuesUnits(units);


    double zBegin = 0.0;
    double zStep = 0.0;
    int zCount = zValues.size();

    zBegin = zValues.at(0);

    if (zCount == 1) {
        //одиночный канал
        _data->setZValues(zBegin, zStep, zCount);
    }
    else {
        //определяем, равномерная ли шкала
        if (zCount >= 2) zStep = zValues.at(1) - zValues.at(0);

        bool uniform = true;
        for (int i=2; i<zCount; ++i) {
            if (!qFuzzyIsNull(zValues.at(i) - zValues.at(i-1) - zStep)) {
                uniform = false;
                break;
            }
        }
        if (uniform)
            _data->setZValues(zBegin, zStep, zCount);
        else
            _data->setZValues(zValues);
    }
}


void Function::write(QTextStream &stream, int &id)
{DD;
    int samples = data()->samplesCount();
    int blocks = data()->blocksCount();
    dataPositions.clear();  dataEnds.clear();

    type58[26].value = data()->samplesCount();
    type58[29].value = data()->xStep();

    for (int block = 0; block < blocks; ++block) {
        //writing header
        FunctionHeader h = header;
        h.type1858[4].value = block+1;
        h.write(stream);

        auto t58 = type58;
        t58[10].value = QString("Record %1").arg(block+1);
        t58[15].value = id+block;
        t58[16].value = block+1;
        t58[30].value = data()->zValue(block);


        for (int i=0; i<60; ++i) {
            fields[t58[i].type]->print(t58[i].value, stream);
        }
        dataPositions << stream.pos();

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
                QVector<double> values = data()->rawYValues(block);
                int j = 0;
                const int format = data()->xValuesFormat();
                for (int i=0; i<samples; ++i) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data()->xValue(i), stream);
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
                QVector<double> values = data()->rawYValues(block);
                int j = 0;
                const int format = data()->xValuesFormat();
                for (int i=0; i<samples; ++i) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data()->xValue(i), stream);
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
                auto values = data()->yValuesComplex(block);
                int j = 0;
                const int format = data()->xValuesFormat();
                for (int i=0; i<samples; i++) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data()->xValue(i), stream);
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
                auto values = data()->yValuesComplex(block);
                int j = 0;
                const int format = data()->xValuesFormat();
                int limit = format == DataHolder::XValuesNonUniform ? 3 : 4;
                for (int i=0; i<samples; i++) {
                    if (format == DataHolder::XValuesNonUniform) {
                        fields[FTFloat13_5]->print(data()->xValue(i), stream);
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
        dataEnds << stream.pos();
        fields[FTDelimiter]->print("", stream);
        fields[FTEmpty]->print(0, stream);
    }
    id += blocks;
}

FileDescriptor *Function::descriptor()
{
     return parent;
}

QVariant Function::info(int column, bool edit) const
{
    Q_UNUSED(edit)
    switch (column) {
        case 0: return type58[4].value; //name(); //avoiding conversion variant->string->variant
        case 1: return type58[44].value; //yName();
        case 2: return data()->yValuesFormatString();
        case 3: return type58[6].value; //description();
        case 4: return functionTypeDescription(type());
        case 5: return data()->blocksCount();
        case 6: return correction();
        default: ;
    }
    return QVariant();
}

int Function::columnsCount() const
{
    return 7;
}

QVariant Function::channelHeader(int column) const
{
    switch (column) {
        case 0: return QString("Имя");
        case 1: return QString("Ед.изм.");
        case 2: return QString("Формат");
        case 3: return QString("Описание");
        case 4: return QString("Функция");
        case 5: return QString("Кол-во блоков");
        case 6: return QString("Коррекция");
        default: return QVariant();
    }
    return QVariant();
}

Descriptor::DataType Function::type() const
{
    return (Descriptor::DataType)type58[14].value.toInt();
}

int Function::octaveType() const
{
    return header.type1858[5].value.toInt();
}

bool Function::populateWithMmap()
{
    QFile uff(parent->fileName());
    if (!uff.open(QFile::ReadOnly)) {
        qDebug()<<"Не удалось открыть файл"<<parent->fileName();
        return false;
    }

    uchar *mmap = uff.map(0, uff.size());
    if (!mmap) {
        qDebug()<<"Ошибка чтения данных";
        return false;
    }
    char *data = reinterpret_cast<char*>(mmap);

    Q_ASSERT_X (dataPositions.first() != -1, "Function::populate", "Data positions have been invalidated");
    Q_ASSERT_X (_data->blocksCount() == dataPositions.size(), "Function::populate",
                "Data positions не соответствуют количеству блоков");

    for (int block = 0; block < _data->blocksCount(); ++block) {
//        qDebug()<<"reading block"<<block+1;
        qint64 pos = dataPositions.at(block);
        qint64 len = dataEnds.at(block) - pos;
//        qDebug()<<"reding pos"<<pos<<"of length"<<len;

        std::vector<double> vals;

        std::string str = std::string(data+pos, data+pos+len);
        strtk::parse(str, " \r\n", vals);
//        qDebug()<<vals.size()<<"vals parsed";

        QVector<double> values;
        QVector<double> xvalues;
        QVector<cx_double> valuesComplex;

        if (type58[25].value.toInt() < 5) {//real values
            if (type58[27].value.toInt() == 0) {// uneven abscissa
                for (uint i=0; i<vals.size(); ++i) {
                    if (i%2==0) xvalues << vals[i];
                    else values << vals[i];
                }
            }
            else {
                values = QVector<double>::fromStdVector(vals);
            }
        }
        else {//complex values
            if (type58[27].value.toInt() == 0) {// uneven abscissa
                for (uint i=0; i<vals.size()-2; i+=3) {
                    xvalues << vals[i];
                    valuesComplex << cx_double(vals[i+1], vals[i+2]);
                }
            }
            else {
                for (uint i=0; i<vals.size()-1; i+=2) {
                    valuesComplex << cx_double(vals[i], vals[i+1]);
                }
            }
        }
//        qDebug()<<"values size"<<values.size();
//        qDebug()<<"xvalues size"<<xvalues.size();
//        qDebug()<<"complex values size"<<valuesComplex.size();

        if (type58[27].value.toInt() == 0) {// uneven abscissa
            _data->setXValues(xvalues);
        }
        if (type58[25].value.toInt() < 5) {//real values
            _data->setYValues(values, _data->yValuesFormat(), block);
        }
        else
            _data->setYValues(valuesComplex, block);
    }
    uff.unmap(mmap);
    return true;
}

bool Function::populateWithStream()
{
    QFile uff(parent->fileName());
    if (!uff.open(QFile::ReadOnly | QFile::Text)) {
        qDebug()<<"Не удалось открыть файл"<<parent->fileName();
        return false;
    }

    int sc = data()->samplesCount();


    Q_ASSERT_X (dataPositions.first() != -1, "Function::populate", "Data positions have been invalidated");
    Q_ASSERT_X (_data->blocksCount() == dataPositions.size(), "Function::populate",
                "Data positions не соответствуют количеству блоков");

    QTextStream stream(&uff);
    for (int block = 0; block < _data->blocksCount(); ++block) {
//        qDebug()<<"reading block"<<block+1;
        if (stream.seek(dataPositions.at(block))) {
//            qDebug()<<"reding pos"<<dataPositions.at(block);

            QVector<double> values, xvalues;
            QVector<cx_double> valuesComplex;

            if (_data->yValuesFormat() == DataHolder::YValuesComplex) { //complex values
                valuesComplex = QVector<cx_double>(sc, cx_double());
            }
            else
                values = QVector<double>(sc, 0.0);

            if (type58[27].value.toInt() == 0) {
                // uneven abscissa, read data pairs
                xvalues = QVector<double>(sc, 0.0);
            }
            if (type58[25].value.toInt() < 5) {//real values
                int j=0;
                for (int i=0; i<sc; ++i) {
                    double value;
                    stream >> value;

                    if (type58[27].value.toInt() == 0) {// uneven abscissa
                        xvalues[j] = value;
                        stream >> value;
                    }

                    values[j] = value;
                    j++;
                }
            }
            else {//complex values
                double first, second;
                int j=0;
                for (int i=0; i<sc; ++i) {
                    if (type58[27].value.toInt() == 0) {// uneven abscissa
                        stream >> first;
                        xvalues[j] = first;
                    }
                    stream >> first >> second; //qDebug()<<first<<second;
                    valuesComplex[j] = {first, second};
                    j++;
                }
            }
            if (type58[27].value.toInt() == 0) {// uneven abscissa
                _data->setXValues(xvalues);
            }
            if (type58[25].value.toInt() < 5) {//real values
                _data->setYValues(values, _data->yValuesFormat(), block);
            }
            else
                _data->setYValues(valuesComplex, block);

//            qDebug()<<"values size"<<values.size();
//            qDebug()<<"xvalues size"<<xvalues.size();
//            qDebug()<<"complex values size"<<valuesComplex.size();

            QString end = stream.readLine();
            end = stream.readLine().trimmed();
            if (end != "-1") {
                qDebug()<<"ERROR:"<<parent->fileName()<<"channel"<<this->index()
                       <<"ends abruptly. Check the file!";
            }
        }
    }
    return true;
}

void Function::populate()
{DD;
    _data->clear();

    setPopulated(false);
    if (!populateWithMmap()) {
        if (populateWithStream())
            setPopulated(true);
    }
    else
        setPopulated(true);
}

QString Function::name() const
{DD;
    QString s = type58[4].value.toString();
    if (s == "NONE") return "";
    return s;
}

void Function::setName(const QString &name)
{DD;
    type58[4].value = name.isEmpty()?"NONE":name;
}

QString Function::description() const
{DD;
    QString s = type58[6].value.toString();
    if (s == "NONE") return "";
    return s;
}

void Function::setDescription(const QString &description)
{DD;
    type58[6].value = description.isEmpty()?"NONE":description;
}

QString Function::xName() const
{
    QString s = type58[37].value.toString();
    if (s == "NONE") return "";
    return s;
}

QString Function::yName() const
{DD;
    QString s = type58[44].value.toString();
    if (s == "NONE") return "";
    return s;
}

QString Function::zName() const
{
    QString s = type58[58].value.toString();
    if (s == "NONE") return "";
    return s;
}

void Function::setYName(const QString &yName)
{
    type58[44].value = yName.isEmpty()?"NONE":yName;
}

void Function::setXName(const QString &xName)
{
    type58[37].value = xName.isEmpty()?"NONE":xName;
}

void Function::setZName(const QString &zName)
{
    type58[58].value = zName.isEmpty()?"NONE":zName;
}

QString Function::legendName() const
{DD;
    QStringList l;
    l << name();
    if (!correction().isEmpty()) l << correction();
    if (!parent->legend().isEmpty()) l << parent->legend();

    return l.join(" ");
}

void Function::setCorrection(const QString &s)
{
    Channel::setCorrection(s);
    type58[12].value = s;
}

//QString UffFileDescriptor::saveTimeSegment(double from, double to)
//{DD;
//    // 0 проверяем, чтобы этот файл имел тип временных данных
//    if (type() != Descriptor::TimeResponse) return "";
//    // и имел данные
//    if (channels.size() == 0) return "";

//    // 1 создаем уникальное имя файла по параметрам from и to
//    QString fromString, toString;
//    getUniqueFromToValues(fromString, toString, from, to);
//    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

//    QString newFileName = createUniqueFileName("", fileName(), suffix, "uff", false);

//    // 2 создаем новый файл
//    UffFileDescriptor *newUff = new UffFileDescriptor(*this);

//    // 3 ищем границы данных по параметрам from и to
//    Channel *ch = channels.constFirst();

//    int sampleStart = qRound((from - ch->data()->xMin())/ch->data()->xStep());
//    if (sampleStart<0) sampleStart = 0;
//    int sampleEnd = qRound((to - ch->data()->xMin())/ch->data()->xStep());
//    if (sampleEnd >= ch->data()->samplesCount()) sampleEnd = ch->data()->samplesCount() - 1;
////    newUff->setSamplesCount(sampleEnd - sampleStart + 1); //число отсчетов в новом файле

//    // 4 сохраняем файл

//    for (int i=0; i<channels.size(); ++i) {
//        bool wasPopulated = channels[i]->populated();
//        if (!wasPopulated) channels[i]->populate();

//        Function *ch = new Function(*(this->channels[i]), newUff);
//        ch->data()->setSegment(*(channels[i]->data()), sampleStart, sampleEnd);
//        ch->setPopulated(true);
//        if (!wasPopulated) {
//            //clearing data
//            channels[i]->data()->clear();
//        }
//    }

//    newUff->setChanged(true);
//    newUff->setDataChanged(true);
//    newUff->write();
//    delete newUff;

//    // 5 возвращаем имя нового файла
//    return newFileName;
//}

void UffFileDescriptor::setChanged(bool changed)
{DD;
    FileDescriptor::setChanged(changed);
    if (changed) removeTempFile();
}


QDataStream &operator>>(QDataStream &stream, UffHeader &header)
{
    stream >> header.type151;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const UffHeader &header)
{
    stream << header.type151;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, UffUnits &header)
{
    stream >> header.type164;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const UffUnits &header)
{
    stream << header.type164;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const FunctionHeader &header)
{
    stream << header.type1858;
    stream << header.valid;
    return stream;
}




int Function::index() const
{
    return parent->channels.indexOf(const_cast<Function*>(this));
}
