#include "dfdio.h"
#include "filedescriptor.h"
#include "dfdfiledescriptor.h"
#include "logging.h"

DfdIO::DfdIO(const QVector<Channel *> &source, const QString &fileName, QObject *parent)
    : FileIO(fileName, parent)
{
    if (source.isEmpty()) return;

    Channel *firstChannel = source.constFirst();
    auto other = firstChannel->descriptor();

    DataDescription description(other->dataDescription());

    QDateTime dt = QDateTime::currentDateTime();
    description.put("fileCreationTime", dt);
    description.put("guid", FileDescriptor::createGUID());
    description.put("createdBy", "DeepSea Base");

    QString rawFileName = fileName.left(fileName.length()-4)+".raw";

    const DfdFileDescriptor *dfd = dynamic_cast<const DfdFileDescriptor*>(other);
    DfdDataType dataType;
    if (dfd) {
        dataType = dfd->DataType;
    }
    else {
        ///TODO: переписать определение типа данных файла DFD
        dataType = dfdDataTypeFromDataType(*source.first());
    }
    // time data tweak, so deepseabase doesn't take the file as raw time data
    //так как мы вызываем эту функцию только из новых файлов,
    //все сведения из файлов rawChannel нам не нужны
    if (dataType == SourceData) dataType = CuttedData;

    int samplesCount = m_parameters.value("samplesCount", firstChannel->data()->samplesCount()).toInt();

    //Поскольку other может содержать каналы с разным типом, размером и шагом,
    //данные берем из первого канала, который будем сохранять
    //предполагается, что все каналы из indexes имеют одинаковые параметры

    //копируем каналы и записываем данные
    QFile raw(rawFileName);
    if (!raw.open(QFile::WriteOnly)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<rawFileName;
        return;
    }
    QDataStream w(&raw);
    w.setByteOrder(QDataStream::LittleEndian);
    bool xChannel = firstChannel->data()->xValuesFormat()==DataHolder::XValuesNonUniform;

    //Сохраняем файл dfd
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<fileName;
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    QTextStream dfdStream(&file);
    dfdStream.setCodec(codec);

    //Записываем шапку файла и канал с осью X, если есть
    dfdStream << "[DataFileDescriptor]" << endl;
    dfdStream << "DFDGUID="<<description.get("guid").toString() << endl;
    dfdStream << "DataType="<<dataType << endl;
    auto dateTime = description.dateTime("dateTime");
    dfdStream << "Date="<<dateTime.toString("dd.MM.yyyy")
        << endl;
    dfdStream << "Time="<<dateTime.toString("hh:mm:ss")
        << endl;
    dfdStream << "NumChans="<< (xChannel ? source.size()+1 : source.size()) << endl;
    int numInd = firstChannel->data()->samplesCount();
    if (samplesCount > 0) numInd = samplesCount;
    double xBegin = firstChannel->data()->xMin();
    double xStep = firstChannel->data()->xStep();

    dfdStream << "NumInd="<< numInd << endl;
    dfdStream << "BlockSize="<< 0 << endl;
    auto xname = description.get("xname").toString();
    if (xname.isEmpty()) xname="s";
    dfdStream << "XName="<< xname << endl;
    dfdStream << "XBegin=" << doubletohex(xBegin).toUpper() << endl;
    dfdStream << "XStep=" << doubletohex(xStep).toUpper() << endl;
    dfdStream << "DescriptionFormat=" << description.get("description.format").toString() << endl;
    dfdStream << "CreatedBy="<<description.get("createdBy").toString() << endl;

    /** [DataDescription] */
    Description descr(nullptr);
    descr.write(dfdStream, description);

    /** [Source] */
    if (channelsFromSameFile(source)) {
        description.put("source.file", other->fileName());
        description.put("source.guid", other->dataDescription().get("guid"));
        description.put("source.dateTime", other->dataDescription().get("dateTime"));

        if (other->channelsCount() > source.size()) {
            //только если копируем не все каналы
            description.put("source.channels", stringify(channelIndexes(source)));
        }
        Source source1(nullptr);
        source1.write(dfdStream, description);
    }

    /** [Process] */
    if (!source.isEmpty()) {
        Process process(&firstChannel->dataDescription());
        process.write(dfdStream);
    }

    if (xChannel) {
        //добавляем нулевой канал с осью Х
        w.setFloatingPointPrecision(QDataStream::SinglePrecision);
        QVector<double> vals = firstChannel->data()->xValues();
        if (samplesCount>0) vals.resize(samplesCount);
        DfdChannel ch(0,0);
        ch.IndType = 0xC0000004;
        ch.ChanBlockSize = numInd;
        ch.setName("ось X");
        ch.setYName("Гц");
        ch.data()->setThreshold(1.0);
        ch.data()->setXValues(firstChannel->data()->xValues());
        ch.data()->setYValues(firstChannel->data()->xValues(), DataHolder::YValuesReals);

        for (double val: vals)
            ch.setValue(val, w);

        ch.write(dfdStream, 0);
        channelsCount++;
    }
}


