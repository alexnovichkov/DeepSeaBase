#include "octavealgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "octavefunction.h"
#include "savingfunction.h"
#include "averagingfunction.h"
#include "framecutterfunction.h"
#include "logging.h"

//#define NO_BLOCKS

OctaveAlgorithm::OctaveAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(this);
    octaveF = new OctaveFunction(this);
    saver = new SavingFunction(this);
#ifndef NO_BLOCKS
    averagingF = new AveragingFunction(this);
    samplingF = new FrameCutterFunction(this);
#endif
    m_chain << channelF;
    m_chain << saver;

#ifndef NO_BLOCKS
    samplingF->setInput(channelF);
    octaveF->setInput(samplingF);
    averagingF->setInput(octaveF);
    saver->setInput(averagingF);
#else
    octaveF->setInput(channelF);
    saver->setInput(octaveF);
#endif

    m_functions << channelF;
    if (samplingF) m_functions << samplingF;
    m_functions << octaveF;
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

    if (samplingF) samplingF->setParameter(samplingF->name()+"/xStep", xStep);

    channelF->setFile(dataBase.constFirst());

    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    if (samplingF) connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/blockSize"
    if (samplingF) connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            octaveF, SLOT(updateProperty(QString,QVariant)));
}

QString OctaveAlgorithm::description() const
{DD;
    return "Расчет полосовых спектров";
}

QString OctaveAlgorithm::displayName() const
{DD;
    return "OCTF";
}

void OctaveAlgorithm::resetChain()
{DD;
    if (samplingF) samplingF->reset();
    if (averagingF) averagingF->reset();
}

void OctaveAlgorithm::initChain(FileDescriptor *file)
{DD;
    Q_UNUSED(file);
    if (averagingF) averagingF->setParameter(averagingF->name()+"/type", 1); //линейное усреднение
    if (samplingF) samplingF->setParameter(samplingF->name()+"/xStep", file->xStep());
    if (samplingF) samplingF->setParameter(samplingF->name()+"/type", 0);
}
