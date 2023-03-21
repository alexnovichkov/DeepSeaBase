#include "d94io.h"

#include "filedescriptor.h"
#include "data94file.h"
#include "logging.h"

D94IO::D94IO(const QVector<Channel *> &source, const QString &fileName, QObject *parent)
    : FileIO(fileName, parent)
{
    auto d = source.first()->descriptor();

    DataDescription dataDescription = d->dataDescription();

    QDateTime dt = QDateTime::currentDateTime();
    dataDescription.put("fileCreationTime", dt);
    dataDescription.put("guid", FileDescriptor::createGUID());
    dataDescription.put("createdBy", "DeepSea Base");

    if (channelsFromSameFile(source)) {
        dataDescription.put("source.file", d->fileName());
        dataDescription.put("source.guid", d->dataDescription().get("guid"));
        dataDescription.put("source.dateTime", d->dataDescription().get("dateTime"));

        if (d->channelsCount() > source.size()) {
            //только если копируем не все каналы
            dataDescription.put("source.channels", stringify(channelIndexes(source)));
        }
    }

    QFile f(fileName);
    if (!f.open(QFile::WriteOnly)) {
        LOG(ERROR)<<"Не удалось открыть файл для записи:"<<fileName;
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");

    QJsonDocument doc(dataDescription.toJson());
    QByteArray json = doc.toJson();
    quint32 descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    //записываем количество каналов
    r << quint32(source.size());
}


void D94IO::addChannel(DataDescription *description, DataHolder *data)
{
    QFile f(fileName);
    if (!f.open(QFile::Append)) {
        LOG(ERROR)<<"Не удалось открыть файл для записи:"<<fileName;
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);

    auto precision = description->get("function.precision").toString();
    //по умолчанию точность float
    //int8 / uint8 / int16 / uint16 / int32 / uint32 / int64 / uint64 / float / double
    if (precision.isEmpty()) precision = "float";
    quint32 sampleWidth;
    if (precision.endsWith("int8")) sampleWidth = 1;
    else if (precision.endsWith("int16")) sampleWidth = 2;
    else if (precision.endsWith("int64") || precision=="double") sampleWidth = 8;
    else sampleWidth = 4;
    if (precision == "float")
        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    else
        r.setFloatingPointPrecision(QDataStream::DoublePrecision);


    r.device()->write("d94chan ");
    DataDescription dataDescription = *description;

    bool isComplex = data->yValuesFormat() == DataHolder::YValuesComplex;

    int samplesCount = data->samplesCount();

    AxisBlock xAxisBlock;
    xAxisBlock.uniform = data->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    xAxisBlock.begin = data->xMin();
    if (xAxisBlock.uniform == 0) {// not uniform
        xAxisBlock.values = data->xValues();
        if (samplesCount > 0) xAxisBlock.values.resize(samplesCount);
    }
    xAxisBlock.count = samplesCount > 0 ? samplesCount : data->samplesCount();
    xAxisBlock.step = data->xStep();

    AxisBlock zAxisBlock;
    zAxisBlock.uniform = data->zValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    zAxisBlock.count = data->blocksCount();
    zAxisBlock.begin = data->zMin();
    zAxisBlock.step = data->zStep();
    if (zAxisBlock.uniform == 0) // not uniform
        zAxisBlock.values = data->zValues();
    if (zAxisBlock.values.isEmpty() && !zAxisBlock.uniform) {
        for (uint i=0; i<zAxisBlock.count; ++i) zAxisBlock.values << i;
    }

    if (xAxisBlock.uniform == 1 && !qFuzzyIsNull(xAxisBlock.step))
        dataDescription.put("samplerate", int(1.0 / xAxisBlock.step));
    if (samplesCount>0) dataDescription.put("samples", samplesCount);


    QJsonDocument doc(dataDescription.toJson());
    QByteArray json = doc.toJson();
    quint32 descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    xAxisBlock.write(r);
    zAxisBlock.write(r);

    const quint32 format = isComplex ? 2 : 1;
    r << format;
    r << sampleWidth;

    for (int block = 0; block < data->blocksCount(); ++block) {
        if (!isComplex) {
            QVector<double> yValues = data->rawYValues(block);
            if (samplesCount>0) yValues.resize(samplesCount);
            if (yValues.isEmpty()) {
                LOG(ERROR)<<QString("Отсутствуют данные для записи в канале")<<description->get("name").toString();
                continue;
            }

            for (double v: qAsConst(yValues)) {
                if (precision=="int8")        r << (qint8)v;
                else if (precision=="uint8")  r << (quint8)v;
                else if (precision=="int16")  r << (qint16)v;
                else if (precision=="uint16") r << (quint16)v;
                else if (precision=="uint32") r << (quint32)v;
                else if (precision=="int32")  r << (qint32)v;
                else if (precision=="int64")  r << (qint64)v;
                else if (precision=="uint64") r << (quint64)v;
                else if (precision=="float") r << (float)v;
                else r << v;
            }
        } // !c->isComplex
        else {
            auto yValues = data->yValuesComplex(block);
            if (yValues.isEmpty()) {
                LOG(ERROR)<<QString("Отсутствуют данные для записи в канале")<<description->get("name").toString();
                continue;
            }
            if (samplesCount>0) yValues.resize(samplesCount);
            for (cx_double v: qAsConst(yValues)) {
                if (precision=="int8")        {r << (qint8)v.real(); r << (qint8)v.imag();}
                else if (precision=="uint8")  {r << (quint8)v.real(); r << (quint8)v.imag();}
                else if (precision=="int16")  {r << (qint16)v.real(); r << (qint16)v.imag();}
                else if (precision=="uint16") {r << (quint16)v.real(); r << (quint16)v.imag();}
                else if (precision=="uint32") {r << (quint32)v.real(); r << (quint32)v.imag();}
                else if (precision=="int32")  {r << (qint32)v.real(); r << (qint32)v.imag();}
                else if (precision=="int64")  {r << (qint64)v.real(); r << (qint64)v.imag();}
                else if (precision=="uint64") {r << (quint64)v.real(); r << (quint64)v.imag();}
                else if (precision=="float")  {r << (float)v.real(); r << (float)v.imag();}
                else {
                    r << v.real();
                    r << v.imag();
                }
            }
        } // c->isComplex
    }

    emit tick();
}

void D94IO::finalize()
{
}
