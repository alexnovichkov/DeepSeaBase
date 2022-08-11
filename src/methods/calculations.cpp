#include "calculations.h"

#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"
#include "averaging.h"
#include "app.h"

Averaging *averageChannels(const QList<QPair<FileDescriptor *, int> > &toMean)
{
    QList<Channel*> list;
    for (auto i : toMean)
        list << i.first->channel(i.second);
    Channel *firstChannel = list.constFirst();

    //ищем наименьшее число отсчетов
    int numInd = firstChannel->data()->samplesCount();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->data()->samplesCount() < numInd)
            numInd = list.at(i)->data()->samplesCount();
    }

    // ищем формат данных для нового канала
    // если форматы разные, то формат будет линейный (амплитуды), не логарифмированный
    auto format = firstChannel->data()->yValuesFormat();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->data()->yValuesFormat() != format) {
            format = DataHolder::YValuesAmplitudes;
            break;
        }
    }

    int units = firstChannel->data()->yValuesUnits();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->data()->yValuesUnits() != units) {
            units = DataHolder::UnitsUnknown;
            break;
        }
    }

    auto *averaging = new Averaging(Averaging::Linear, list.size());

    for (Channel *ch: qAsConst(list)) {
        if (ch->data()->yValuesFormat() == DataHolder::YValuesComplex)
            averaging->average(ch->data()->yValuesComplex(0));
        else
            averaging->average(ch->data()->linears());
    }

    return averaging;
}

void calculateMean(FileDescriptor *file, const QList<Channel *> &channels)
{DDD;
    if (channels.isEmpty()) return;

    Channel *firstChannel = channels.constFirst();

    //ищем наибольшее число отсчетов
    QVector<double> xValues;
    int numInd = firstChannel->data()->samplesCount();
    if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesNonUniform)
        xValues = firstChannel->data()->xValues();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->samplesCount() > numInd) {
            numInd = channels.at(i)->data()->samplesCount();
            xValues = channels.at(i)->data()->xValues();
        }
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

    //определяем единицы
    auto units = firstChannel->data()->yValuesUnits();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->yValuesUnits() != units) {
            units = DataHolder::UnitsUnknown;
            break;
        }
    }

    Averaging averaging(Averaging::Linear, channels.size());

    //Выравниваем количество данных по самому длинному вектору
    for (Channel *ch: channels) {
        const bool populated = ch->populated();
        if (!populated) ch->populate();
        if (ch->data()->yValuesFormat() == DataHolder::YValuesComplex) {
            auto d = ch->data()->yValuesComplex(0);
            if (d.size() < averaging.size())
                d.append(QVector<cx_double>(averaging.size()-d.size()));
            averaging.average(d);
        }
        else {
            //усредняем первый блок
            auto d = ch->data()->linears(0);
            if (d.size() < averaging.size())
                d.append(QVector<double>(averaging.size()-d.size()));
            averaging.average(d);
        }
        if (!populated) ch->clear();
    }

    DataHolder *data = new DataHolder;

    data->setThreshold(firstChannel->data()->threshold());
    data->setYValuesUnits(units);

    if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        data->setXValues(firstChannel->data()->xMin(), firstChannel->data()->xStep(), numInd);
    }
    else {
        data->setXValues(xValues);
    }
    //усредняем только первый блок
    //TODO: добавить усреднение по всем блокам
    data->setZValues(firstChannel->data()->zMin(), firstChannel->data()->zStep(), 1);

    if (format == DataHolder::YValuesComplex) {
        auto d = averaging.getComplex();
        d.resize(data->samplesCount());
        data->setYValues(d);
    }
    else {//не комплексные
        QVector<double> d = averaging.get();
        d.resize(data->samplesCount());
        if (format == DataHolder::YValuesAmplitudesInDB) {
            data->setYValues(DataHolder::toLog(d, data->threshold(), units), format);
        }
        else
            data->setYValues(d, format);
    }

    DataDescription descr = channels.first()->dataDescription();
    descr.put("name", "Среднее");
    QStringList l;
    for (Channel *c: channels) {
        l << c->name();
    }
    descr.put("description", QString("Среднее каналов ")+l.join(","));
    descr.put("samples",  data->samplesCount());
    descr.put("blocks", 1);
    descr.put("function.name", "AVG");
    descr.put("function.logref", data->threshold());
    descr.put("function.format", DataHolder::formatToString(data->yValuesFormat()));
    descr.put("function.logscale", DataHolder::unitsToString(data->yValuesUnits()));

    file->addChannelWithData(data, descr);

    file->setChanged(true);
    file->setDataChanged(true);
    file->write();
}

