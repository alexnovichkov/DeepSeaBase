#include "windowingalgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "savingfunction.h"
#include "logging.h"


WindowingAlgorithm::WindowingAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(this);
    samplingF = new FrameCutterFunction(this);
    windowingF = new WindowingFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << saver;

    samplingF->setInput(channelF);
    windowingF->setInput(samplingF);
    saver->setInput(windowingF);

    m_functions << channelF;
    m_functions << samplingF;
    m_functions << windowingF;
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
    samplingF->setParameter(samplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());
}

QString WindowingAlgorithm::description() const
{DD;
    return "Оконная функция";
}

QString WindowingAlgorithm::displayName() const
{DD;
    return "WIN";
}

void WindowingAlgorithm::resetChain()
{
    samplingF->reset();
    windowingF->reset();
}

void WindowingAlgorithm::initChain(FileDescriptor *file)
{
    samplingF->setParameter(samplingF->name()+"/xStep", file->xStep());
}
