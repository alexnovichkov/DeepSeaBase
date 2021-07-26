#include "octavealgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "octavefunction.h"
#include "savingfunction.h"
#include "averagingfunction.h"
#include "framecutterfunction.h"
#include "logging.h"

OctaveAlgorithm::OctaveAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(parent);
    octaveF = new OctaveFunction(parent);
    saver = new SavingFunction(parent);
    averagingF = new AveragingFunction(parent);
    samplingF = new FrameCutterFunction(parent);

    m_chain << channelF;
    m_chain << saver;


    samplingF->setInput(channelF);
    octaveF->setInput(samplingF);
    averagingF->setInput(octaveF);
    saver->setInput(averagingF);

//    octaveF->setInput(channelF);
//    saver->setInput(octaveF);

    m_functions << channelF;
    m_functions << samplingF;
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

    samplingF->setParameter(samplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/blockSize"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            octaveF, SLOT(updateProperty(QString,QVariant)));
}

QString OctaveAlgorithm::description() const
{
    return "Расчет полосовых спектров";
}

QString OctaveAlgorithm::displayName() const
{
    return "OCTF";
}

void OctaveAlgorithm::resetChain()
{
    samplingF->reset();
    averagingF->reset();
}

void OctaveAlgorithm::initChain(FileDescriptor *file)
{
    Q_UNUSED(file);
    averagingF->setParameter(averagingF->name()+"/type", 1); //линейное усреднение
    samplingF->setParameter(samplingF->name()+"/xStep", file->xStep());
    samplingF->setParameter(samplingF->name()+"/type", 0);
}