void calculateMovingAvg(FileDescriptor *file, const QList<Channel *> &channels, int windowSize)
{
    if (channels.isEmpty()) return;

    for (Channel *c: channels) {
        DataHolder *data = new DataHolder;

        bool populated = c->populated();
        if (!populated) c->populate();

        data->setThreshold(c->data()->threshold());
        data->setYValuesUnits(c->data()->yValuesUnits());

        if (c->data()->xValuesFormat()==DataHolder::XValuesUniform)
            data->setXValues(c->data()->xMin(), c->data()->xStep(), c->data()->samplesCount());
        else
            data->setXValues(c->data()->xValues());

        auto format = c->data()->yValuesFormat();
        if (format == DataHolder::YValuesComplex) {
            //только первый блок
            data->setYValues(movingAverage(c->data()->yValuesComplex(0), windowSize));
        }
        else {
            QVector<double> values = movingAverage(c->data()->linears(0), windowSize);
            if (format == DataHolder::YValuesAmplitudesInDB)
                format = DataHolder::YValuesAmplitudes;
            //только первый блок
            data->setYValues(values, format, 0);
        }
        //TODO: добавить сглаживание по всем блокам
        data->setZValues(c->data()->zMin(), c->data()->zStep(), 1);

        DataDescription descr = c->dataDescription();
        descr.put("name", c->name()+" сглаж.");
        descr.put("description", "Скользящее среднее канала "+c->name());
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", 1);
        descr.put("function.name", "RAVG");

        file->addChannelWithData(data, descr);
        if (!populated) c->clear();
    }

    file->setChanged(true);
    file->setDataChanged(true);
    file->write();
}

void calculateThirdOctave(FileDescriptor *file, FileDescriptor *source)
{
    for (int i=0; i<source->channelsCount(); ++i) {
        Channel *ch = source->channel(i);

        const bool populated = ch->populated();
        if (!populated) ch->populate();

        DataHolder *data = new DataHolder;
        auto result = thirdOctave(ch->data()->decibels(0), ch->data()->xMin(), ch->data()->xStep());

        data->setXValues(result.first);
        data->setThreshold(ch->data()->threshold());
        data->setYValuesUnits(ch->data()->yValuesUnits());
        data->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);

        DataDescription descr = ch->dataDescription();
        descr.put("yname", "дБ");
        descr.put("xname", "Гц");
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", 1);
        descr.put("function.octaveFormat", 3);
        descr.put("function.name", "OCTF3");
        descr.put("function.logref", data->threshold());
        descr.put("function.format", "amplitudeDb");
        descr.put("function.logscale", DataHolder::unitsToString(data->yValuesUnits()));

        file->addChannelWithData(data, descr);

        if (!populated) ch->clear();
    }

    file->setChanged(true);
    file->setDataChanged(true);
    file->write();
}

QString saveTimeSegment(FileDescriptor *file, double from, double to)
{
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (file->type() != Descriptor::TimeResponse) return "";
    // и имел данные
    if (file->channelsCount() == 0) return "";

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString ext = QFileInfo(file->fileName()).suffix();
    QString newFileName = createUniqueFileName("", file->fileName(), suffix, ext, false);

    // 2 создаем новый файл
    auto newFile = App->formatFactory->createDescriptor(newFileName);

    newFile->setDataDescription(file->dataDescription());
    newFile->dataDescription().put("source.file", file->fileName());
    newFile->dataDescription().put("source.guid", file->dataDescription().get("guid"));
    newFile->dataDescription().put("source.dateTime", file->dataDescription().get("dateTime"));
    newFile->fillPreliminary(file);

    // 3 ищем границы данных по параметрам from и to
    Channel *ch = file->channel(0);
    int sampleStart = qRound((from - ch->data()->xMin())/ch->data()->xStep());
    if (sampleStart<0) sampleStart = 0;
    int sampleEnd = qRound((to - ch->data()->xMin())/ch->data()->xStep());
    if (sampleEnd >= ch->data()->samplesCount()) sampleEnd = ch->data()->samplesCount() - 1;

    // 4 сохраняем файл

    for (int i=0; i<file->channelsCount(); ++i) {
        Channel *c = file->channel(i);
        bool wasPopulated = c->populated();
        if (!wasPopulated) c->populate();

        DataHolder *data = new DataHolder();
        data->setSegment(*(c->data()), sampleStart, sampleEnd);

        DataDescription descr = c->dataDescription();
        descr.put("name", c->name()+" вырезка");
        descr.put("description", QString("Вырезка %1s-%2s").arg(fromString).arg(toString));
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", data->blocksCount());
        descr.put("function.name", "SECTION");
        descr.put("function.logref", data->threshold());
        newFile->addChannelWithData(data, descr);

        if (!wasPopulated)
            c->clear();
    }

    newFile->setChanged(true);
    newFile->setDataChanged(true);
    newFile->write();
    delete newFile;

    // 5 возвращаем имя нового файла
    return newFileName;
}

