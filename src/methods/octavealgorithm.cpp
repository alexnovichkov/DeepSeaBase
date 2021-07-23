#include "octavealgorithm.h"

#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "octavefunction.h"
#include "savingfunction.h"
#include "logging.h"

OctaveAlgorithm::OctaveAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(parent);
    octaveF = new OctaveFunction(parent);
    saver = new SavingFunction(parent);

    m_chain << channelF;
    m_chain << saver;

    octaveF->setInput(channelF);
    saver->setInput(octaveF);

    m_functions << channelF;
    m_functions << octaveF;
    m_functions << saver;

    channelF->setFile(dataBase.constFirst());
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
    //no-op
}

void OctaveAlgorithm::initChain(FileDescriptor *file)
{
    Q_UNUSED(file);
}
