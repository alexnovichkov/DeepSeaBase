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
    channelF = new ChannelFunction(parent);
    samplingF = new FrameCutterFunction(parent);
    windowingF = new WindowingFunction(parent);
    saver = new SavingFunction(parent);

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
    samplingF->setProperty(samplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

    //перенаправляем сигналы от функций в интерфейс пользователя
    for (AbstractFunction *f: m_functions) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}

QString WindowingAlgorithm::name() const
{DD;
    return "Window";
}

QString WindowingAlgorithm::description() const
{DD;
    return "Оконная функция";
}

QString WindowingAlgorithm::displayName() const
{DD;
    return "WIN";
}

bool WindowingAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    saver->setProperty(saver->name()+"/destination", QFileInfo(file->fileName()).canonicalPath());
    saver->reset();

    samplingF->setProperty(samplingF->name()+"/xStep", file->xStep());

    const int count = file->channelsCount();
    for (int i=0; i<count; ++i) {
        const bool wasPopulated = file->channel(i)->populated();

        samplingF->reset();
        windowingF->reset();

        //beginning of the chain
        channelF->setProperty("Channel/channelIndex", i);

        //so far end of the chain
        // for each channel
        saver->setFile(file);
        saver->compute(file); //and collect the result

        if (!wasPopulated) file->channel(i)->clear();
        emit tick();
    }
    saver->reset();
    QString fileName = saver->getProperty(saver->name()+"/name").toString();
//    qDebug()<<fileName;

    if (fileName.isEmpty()) return false;
    newFiles << fileName;
    return true;
}
