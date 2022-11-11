#include "resamplingalgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "fftfunction.h"
#include "savingfunction.h"
#include "logging.h"


ResamplingAlgorithm::ResamplingAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(this);
    resamplingF = new ResamplingFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << saver;

    resamplingF->setInput(channelF);
    saver->setInput(resamplingF);

    m_functions << channelF;
    m_functions << resamplingF;
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
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

    //resamplingF отправляет сигнал об изменении "?/xStep"
//    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
//            samplingF, SLOT(updateProperty(QString,QVariant)));
}

QString ResamplingAlgorithm::description() const
{DD;
    return "Передискретизация временных данных";
}


QString ResamplingAlgorithm::displayName() const
{DD;
    return "RSMPL";
}

void ResamplingAlgorithm::resetChain()
{DD;
    resamplingF->reset();
}

void ResamplingAlgorithm::initChain(FileDescriptor *file)
{DD;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());
}
