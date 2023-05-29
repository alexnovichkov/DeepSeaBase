#include "splitalgorithm.h"

#include "channelfunction.h"
#include "savingfunction.h"
#include "splitfunction.h"

SplitAlgorithm::SplitAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD0;
    channelF = new ChannelFunction(this);
//    filteringF = new FilteringFunction(this);
    splitF = new SplitFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << splitF;
    m_chain << saver;

    splitF->setInput(channelF);
    saver->setInput(splitF);

    m_functions << channelF;
    m_functions << splitF;
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
    //connect(splitF, &AbstractFunction::parameterChanged, samplingF, &AbstractFunction::updateParameter);
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    //connect(samplingF, &AbstractFunction::parameterChanged, channelF, &AbstractFunction::updateParameter);

    //начальные значения, которые будут использоваться в показе функций
    channelF->setFile(dataBase.constFirst());
}

QString SplitAlgorithm::description() const
{
    return "Разделение";
}

QString SplitAlgorithm::displayName() const
{
    return "SPLIT";
}

void SplitAlgorithm::resetChain()
{
    splitF->reset();
}

void SplitAlgorithm::initChain(FileDescriptor *file)
{DD;
    Q_UNUSED(file);
}
