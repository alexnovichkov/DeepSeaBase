#include "rmsalgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "psfunction.h"
#include "rmsfunction.h"
#include "savingfunction.h"
#include "logging.h"

RmsAlgorithm::RmsAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(this);
    resamplingF = new ResamplingFunction(this);
    samplingF = new FrameCutterFunction(this);
    windowingF = new WindowingFunction(this);
    averagingF = new AveragingFunction(this);
    psF = new PsFunction(this);
    rmsF = new RmsFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << saver;

    resamplingF->setInput(channelF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    psF->setInput(windowingF);
    averagingF->setInput(psF);
    rmsF->setInput(averagingF);
    saver->setInput(rmsF);

    m_functions << channelF;
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
    connect(resamplingF, &AbstractFunction::parameterChanged, samplingF, &AbstractFunction::updateParameter);
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, &AbstractFunction::parameterChanged, channelF, &AbstractFunction::updateParameter);
    //averagingF отправляет сигнал об изменении ?/averagingType
    connect(averagingF, &AbstractFunction::parameterChanged, saver, &AbstractFunction::updateParameter);

    //начальные значения, которые будут использоваться в показе функций
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);  //автоматически задает xStep для samplingF
    channelF->setFile(dataBase.constFirst());
}


QString RmsAlgorithm::description() const
{DD;
    return "Спектр СКЗ";
}


QString RmsAlgorithm::displayName() const
{DD;
    return "RMS";
}

void RmsAlgorithm::resetChain()
{DD;
    samplingF->reset();
    windowingF->reset();
    psF->reset();
    rmsF->reset();
    averagingF->reset();
    resamplingF->reset();
}

void RmsAlgorithm::initChain(FileDescriptor *file)
{DD;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());  //автоматически задает xStep для samplingF
}
