#include "filteringalgorithm.h"

#include "channelfunction.h"
#include "filteringfunction.h"
#include "savingfunction.h"
#include "logging.h"

FilteringAlgorithm::FilteringAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent)
    : AbstractAlgorithm(dataBase, parent)
{
    channelF = new ChannelFunction(parent);
    filteringF = new FilteringFunction(parent);
    saver = new SavingFunction(parent);

    m_chain << channelF;
    m_chain << saver;

    filteringF->setInput(channelF);
    saver->setInput(filteringF);

    m_functions << channelF;
    m_functions << filteringF;
    m_functions << saver;

    channelF->setFile(dataBase.constFirst());
}

QString FilteringAlgorithm::description() const
{
    return "Фильтр временных данных";
}

QString FilteringAlgorithm::displayName() const
{
    return "FILT";
}

void FilteringAlgorithm::resetChain()
{
    filteringF->reset();
}

void FilteringAlgorithm::initChain(FileDescriptor *file)
{
    Q_UNUSED(file);
}
