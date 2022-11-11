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
    channelF = new ChannelFunction(this);
//    filteringF = new FilteringFunction(this);
    resamplingF = new ResamplingFunction(this);
    samplingF = new FrameCutterFunction(this);
    windowingF = new WindowingFunction(this);
    averagingF = new AveragingFunction(this);
    psF = new PsFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << saver;

//    filteringF->setInput(channelF);
    resamplingF->setInput(channelF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    psF->setInput(windowingF);
    averagingF->setInput(psF);
    saver->setInput(averagingF);

    m_functions << channelF;
//    m_functions << filteringF;
    m_functions << resamplingF;
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

    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            samplingF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));

    //начальные значения, которые будут использоваться в показе функций
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);  //автоматически задает xStep для samplingF
    channelF->setFile(dataBase.constFirst());
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
{DD;
    samplingF->reset();
    windowingF->reset();
    psF->reset();
    averagingF->reset();
    resamplingF->reset();
}

void PsAlgorithm::initChain(FileDescriptor *file)
{DD;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());  //автоматически задает xStep для samplingF
}