void DfdIO::addChannel(DataDescription *description, DataHolder *data)
{
    int samplesCount = data->samplesCount();

    //копируем каналы и записываем данные
    QString rawFileName = fileName.left(fileName.length()-4)+".raw";
    QFile raw(rawFileName);
    if (!raw.open(QFile::Append)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<rawFileName;
        return;
    }
    QDataStream w(&raw);
    w.setByteOrder(QDataStream::LittleEndian);

    QString precision = description->get("function.precision").toString();
    uint IndType;
    if (precision == "uint8") IndType = 0x1;
    else if (precision == "int8") IndType = 0x80000001;
    else if (precision == "uint16") IndType = 0x2;
    else if (precision == "int16") IndType = 0x80000002;
    else if (precision == "uint32") IndType = 0x4;
    else if (precision == "int32") IndType = 0x80000004;
    else if (precision == "uint64") IndType = 0x8;
    else if (precision == "int64") IndType = 0x80000008;
    else if (precision == "float") IndType = 0xc0000004;
    else if (precision == "double") IndType = 0xc0000004; //DeepSea не умеет читать такие файлы
    else IndType = 0xc0000004; //по умолчанию
    w.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //dfd не понимает многоблочные файлы
    QVector<double> yValues = data->rawYValues(0);
    if (yValues.isEmpty() && !data->yValuesComplex(0).isEmpty())
        yValues = data->linears(0);
    if (samplesCount > 0) yValues.resize(samplesCount);

    for (auto val: yValues) {
        switch (IndType) {
            case 0x00000001: {
                quint8 v = (quint8)val;
                w << v;
                break;}
            case 0x80000001: {
                qint8 v = (qint8)val;
                w << v;
                break;}
            case 0x00000002: {
                quint16 v = (quint16)val;
                w << v;
                break;}
            case 0x80000002: {
                qint16 v = (qint16)val;
                w << v;
                break;}
            case 0x00000004: {
                quint32 v = (quint32)val;
                w << v;
                break;}
            case 0x80000004: {
                qint32 v = (qint32)val;
                w << v;
                break;}
            case 0x80000008: {
                qint64 v = (qint64)val;
                w << v;
                break;}
            case 0xC0000004: {
                float v = (float)val;
                w << v;
                break;}
            case 0xC0000008:
            case 0xC000000A:
                w << val;
                break;
            default: break;
        }
    }

    QFile file(fileName);
    if (!file.open(QFile::Append | QFile::Text)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<fileName;
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName("Windows-1251");
    QTextStream dfd(&file);
    dfd.setCodec(codec);

    dfd << QString("[Channel%1]").arg(++channelsCount) << endl;
    dfd << "ChanAddress=" << description->get("sensorID").toString() << endl;
    dfd << "ChanName=" << description->get("name").toString() << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << yValues.size() << endl;
    dfd << "YName=" << description->get("yname").toString() << endl;
    dfd << "YNameOld=" << description->get("ynameold").toString() << endl;
    dfd << "InputType="<< description->get("inputType").toString() << endl;
    dfd << "ChanDscr="<< description->get("description").toString() << endl;
    dfd << "Correction="<< description->get("correction").toString() << endl;
    dfd << "threshold=" << QString::number(data->threshold()) << endl;
    dfd << "units=" << DataHolder::unitsToString(data->yValuesUnits()) << endl;
    emit tick();
}

void DfdIO::finalize()
{
}
