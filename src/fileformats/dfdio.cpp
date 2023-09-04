#include "dfdio.h"
#include "dfdfiledescriptor.h"
#include "logging.h"

DfdIO::DfdIO(const QVector<Channel *> &source, const QString &fileName, QObject *parent, const QMap<QString, QVariant> &parameters)
    : FileIO(fileName, parent, parameters)
{
    if (source.isEmpty()) return;

    if (QFile::exists(fileName)) {
        LOG(ERROR)<<QString("Такой файл уже существует: ")<<m_rawFileName;
        return;
    }
    int dataFormat = m_parameters.value("dataFormat", 0).toInt();

    Channel *firstChannel = source.constFirst();
    auto other = firstChannel->descriptor();

    m_fileDescription = other->dataDescription();

    QDateTime dt = QDateTime::currentDateTime();
    m_fileDescription.put("fileCreationTime", dt);
    m_fileDescription.put("guid", FileDescriptor::createGUID());
    m_fileDescription.put("createdBy", "DeepSea Base");

    m_rawFileName = fileName.left(fileName.length()-4)+".raw";

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
    //time data tweak
    if (dataFormat == 1) dataType = SourceData;

    int samplesCount = m_parameters.value("samplesCount", firstChannel->data()->samplesCount()).toInt();

    //Поскольку other может содержать каналы с разным типом, размером и шагом,
    //данные берем из первого канала, который будем сохранять
    //предполагается, что все каналы из indexes имеют одинаковые параметры

    //копируем каналы и записываем данные
    QFile raw(m_rawFileName);
    if (!raw.open(QFile::WriteOnly)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<m_rawFileName;
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
    dfdStream << "DFDGUID="<<m_fileDescription.get("guid").toString() << endl;
    dfdStream << "DataType="<<dataType << endl;
    auto dateTime = m_fileDescription.dateTime("dateTime");
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
    auto xname = m_fileDescription.get("xname").toString();
    if (xname.isEmpty()) xname="s";
    dfdStream << "XName="<< xname << endl;
    dfdStream << "XBegin=" << doubletohex(xBegin).toUpper() << endl;
    dfdStream << "XStep=" << doubletohex(xStep).toUpper() << endl;
    dfdStream << "DescriptionFormat=" << m_fileDescription.get("description.format").toString() << endl;
    dfdStream << "CreatedBy="<<m_fileDescription.get("createdBy").toString() << endl;

    /** [DataDescription] */
    Description descr(nullptr);
    descr.write(dfdStream, m_fileDescription);

    /** [Source] */
    if (channelsFromSameFile(source)) {
        m_fileDescription.put("source.file", other->fileName());
        m_fileDescription.put("source.guid", other->dataDescription().get("guid"));
        m_fileDescription.put("source.dateTime", other->dataDescription().get("dateTime"));

        if (other->channelsCount() > source.size()) {
            //только если копируем не все каналы
            m_fileDescription.put("source.channels", stringify(channelIndexes(source)));
        }
        Source source1(nullptr);
        source1.write(dfdStream, m_fileDescription);
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
        m_channelsCount++;
    }
}

DfdIO::DfdIO(const DataDescription &description, const QString &fileName, QObject *parent,
             const QMap<QString, QVariant> & parameters)
    : FileIO(fileName, parent, parameters)
{
    m_fileExists = QFile::exists(fileName);

    //Если файл существует, то считаем, что можем дописывать в него каналы
    //Все проверки на совпадение типов должны проводиться где-то еще

    if (m_fileExists) {
        DfdFileDescriptor d(fileName);
        d.read();
        m_fileDescription = d.dataDescription();
    }
    else {
        m_fileDescription = description;
        QDateTime dt = QDateTime::currentDateTime();
        m_fileDescription.put("fileCreationTime", dt);
        m_fileDescription.put("guid", FileDescriptor::createGUID());
        m_fileDescription.put("createdBy", "DeepSea Base");

        //Тип файла берется по первому каналу

        //Date и Time берутся по первому каналу
    }
}


void DfdIO::addChannel(DataDescription *description, DataHolder *data)
{
    int samplesCount = data->samplesCount();
    int dataFormat = m_parameters.value("dataFormat", 0).toInt();

    //копируем каналы и записываем данные
    QFile raw(m_rawFileName);
    if (!raw.open(QFile::Append)) {
        LOG(ERROR)<<QString("Не могу открыть файл для записи: ")<<m_rawFileName;
        return;
    }
    QDataStream w(&raw);
    w.setByteOrder(QDataStream::LittleEndian);

    QString precision = description->get("function.precision").toString();
    const uint IndType = dataFormat == 1 ? 0x00000002 : 0xc0000004;
    const double absvolt = description->get("description.absvolt", 1.0).toDouble();
    w.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //dfd не понимает многоблочные файлы
    QVector<double> yValues = data->rawYValues(0);
    if (yValues.isEmpty() && !data->yValuesComplex(0).isEmpty())
        yValues = data->linears(0);
    if (samplesCount > 0) yValues.resize(samplesCount);

    for (auto val: yValues) {
        switch (IndType) {
            case 0x00000002: {
                quint16 v = (quint16)(val/absvolt)+32768;
                w << v;
                break;}
            case 0xC0000004: {
                float v = (float)val;
                w << v;
                break;}
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

    dfd << QString("[Channel%1]").arg(++m_channelsCount) << endl;
    dfd << "ChanAddress=" << description->get("sensorID").toString() << endl;
    dfd << "ChanName=" << description->get("name").toString() << endl;
    dfd << "IndType=" << IndType << endl;
    dfd << "ChanBlockSize=" << yValues.size() << endl;
    dfd << "YName=" << description->get("yname").toString() << endl;
    dfd << "YNameOld=" << description->get("ynameold").toString() << endl;
    dfd << "InputType="<< description->get("inputType").toString() << endl;
    dfd << "ChanDscr="<< description->get("description").toString() << endl;
    if (dataFormat == 1) {
        dfd << "ADC0=" <<  doubletohex(-32768.0 * absvolt).toUpper() << endl;
        dfd << "ADCStep=" <<  doubletohex(absvolt).toUpper() << endl;
        dfd << "AmplShift=" << doubletohex(0).toUpper() << endl;
        dfd << "AmplLevel=" << doubletohex(1).toUpper() << endl;
        dfd << "Sens0Shift=" << doubletohex(0).toUpper() << endl;
        dfd << "SensSensitivity=" << doubletohex(1).toUpper() << endl;
    }
    dfd << "Correction="<< description->get("correction").toString() << endl;
    dfd << "threshold=" << QString::number(data->threshold()) << endl;
    dfd << "units=" << DataHolder::unitsToString(data->yValuesUnits()) << endl;
    emit tick();
}

void DfdIO::finalize()
{
}
