#include "spectrefunction.h"

#include "filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "fftfunction.h"


SpectreFunction::SpectreFunction(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractFunction(dataBase, parent)
{
    channelF = new ChannelFunction(dataBase, parent);
    filteringF = new FilteringFunction(dataBase, parent);
    resamplingF = new ResamplingFunction(dataBase, parent);
    samplingF = new FrameCutterFunction(dataBase, parent);
    windowingF = new WindowingFunction(dataBase, parent);
    averagingF = new AveragingFunction(dataBase, parent);
    fftF = new FftFunction(dataBase, parent);

    filteringF->setInput(channelF);
    resamplingF->setInput(filteringF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    fftF->setInput(windowingF);
    averagingF->setInput(fftF);

    m_functions << channelF;
    m_functions << filteringF;
    m_functions << resamplingF;
    m_functions << samplingF;
    m_functions << windowingF;
    m_functions << fftF;
    m_functions << averagingF;

}


QString SpectreFunction::name() const
{
    return "Spectrum";
}

QString SpectreFunction::description() const
{
    return "Спектр";
}

QStringList SpectreFunction::properties() const
{
    return QStringList();
}

QString SpectreFunction::propertyDescription(const QString &property) const
{
    Q_UNUSED(property);
    return "";
}

QVariant SpectreFunction::getProperty(const QString &property) const
{
    foreach (AbstractFunction *f, functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->getProperty(property);
        }
    }

    return QVariant();
}

void SpectreFunction::setProperty(const QString &property, const QVariant &val)
{
    foreach (AbstractFunction *f, functions()) {
        if (property.startsWith(f->name()+"/")) {
            f->setProperty(property, val);
        }
    }
}

bool SpectreFunction::propertyShowsFor(const QString &property) const
{
    foreach (AbstractFunction *f, functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->propertyShowsFor(property);
        }
    }

    return true;
}


QString SpectreFunction::displayName() const
{
    return "Спектр";
}

//void SpectreFunction::start()
//{
//    emit finished();
//}


bool SpectreFunction::compute(FileDescriptor *file, const QString &tempFolderName)
{
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    channelF->setProperty("Channel/fileIndex", m_dataBase.indexOf(file));


//    // trigger channel needs special excruciating treatment
//    QVector<double> triggerData;
//    const int triggerChannel = samplingF->getProperty("FrameCutter/channel").toInt()-1;
//    if (samplingF->getProperty("frameCutter/type").toInt() == FrameCutter::Trigger &&
//        triggerChannel >=0 && triggerChannel < file->channelsCount()) {
//        //we need to downsample data from trigger channel if we want to use it alongside with
//        //downsampled current channel data
//        const bool triggerChannelWasPopulated = file->channel(triggerChannel)->populated();
//        file->channel(triggerChannel)->populate();
//        triggerData = file->channel(triggerChannel)->yValues();
//        resamplingF->reset();
//        triggerData = resamplingF->get(file, triggerData);
//        samplingF->set
//        if (!triggerChannelWasPopulated) file->channel(triggerChannel)->clear();
//    }

    for (int i=0; i<file->channelsCount(); ++i) {
        const bool wasPopulated = file->channel(i)->populated();

        filteringF->reset();
        resamplingF->reset();
        samplingF->reset();
        windowingF->reset();
        fftF->reset();
        averagingF->reset();

        //beginning of the chain
        channelF->setProperty("Channel/channelIndex", i);

        //so far end of the chain
        averagingF->compute();
        QVector<double> data = averagingF->getData("input");
        //qDebug()<<data.size()<<data.mid(0,300);
        for (int y = 0; y<data.size(); ++y) qDebug()<<data[y];


        //channelF->compute(file, tempFolderName);

        //QVector<double> data = channelF->get(file, QVector<double>()<<i);
        //if (data.isEmpty()) continue;


        //1. Фильтрация - канал целиком
//        data = filteringF->get(file, data);


        //2. Ресамплинг - канал целиком
//        data = resamplingF->get(file, data);

        //3. Разбиение на блоки

//        samplingF->setProperty("blockSize", resamplingF->getProperty("blockSize"));
//        while (1) {
//            QVector<double> chunk = samplingF->get(file, data);
//            if (chunk.isEmpty()) break;

//            // 4. окно
//            chunk = windowingF->get(file, chunk);

//            // 5. FFT
//            QVector<double> fft = Fft::computeReal(chunk);

//            //6. type
//            switch (map.value("type")) {
//                case 0:
//                    // FFT - do nothing
//                    averagingF->get(file, fft);
//                    break;
//                case 1:
//                    // Спектр мощности

//                    break;

//                case 2:
//                    // Спектральная плотность мощности

//                    break;
//            }
//        }
    }

    //savingF->get()
    return false;
}


QVector<double> SpectreFunction::getData(const QString &id)
{
    return QVector<double>();
}
