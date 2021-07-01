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
    else
        dataDescription().put("source.channels", stringify(indexes));

    const int count = other.channelsCount();

    int referenceChannelNumber = -1; //номер опорного канала ("сила")
    QString referenceChannelName;

    //ищем силу и номер канала
    for (int i=0; i<count; ++i) {
        Channel *ch = other.channel(i);
        if (ch->xName().toLower()=="сила" || ch->xName().toLower()=="sila") {
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
    for (int i: indexes) {
        Channel *ch = other.channel(i);
        bool populated = ch->populated();
        if (!populated) ch->populate();

        Function *f = new Function(*ch, this);

        //заполнение инфы об опорном канале
        if (referenceChannelNumber>=0) {
            f->dataDescription().put("referenceName", referenceChannelName);
            f->dataDescription().put("referenceNode", referenceChannelNumber+1);
        }

        f->write(stream, id);

        //clearing
        if (!populated) ch->clear();
        f->clear();
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

        //not needed, just read through
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
    QFile uff(fileName());
    if (!uff.exists()) return false;

    if (uff.open(QFile::ReadOnly)) {
        unsigned char *mapped = uff.map(0, uff.size());
        if (!mapped) return false;

        char *pos = reinterpret_cast<char*>(mapped);
        const qint64 size = uff.size();

        qint64 offset = 0;

        UffHeader h;
        h.read(pos, offset);
        setDataDescription(h.toDataDescription());

        //not needed, just read through
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

    if (QFile::exists(fileName()+QString("~%1").arg(newUffFormat))) {
        // в папке с записью есть двоичный файл с описанием записи
        QFile uff(fileName()+QString("~%1").arg(newUffFormat));
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
            bool sequence = channels.at(i)->dataDescription().get("sequence").toInt() > 1;
            channels.at(i)->dataDescription().data.remove("sequence");
            if (sequence) {
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
        QFile buff(fileName()+QString("~%1").arg(newUffFormat));
        if (buff.open(QFile::WriteOnly)) {
            QDataStream stream(&buff);

            stream << dataDescription();

            for (Function *f: channels) {
                stream << f->dataDescription();
                stream << f->dataPositions;
                stream << f->dataEnds;
                stream << f->zValues;
            }
        }
        App->setSetting("newUffFormat", 3);
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
    int newUffFormat = App->getSetting("newUffFormat", 0).toInt();
    QString name = fileName()+QString("~%1").arg(newUffFormat);
    if (QFile::exists(name)) QFile::remove(name);
}

void UffFileDescriptor::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{DD;
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
        ch->write(stream, id);

        //clearing
        if (!populated) {
            f->clear();
            ch->clear();
        }
    }

    removeTempFile();
}

void UffFileDescriptor::addChannelWithData(DataHolder *data, const DataDescription &description)
{
    // обновляем сведения канала
    Function *ch = new Function(this);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setPopulated(true);
    ch->setData(data);
    ch->dataDescription() = description;

    ch->dataDescription().put("dateTime", QDateTime::currentDateTime());

    if (data->yValuesFormat() == DataHolder::YValuesAmplitudesInDB) {
        // записываем в файлы uff только линейные данные
        auto d = DataHolder::fromLog(data->rawYValues(-1), data->threshold(), data->yValuesUnits());
        ch->data()->setYValues(d, DataHolder::YValuesAmplitudes);
        ch->dataDescription().put("function.format", "amplitude");
    }
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
    qint64 offs = stream.pos();
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
        stream.seek(offs); // перемещаемся в начало, как будто и не читали
    }
}

void FunctionHeader::read(char *data, qint64 &offset)
{DD;
    qint64 offs = offset;
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
        offset = offs; // перемещаемся в начало, как будто и не читали
    }
}

void FunctionHeader::write(QTextStream &stream)
{DD;
    for (int i=0; i<48; ++i) {
        fields[type1858[i].type]->print(type1858[i].value, stream);
    }
}

void FunctionHeader::toDataDescription(DataDescription &d)
{
    //{FTInteger12, 1}, //4 set record number
    if (int v = type1858[5].value.toInt(); v!=0) {
        d.put("function.octaveFormat", v);
    }
    //{FTInteger12, 0}, //6 measurement run number
    if (int v = type1858[11].value.toInt(); v!=0) {
        d.put("function.weighting", weightingFromType(v));
    }
    if (int v = type1858[12].value.toInt(); v!=0) {
        d.put("function.window", windowDescriptionFromUffType(v));
    }
    if (int v = type1858[13].value.toInt(); v!=0) {
        d.put("function.amplitudeScaling", scalingDescriptionFromUffType(v));
    }
    if (int v = type1858[14].value.toInt(); v!=0) {
        d.put("function.normalization", normalizationDescriptionFromUffType(v));
    }
    //{FTInteger6, 0}, //15  Abscissa Data Type Qualifier,
                      //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    //{FTInteger6, 0}, //16 Ordinate Numerator Data Type Qualifier, see 15
    //{FTInteger6, 0}, //17 Ordinate Denominator Data Type Qualifier, see 15
    //{FTInteger6, 0}, //18 Z-axis Data Type Qualifier, see 15
    //{FTInteger6, 0}, //19 sampling type, 0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
    //{FTFloat15_7, 0.0}, //24 Z RPM value
    //{FTFloat15_7, 0.0}, //25 Z time value
    //{FTFloat15_7, 0.0}, //26 Z order value
    //{FTFloat15_7, 0.0}, //27 number of samples
    if (double v = type1858[34].value.toDouble(); !qIsNull(v))
        d.put("function.windowParameter", v); //34-35 Exponential window damping factor
    //{FTString10a,"NONE  NONE"}, //42 response direction, reference direction
}

void FunctionHeader::sanitize()
{
    for (int i=0; i<48; ++i) {
        if (type1858[i].type >= FTString80 && type1858[i].type <= FTTimeDate80) {
            if (type1858[i].value.toString() == "NONE") type1858[i].value.clear();
        }
    }
}

FunctionHeader FunctionHeader::fromDescription(const DataDescription &d)
{
    FunctionHeader h;
    h.type1858[5].value = d.get("function.octaveFormat");
    //{FTInteger12, 0}, //6 measurement run number
    h.type1858[11].value = weightingType(d.get("function.weighting").toString());
    h.type1858[12].value = uffWindowTypeFromDescription(d.get("function.window").toString());
    h.type1858[13].value = scalingTypeFromDescription(d.get("function.amplitudeScaling").toString());
    h.type1858[14].value = normalizationTypeFromDescription(d.get("function.normalization").toString());
    //{FTInteger6, 0}, //15  Abscissa Data Type Qualifier,
                      //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    //{FTInteger6, 0}, //16 Ordinate Numerator Data Type Qualifier, see 15
    //{FTInteger6, 0}, //17 Ordinate Denominator Data Type Qualifier, see 15
    //{FTInteger6, 0}, //18 Z-axis Data Type Qualifier, see 15
    //{FTInteger6, 0}, //19 sampling type, 0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
    //{FTFloat15_7, 0.0}, //24 Z RPM value
    //{FTFloat15_7, 0.0}, //25 Z time value
    //{FTFloat15_7, 0.0}, //26 Z order value
    h.type1858[27].value = d.get("samples");
    h.type1858[34].value = d.get("function.windowParameter");
    QString resp = d.get("function.responseDirection").toString();
    if (resp.isEmpty()) resp = "NONE";
    QString ref = d.get("function.referenceDirection").toString();
    if (ref.isEmpty()) ref = "NONE";
    h.type1858[42].value = QString("%1  %2").arg(resp,ref);
    return h;
}

FunctionDescription::FunctionDescription()
{
    setType58(type58);
    valid = true;
}

void FunctionDescription::read(QTextStream &stream)
{
    qint64 offs = stream.pos();
    for (int i=0; i<4; ++i) {
        fields[type58[i].type]->read(type58[i].value, stream);
    }
    if (type58[2].value.toInt()==58) {
        for (int i=4; i<60; ++i) {
            fields[type58[i].type]->read(type58[i].value, stream);
        }
    }
    else {
        valid = false;
        type58[2].value = 58;
        stream.seek(offs); // перемещаемся в начало, как будто и не читали
    }
}

void FunctionDescription::read(char *data, qint64 &offset)
{
    qint64 offs = offset;
    for (int i=0; i<4; ++i) {
        offset += fields[type58[i].type]->read(type58[i].value, data, offset);
    }
    if (type58[2].value.toInt()==58) {
        for (int i=4; i<60; ++i) {
            offset += fields[type58[i].type]->read(type58[i].value, data, offset);
        }
    }
    else {
        valid = false;
        type58[2].value = 58;
        offset = offs; // перемещаемся в начало, как будто и не читали
    }
}

void FunctionDescription::write(QTextStream &stream)
{
    for (int i=0; i<60; ++i) {
        fields[type58[i].type]->print(type58[i].value, stream);
    }
}

void FunctionDescription::toDataDescription(DataDescription &d)
{
    d.put("name", type58[4].value);
    d.put("description", type58[6].value);
    d.put("dateTime", type58[8].value); //8-9 Time date of function creation
    //{FTString80, "Record 1" }, {FTEmpty,""}, //10-11 ID line 4,
    d.put("correction", type58[12].value); //12-13 ID line 5
    d.put("function.type", type58[14].value);
    d.put("function.name", Descriptor::functionTypeDescription(type58[14].value.toInt()));
    //{FTInteger10, 1}, //15 Function Identification Number
    d.put("sequence", type58[16].value);
    //{FTInteger10, 0},//17 Load Case Identification Number
    if (QString s = type58[18].value.toString(); !s.isEmpty()) {
        d.put("function.responseName", s);
    }
    //{FTInteger10, 0}, //19 Response Node
    if (QString s = type58[20].value.toString(); !s.isEmpty()) {
        d.put("function.responseDirection", s);
    }
    if (QString s = type58[21].value.toString(); !s.isEmpty()) {
        d.put("function.referenceName", s);
    }
    //{FTInteger10, 0}, //22 Reference Node
    if (QString s = type58[21].value.toString(); !s.isEmpty()) {
        d.put("function.referenceDirection", s);
    }
    {
        int v = type58[25].value.toInt();
        d.put("function.precision", (v==2 || v==5)?"float":"double");
        if (v>=5)
            d.put("function.format", "complex");
    }
    d.put("samples", type58[26].value);
    d.put("uneven", (type58[27].value.toInt() == 0));
    d.put("xmin", type58[28].value.toDouble());
    d.put("xstep", type58[29].value.toDouble());
    //d.put("zvalue", type58[30].value.toDouble());

    //Abscissa Data Characteristics 32-38
    {
        QString s = type58[37].value.toString();
        if (s.isEmpty()) s = unitNameFromUffType(type58[32].value.toInt());
        d.put("xname", s);
        //{FTInteger5, 0}, //33 Length units exponent
        //{FTInteger5,0}, //34 Force units exponent
        //{FTInteger5, 0}, //35 Temperature units exponent
        //{FTString20, "NONE"}, //36 Axis label ("NONE" if not used)
    }
    //Ordinate (or ordinate numerator) Data Characteristics 39-45
    {
        //может оказаться, что тип единицы не соответствует названию. Меняем тип единицы
        int type = type58[39].value.toInt();
        d.put("ylabel", type58[43].value);
        QString s = type58[44].value.toString();
        if (s.isEmpty()) s = unitNameFromUffType(type);
        if (int t = unitTypeFromName(s); t != type) type = t;
        d.put("yname", s);
        d.put("yUnitType", type);
        //{FTInteger5,0}, //40 Length units exponent
        //{FTInteger5,0}, //41 Force units exponent
        //{FTInteger5, 0}, //42 Temperature units exponent
        //{FTString20, "NONE"}, //43  Axis label ("NONE" if not used)
    }

    {//Знаменатель единицы измерения - м/с^2/Гц = Гц
        QString s = type58[51].value.toString();
        if (s.isEmpty()) s = unitNameFromUffType(type58[46].value.toInt());
        if (!s.isEmpty())
            d.put("yname", d.get("yname").toString()+"/"+s);
    }
    //{FTInteger5,0}, //47
    //{FTInteger5,0}, //48
    //{FTInteger5,0}, //49
    //{FTString20, "NONE"}, //50
    {
        QString s = type58[58].value.toString();
        if (s.isEmpty()) s = unitNameFromUffType(type58[53].value.toInt());
        d.put("zname", s);
    }
    //{FTInteger5, 0}, //54
    //{FTInteger5, 0}, //55
    //{FTInteger5, 0}, //56
    //{FTString20, "NONE"}, //57
}

void FunctionDescription::sanitize()
{
    for (int i=0; i<60; ++i) {
        if (type58[i].type >= FTString80 && type58[i].type <= FTTimeDate80) {
            if (type58[i].value.toString() == "NONE") type58[i].value.clear();
        }
    }
}

FunctionDescription FunctionDescription::fromDescription(const DataDescription &d)
{
    FunctionDescription h;
    h.type58[4].value = d.get("name");
    h.type58[6].value = d.get("description");
    h.type58[8].value = d.get("dateTime");
    //{FTString80, "Record 1" }, {FTEmpty,""}, //10-11 ID line 4,
    h.type58[12].value = d.get("correction");
    h.type58[14].value = d.get("function.type");
    //{FTInteger10, 1}, //15 Function Identification Number
    //{FTInteger5, 1}, //16 Version Number, or sequence number
    //{FTInteger10, 0},//17 Load Case Identification Number
    h.type58[18].value = d.get("function.responseName");
    h.type58[19].value = d.get("function.responseNode");
    h.type58[20].value = d.get("function.responseDirection");
    h.type58[21].value = d.get("function.referenceName");
    h.type58[22].value = d.get("function.referenceNode");
    h.type58[23].value = d.get("function.referenceDirection");
    QString format = d.get("function.format").toString();
    QString precision = d.get("function.precision").toString();
    if (format=="complex") {
        //write all other types (int) as float
        h.type58[25].value = (precision == "double" ? 6 : 5);
    }
    else {
        //write all other types (int) as float
        h.type58[25].value = (precision == "double" ? 4 : 2);
    }
    h.type58[26].value = d.get("samples");
    h.type58[27].value = (d.get("uneven").toBool() ? 0 : 1);
    h.type58[28].value = d.get("xmin");
    h.type58[29].value = d.get("xstep");
    h.type58[30].value = d.get("zvalue");
    h.type58[32].value = unitTypeFromName(d.get("xname").toString());
    //{FTInteger5, 0}, //33 Length units exponent
    //{FTInteger5,0}, //34 Force units exponent
    //{FTInteger5, 0}, //35 Temperature units exponent
    //{FTString20, "NONE"}, //36 Axis label ("NONE" if not used)
    h.type58[36].value = unitDescriptionFromUffType(h.type58[32].value.toInt());
    h.type58[37].value = d.get("xname");
    QString yname = d.get("yname").toString();
    h.type58[39].value = unitTypeFromName(yname);
    //{FTInteger5,0}, //40 Length units exponent
    //{FTInteger5,0}, //41 Force units exponent
    //{FTInteger5, 0}, //42 Temperature units exponent
    h.type58[43].value = unitDescriptionFromUffType(h.type58[39].value.toInt());
    h.type58[44].value = yname;
    //Игнорируем знаменатель единицы измерения - слишком сложно реализовывать
    //h.type58[46].value = unitTypeFromName(ynameDenom);
    //{FTInteger5,0}, //47
    //{FTInteger5,0}, //48
    //{FTInteger5,0}, //49
    //FTString20, "NONE"}, //50
    //h.type58[51].value = ynameDenom;
    h.type58[53].value = unitTypeFromName(d.get("zname").toString());
    //{FTInteger5, 0}, //54
    //{FTInteger5, 0}, //55
    //{FTInteger5, 0}, //56
    h.type58[57].value = unitDescriptionFromUffType(h.type58[53].value.toInt());
    h.type58[58].value = d.get("zname");

    //sanitizing
    for (int i=0; i<60; ++i) {
        if (h.type58[i].type >= FTString80 && h.type58[i].type <= FTTimeDate80) {
            if (h.type58[i].value.toString().isEmpty())
                h.type58[i].value = "NONE";
        }
    }
    return h;
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
    //setType58(type58);
    parent->channels << this;
}



Function::Function(Channel &other, UffFileDescriptor *parent) : Channel(other), parent(parent)
{DD;
    parent->channels << this;

    dataDescription().put("dateTime", QDateTime::currentDateTime());
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

    FunctionHeader header;
    header.read(stream);
    header.sanitize();
    FunctionDescription description;
    description.read(stream);
    description.sanitize();
    header.toDataDescription(dataDescription());
    description.toDataDescription(dataDescription());

    //первое положение данных
    dataPositions << stream.pos();
    zValues << description.type58[30].value.toDouble();

    if (pos == -1) {
        QString s;
        do {
            s = stream.readLine().trimmed();
            if (stream.atEnd()) s= "-1";
        }
        while (s != "-1");
    }
    dataEnds << stream.pos() - 6-2-2;
}

void Function::read(char *data, qint64 &offset, int size)
{DD;
    dataPositions.clear();  dataEnds.clear();
    zValues.clear();

    FunctionHeader header;
    header.read(data, offset);
    header.sanitize();
    FunctionDescription description;
    description.read(data, offset);
    description.sanitize();
    header.toDataDescription(dataDescription());
    description.toDataDescription(dataDescription());

    //первое положение данных
    dataPositions << qint64(offset);
    zValues << description.type58[30].value.toDouble();

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
}

void Function::read(QDataStream &stream)
{
    stream >> dataDescription();
    stream >> dataPositions;
    stream >> dataEnds;
    stream >> zValues;

    readRest();
}

void Function::readRest()
{
    //zValues may be long - multiblock file

    //может так получиться, что тип единицы по оси y будет неправильным.
    //может так получиться, что название единицы по оси y будет неправильным.
    //определяем пороговые значения отдельно и сравниваем
    double thr1 = logrefFromUffUnit(dataDescription().get("yUnitType").toInt());
    double thr2 = logrefFromUffUnit(unitTypeFromName(yName()));
    double thr = thr1;
    if (!qFuzzyCompare(thr1, thr2)) {
        //если какой-то порог не равен 1.0, берем его, отдаем преимущество порогу по имени единицы
        if (!qFuzzyCompare(thr2, 1.0)) thr = thr2;
        else thr = thr1;
    }
    //у FRF порог всегда равен 1
    int ftype = dataDescription().get("function.type").toInt();
    if (ftype==Descriptor::FrequencyResponseFunction) thr = 1.0;
    _data->setThreshold(thr);
    dataDescription().put("function.logref", thr);

    auto yValueFormat = DataHolder::YValuesReals;

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
    if (dataDescription().get("function.format").toString()=="complex")
        yValueFormat = DataHolder::YValuesComplex;

    QString ylabel = dataDescription().get("ylabel").toString();

    if (yName().toLower() == "dB" || yName().toLower() == "дБ" || ylabel.toLower()=="уровень")
        yValueFormat = DataHolder::YValuesAmplitudesInDB;

    if (ylabel.toLower()=="phase")
        yValueFormat = DataHolder::YValuesPhases;

    _data->setYValuesFormat(yValueFormat);
    dataDescription().put("function.format", DataHolder::formatToString(yValueFormat));

    auto units = DataHolder::UnitsLinear;
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

    if (! dataDescription().get("uneven").toBool()) {// abscissa, even spacing
        _data->setXValues(dataDescription().get("xmin").toDouble(),
                dataDescription().get("xstep").toDouble(),
                dataDescription().get("samples").toInt());
    }
    else {// abscissa, uneven spacing
        _data->setXValues(QVector<double>(dataDescription().get("samples").toInt()));
        populate();
    }

    Q_ASSERT(!zValues.isEmpty());
    double zStep = 0.0;
    int zCount = zValues.size();
    double zBegin = zValues.at(0);


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


void Function::write(QTextStream &stream, int &id)
{DD;
    const int samples = data()->samplesCount();
    const int blocks = data()->blocksCount();
    dataPositions.clear();  dataEnds.clear();

    FunctionHeader head = FunctionHeader::fromDescription(dataDescription());
    FunctionDescription descr = FunctionDescription::fromDescription(dataDescription());

    head.type1858[27].value = samples;
    descr.type58[26].value = samples;
    descr.type58[29].value = data()->xStep();

    for (int block = 0; block < blocks; ++block) {
        //writing header
        FunctionHeader h = head;
        h.type1858[4].value = block+1;
        h.write(stream);

        auto t58 = descr.type58;
        t58[10].value = QString("Record %1").arg(block+1);
        t58[15].value = id+block;
        t58[16].value = block+1;
        t58[30].value = data()->zValue(block);


        for (int i=0; i<60; ++i) {
            fields[t58[i].type]->print(t58[i].value, stream);
        }
        dataPositions << stream.pos();

        const int format = data()->xValuesFormat();
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

FileDescriptor *Function::descriptor() const
{
     return parent;
}

Descriptor::DataType Function::type() const
{
    return static_cast<Descriptor::DataType>(dataDescription().get("function.type").toInt());
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
        qDebug()<<"Ошибка чтения данных с помощью mmap";
        return false;
    }
    char *data = reinterpret_cast<char*>(mmap);

    Q_ASSERT_X (dataPositions.first() != -1, "Function::populate", "Data positions have been invalidated");
    Q_ASSERT_X (_data->blocksCount() == dataPositions.size(), "Function::populate",
                "Data positions не соответствуют количеству блоков");

    bool real = _data->yValuesFormat() != DataHolder::YValuesComplex;
    bool uneven = _data->xValuesFormat() == DataHolder::XValuesNonUniform;

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

        if (real) {//real values
            if (uneven) {// uneven abscissa
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
            if (uneven) {// uneven abscissa
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

        if (uneven) {// uneven abscissa
            _data->setXValues(xvalues);
        }
        if (real) {//real values
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

    bool complex = _data->yValuesFormat() == DataHolder::YValuesComplex;
    bool uneven = _data->xValuesFormat() == DataHolder::XValuesNonUniform;

    for (int block = 0; block < _data->blocksCount(); ++block) {
//        qDebug()<<"reading block"<<block+1;
        if (stream.seek(dataPositions.at(block))) {
//            qDebug()<<"reding pos"<<dataPositions.at(block);

            QVector<double> values, xvalues;
            QVector<cx_double> valuesComplex;

            if (complex) { //complex values
                valuesComplex = QVector<cx_double>(sc, cx_double());
            }
            else
                values = QVector<double>(sc, 0.0);

            if (uneven) {
                // uneven abscissa, read data pairs
                xvalues = QVector<double>(sc, 0.0);
            }
            if (!complex) {//real values
                int j=0;
                for (int i=0; i<sc; ++i) {
                    double value;
                    stream >> value;

                    if (uneven) {// uneven abscissa
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
                    if (uneven) {// uneven abscissa
                        stream >> first;
                        xvalues[j] = first;
                    }
                    stream >> first >> second; //qDebug()<<first<<second;
                    valuesComplex[j] = {first, second};
                    j++;
                }
            }
            if (uneven) {// uneven abscissa
                _data->setXValues(xvalues);
            }
            if (!complex) {//real values
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

int uffWindowTypeFromDescription(const QString &description)
{
    //window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
                     //4=exponential, 5=impact, 6=impact and exponential

    if (description == "no") return 0;
    if (description == "Hanning") return 1;
    if (description == "Hanning broad") return 2;
    if (description == "flattop") return 3;
    if (description == "exponential") return 4;
    if (description == "impact") return 5;
    if (description == "force") return 6;

    //other types of windows unknown to uff 1858
    return 0;
}

QString windowDescriptionFromUffType(int type)
{
    //window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
                     //4=exponential, 5=impact, 6=impact and exponential

    switch (type) {
        case 0: return "no";
        case 1: return "Hanning";
        case 2: return "Hanning broad";
        case 3: return "flattop";
        case 4: return "exponential";
        case 5: return "impact";
        case 6: return "force";
        default: break;
    }
    return "unknown";
}

int scalingTypeFromDescription(const QString &description)
{
    if (description == "unknown") return 0;
    if (description == "half-peak") return 1;
    if (description == "peak") return 2;
    if (description == "RMS") return 3;
    return 0;
}

QString scalingDescriptionFromUffType(int type)
{
    switch (type) {
        case 1: return "half-peak";
        case 2: return "peak";
        case 3: return "RMS";
    }
    return "unknown";
}

int normalizationTypeFromDescription(const QString &description)
{
    //0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
    //3=Units squared seconds per Hz (ESD)
    if (description == "unknown") return 0;
    if (description == "squared") return 1;
    if (description == "squared/Hz") return 2;
    if (description == "squared sec/Hz") return 3;
    return 0;
}

QString normalizationDescriptionFromUffType(int type)
{
    switch (type) {
        case 1: return "squared";
        case 2: return "squared/Hz";
        case 3: return "squared sec/Hz";
    }
    return "unknown";
}

int unitTypeFromName(QString s)
{
    s = s.toLower();
    //2 - stress - not detectable (= pressure)
    //3 - strain - dimensionless
    //5 - temperature
    if (s == "k" || s == "°c" || s == "°f") return 5;
    //6 - heat flux
    if (s == "w/m^2" || s == "w/m2" || s == "вт/м^2" || s == "вт/м2") return 6;
    //8 - displacement
    if (s == "m" || s == "м" || s == "mm" || s == "мм" || s == "cm"
        || s == "см") return 8;
    //9 - reaction force - non detectable (= force)
    //11 - velocity
    if (s == "m/s" || s == "м/с") return 11;
    //12 - acceleration
    if (s == "m/s2" || s == "m/s^2" || s == "м/с2" || s == "м/с^2" || s == "g") return 12;
    //13 - excitation force
    if (s == "n" || s == "н") return 13;
    //15 - pressure
    if (s == "pa" || s == "psi" || s == "па") return 15;
    //16 - mass
    if (s == "kg" || s == "кг") return 16;
    //17 - time
    if (s == "s" || s == "с") return 17;
    //18 - frequency
    if (s == "hz" || s == "гц" || s == "1/s" || s == "1/с") return 18;
    //19 - rpm
    if (s == "rpm" || s == "rad/s" || s == "deg/s" || s == "°/s" || s == "рад/с" || s == "°/с"
        || s == "об/мин" || s == "об/с") return 19;
    //20 - order
    if (s == "1/deg" || s == "1/rad" || s == "1/рад") return 20;

    return 0; //0 - unknown
}

QString unitNameFromUffType(int type)
{
    switch (type) {
        case 2:
        case 15: return "Па";
        case 5:  return "°C";
        case 6:  return "Вт/м^2";
        case 8:  return "м";
        case 9:
        case 13: return "Н";
        case 11: return "м/с";
        case 12: return "м/с^2";
        case 16: return "кг";
        case 17: return "с";
        case 18: return "Гц";
        case 19: return "об/мин";
        case 20: return "1/рад";
    }
    return "";
}

QString unitDescriptionFromUffType(int type)
{
    switch (type) {
        case 1: return "General";
        case 2: return "Stress";
        case 3: return "Strain";
        case 5: return "Temperature";
        case 6: return "Heat flux";
        case 8: return "Displacement";
        case 9: return "Reaction force";
        case 11: return "Velocity";
        case 12: return "Acceleration";
        case 13: return "Excitation force";
        case 15: return "Pressure";
        case 16: return "Mass";
        case 17: return "Time";
        case 18: return "Frequency";
        case 19: return "RPM";
        case 20: return "Order";
    }
    return "Unknown";
}

double logrefFromUffUnit(int type)
{
    switch (type) {
        case 2: return 2.0e-5; //"Stress";
        case 3: return 1.0; //Strain
        case 5: return 1.0; //Temperature
        case 6: return 1.0e-12; //"Heat flux";
        case 8: return 8.0e-14; //"Displacement"; <- значение DeepSea
        case 9: return 1.0; //"Reaction force";
        case 11: return 5.0e-8; //"Velocity";
        case 12: return 3.14e-4; //"Acceleration"; <- значение базы датчиков
        case 13: return 1.0; //"Excitation force";
        case 15: return 2.0e-5; //Pressure
        case 16: return 1.0; //"Mass";
        case 17: return 1.0; //"Time";
        case 18: return 1.0;//"Frequency";
        case 19: return 5.0e-8;//"RPM";
        case 20: return 1.0; //"Order";
    }
    return 1.0;
}