void saveSpectre(FileDescriptor *file, Channel *channel, double zValue)
{
    DataHolder *data = new DataHolder;

    bool populated = channel->populated();
    if (!populated) channel->populate();

    data->setThreshold(channel->data()->threshold());
    data->setYValuesUnits(channel->data()->yValuesUnits());

    if (channel->data()->xValuesFormat()==DataHolder::XValuesUniform)
        data->setXValues(channel->data()->xMin(), channel->data()->xStep(), channel->data()->samplesCount());
    else
        data->setXValues(channel->data()->xValues());

    //1 блок, так как вырезка спектра
    data->setZValues(channel->data()->zMin(), channel->data()->zStep(), 1);

    auto zIndex = channel->data()->nearestZ(zValue);

    auto format = channel->data()->yValuesFormat();
    if (format == DataHolder::YValuesComplex) {
        data->setYValues(channel->data()->yValuesComplex(zIndex));
    }
    else {
        QVector<double> values = channel->data()->linears(zIndex);
        if (format == DataHolder::YValuesAmplitudesInDB)
            format = DataHolder::YValuesAmplitudes;
        //только первый блок
        data->setYValues(values, format);
    }


    DataDescription descr = channel->dataDescription();
    descr.put("name", channel->name()+QLocale(QLocale::Russian).toString(channel->data()->zValue(zIndex)));
    descr.put("description", "Вырезка спектрограммы канала "+channel->name());
    descr.put("samples",  data->samplesCount());
    descr.put("blocks", 1);

    file->addChannelWithData(data, descr);
    if (!populated) channel->clear();


    file->setChanged(true);
    file->setDataChanged(true);
    file->write();
}

void saveThrough(FileDescriptor *file, Channel *channel, double xValue)
{
    DataHolder *data = new DataHolder;

    bool populated = channel->populated();
    if (!populated) channel->populate();

    data->setThreshold(channel->data()->threshold());
    data->setYValuesUnits(channel->data()->yValuesUnits());

    //z values становятся x values
    if (channel->data()->zValuesFormat()==DataHolder::XValuesUniform)
        data->setXValues(channel->data()->zMin(), channel->data()->zStep(), channel->data()->blocksCount());
    else
        data->setXValues(channel->data()->zValues());

    //1 блок, так как вырезка
    data->setZValues(0, 1, 1);

    auto xIndex = channel->data()->nearest(xValue);
    xValue = channel->data()->xValue(xIndex);

    auto format = channel->data()->yValuesFormat();
    if (format == DataHolder::YValuesComplex) {
        QVector<cx_double> values(channel->data()->blocksCount());
        for (int i=0; i<channel->data()->blocksCount(); ++i)
            values[i] = channel->data()->yValueComplex(xIndex, i);
        data->setYValues(values);
    }
    else {
        QVector<double> values(channel->data()->blocksCount());
        for (int i=0; i<channel->data()->blocksCount(); ++i)
            values[i] = channel->data()->yValue(xIndex, i);
        data->setYValues(values, format);
    }


    DataDescription descr = channel->dataDescription();
    descr.put("name", channel->name()+QLocale(QLocale::Russian).toString(channel->data()->xValue(xIndex)));
    descr.put("description", "Проходная спектрограммы канала "+channel->name());
    descr.put("samples",  data->samplesCount());
    descr.put("blocks", 1);
    descr.put("xname", descr.get("zname"));
    descr.put("zname", "");
    descr.put("function.name","SECTION");


    file->addChannelWithData(data, descr);
    if (!populated) channel->clear();


    file->setChanged(true);
    file->setDataChanged(true);
    file->write();
}
