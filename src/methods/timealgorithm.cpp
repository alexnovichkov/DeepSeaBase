#include "timealgorithm.h"

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


TimeAlgorithm::TimeAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(parent);
    resamplingF = new ResamplingFunction(parent);
    saver = new SavingFunction(parent);

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

    //перенаправляем сигналы от функций в интерфейс пользователя
//    for (AbstractFunction *f: qAsConst(m_functions)) {
//        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
//        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
//    }
}

QString TimeAlgorithm::description() const
{DD;
    return "Передискретизация временных данных";
}


QString TimeAlgorithm::displayName() const
{DD;
    return "RSMPL";
}

void TimeAlgorithm::resetChain()
{
    resamplingF->reset();
}

void TimeAlgorithm::initChain(FileDescriptor *file)
{
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());
}
