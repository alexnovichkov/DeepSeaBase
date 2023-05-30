#include "filteringalgorithm.h"

#include "channelfunction.h"
#include "filteringfunction.h"
#include "savingfunction.h"
#include "logging.h"

FilteringAlgorithm::FilteringAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent)
    : AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(this);
    filteringF = new FilteringFunction(this);
    saver = new SavingFunction(this);

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
{DD;
    return "Фильтр временных данных";
}

QString FilteringAlgorithm::displayName() const
{DD;
    return "FILT";
}

void FilteringAlgorithm::resetChain()
{DD;
    filteringF->reset();
}

void FilteringAlgorithm::initChain(FileDescriptor *file)
{DD;
    Q_UNUSED(file);
}
