#include "spectrealgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "fftfunction.h"
#include "savingfunction.h"


SpectreAlgorithm::SpectreAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{
    channelF = new ChannelFunction(parent);
    filteringF = new FilteringFunction(parent);
    resamplingF = new ResamplingFunction(parent);
    samplingF = new FrameCutterFunction(parent);
    windowingF = new WindowingFunction(parent);
    averagingF = new AveragingFunction(parent);
    fftF = new FftFunction(parent);
    saver = new SavingFunction(parent);

    filteringF->setInput(channelF);
    resamplingF->setInput(filteringF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    fftF->setInput(windowingF);
    averagingF->setInput(fftF);
    saver->setInput(averagingF);

    m_functions << channelF;
    m_functions << filteringF;
    m_functions << resamplingF;
    m_functions << samplingF;
    m_functions << windowingF;
    m_functions << fftF;
    m_functions << averagingF;
    m_functions << saver;

    //выясняем общее значение xStep или значение первого файла
    double xStep = dataBase.first()->xStep();
    bool xStepsDiffer = false;
    for (int i=1; i<dataBase.size(); ++i) {
        if (xStep != dataBase.at(i)->xStep()) {
            xStepsDiffer = true;
            break;
        }
    }
    if (xStepsDiffer) emit message("Файлы имеют разный шаг по оси X.");

    //начальные значения, которые будут использоваться в показе функций
    resamplingF->setProperty(resamplingF->name()+"/xStep", xStep);
    samplingF->setProperty(samplingF->name()+"/xStep", xStep);

    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            samplingF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
    foreach (AbstractFunction *f, m_functions) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}


QString SpectreAlgorithm::name() const
{
    return "Spectrum";
}

QString SpectreAlgorithm::description() const
{
    return "Спектр";
}


QString SpectreAlgorithm::displayName() const
{
    return "Спектр";
}

bool SpectreAlgorithm::compute(FileDescriptor *file)
{
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    saver->setProperty(saver->name()+"/destination", QFileInfo(file->fileName()).canonicalPath());
    saver->reset();

    resamplingF->setProperty(resamplingF->name()+"/xStep", file->xStep());
    samplingF->setProperty(samplingF->name()+"/xStep", file->xStep());

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
        // for each channel
        saver->compute(file); //and collect the result

        if (!wasPopulated) file->channel(i)->clear();
        emit tick();
    }
    saver->reset();
    QString fileName = saver->getProperty(saver->name()+"/name").toString();
    qDebug()<<fileName;

    if (fileName.isEmpty()) return false;
    newFiles << fileName;
    return true;
}
