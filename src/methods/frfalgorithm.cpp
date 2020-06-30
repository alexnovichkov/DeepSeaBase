#include "frfalgorithm.h"
#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "frffunction.h"
#include "savingfunction.h"


FRFAlgorithm::FRFAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{
    channelF = new ChannelFunction(parent);
    resamplingF = new ResamplingFunction(parent);
    samplingF = new FrameCutterFunction(parent);
    windowingF = new WindowingFunction(parent);
    averagingF = new AveragingFunction(parent);
    frfF = new FrfFunction(parent);
    saver = new SavingFunction(parent);

    resamplingF->setInput(channelF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    frfF->setInput(windowingF);
    averagingF->setInput(frfF);
    saver->setInput(averagingF);

    m_functions << channelF;
    m_functions << resamplingF;
    m_functions << samplingF;
    m_functions << windowingF;
    m_functions << frfF;
    m_functions << averagingF;
    m_functions << saver;

    //выясняем общее значение xStep или значение первого файла
    double xStep = dataBase.constFirst()->xStep();
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
    channelF->setFile(dataBase.constFirst());

    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            samplingF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
    for (AbstractFunction *f: m_functions) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}

QString FRFAlgorithm::name() const
{
    return "FRF";
}

QString FRFAlgorithm::description() const
{
    return "Передаточная функция";
}

QString FRFAlgorithm::displayName() const
{
    return "FRF";
}

bool FRFAlgorithm::compute(FileDescriptor *file)
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

    const int count = file->channelsCount();
    for (int i=0; i<count; ++i) {
        const bool wasPopulated = file->channel(i)->populated();

        resamplingF->reset();
        samplingF->reset();
        windowingF->reset();
        frfF->reset();
        averagingF->reset();

        //beginning of the chain
        channelF->setProperty("Channel/channelIndex", i);

        //so far end of the chain
        // for each channel
        saver->setFile(file);
        saver->compute(file); //and collect the result

        if (!wasPopulated) file->channel(i)->clear();
        emit tick();
    }
    saver->reset();
    QString fileName = saver->getProperty(saver->name()+"/name").toString();
//    qDebug()<<fileName;

    if (fileName.isEmpty()) return false;
    newFiles << fileName;
    return true;
}
