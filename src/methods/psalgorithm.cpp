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

    m_chain << channelF;
    m_chain << saver;

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
//    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);
    samplingF->setParameter(samplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

//    //resamplingF отправляет сигнал об изменении "?/xStep"
//    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
//            samplingF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
//    for (AbstractFunction *f: m_functions) {
//        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
//        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
//    }
}


QString PsAlgorithm::description() const
{DD;
    return "Спектр мощности";
}


QString PsAlgorithm::displayName() const
{DD;
    return "PS";
}

void PsAlgorithm::resetChain()
{
    samplingF->reset();
    windowingF->reset();
    psF->reset();
    averagingF->reset();
}

void PsAlgorithm::initChain(FileDescriptor *file)
{
    //    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());
    samplingF->setParameter(samplingF->name()+"/xStep", file->xStep());
}
