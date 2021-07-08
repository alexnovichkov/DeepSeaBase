#include "psalgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "psfunction.h"
#include "savingfunction.h"
#include "logging.h"

PsAlgorithm::PsAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(parent);
//    filteringF = new FilteringFunction(parent);
//    resamplingF = new ResamplingFunction(parent);
    samplingF = new FrameCutterFunction(parent);
    windowingF = new WindowingFunction(parent);
    averagingF = new AveragingFunction(parent);
    psF = new PsFunction(parent);
    saver = new SavingFunction(parent);

//    filteringF->setInput(channelF);
//    resamplingF->setInput(filteringF);
    samplingF->setInput(channelF);
    windowingF->setInput(samplingF);
    psF->setInput(windowingF);
    averagingF->setInput(psF);
    saver->setInput(averagingF);

    m_functions << channelF;
//    m_functions << filteringF;
//    m_functions << resamplingF;
    m_functions << samplingF;
    m_functions << windowingF;
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
//    resamplingF->setProperty(resamplingF->name()+"/xStep", xStep);
    samplingF->setProperty(samplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

//    //resamplingF отправляет сигнал об изменении "?/xStep"
//    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
//            samplingF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
    for (AbstractFunction *f: m_functions) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}


QString PsAlgorithm::name() const
{DD;
    return "Spectrum";
}

QString PsAlgorithm::description() const
{DD;
    return "Спектр мощности";
}


QString PsAlgorithm::displayName() const
{DD;
    return "PS";
}

bool PsAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    saver->setProperty(saver->name()+"/destination", QFileInfo(file->fileName()).canonicalPath());
    saver->reset();

//    resamplingF->setProperty(resamplingF->name()+"/xStep", file->xStep());
    samplingF->setProperty(samplingF->name()+"/xStep", file->xStep());

    const int count = file->channelsCount();
    for (int i=0; i<count; ++i) {
        const bool wasPopulated = file->channel(i)->populated();

//        filteringF->reset();
//        resamplingF->reset();
        samplingF->reset();
        windowingF->reset();
        psF->reset();
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
