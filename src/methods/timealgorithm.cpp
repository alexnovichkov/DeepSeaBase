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
    resamplingF->setProperty(resamplingF->name()+"/xStep", xStep);
    channelF->setFile(dataBase.constFirst());

    //resamplingF отправляет сигнал об изменении "?/xStep"
//    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
//            samplingF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
    for (AbstractFunction *f: qAsConst(m_functions)) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}


QString TimeAlgorithm::name() const
{DD;
    return "Time data";
}

QString TimeAlgorithm::description() const
{DD;
    return "Передискретизация временных данных";
}


QString TimeAlgorithm::displayName() const
{DD;
    return "RSMPL";
}

bool TimeAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    //TODO: проверить, не заменяет ли это установленный destination
    saver->setProperty(saver->name()+"/destination", QFileInfo(file->fileName()).canonicalPath());
    saver->reset(); //сохраняем предыдущий обработанный файл.

    resamplingF->setProperty(resamplingF->name()+"/xStep", file->xStep());

    const int count = file->channelsCount();
    for (int i=0; i<count; ++i) {
        const bool wasPopulated = file->channel(i)->populated();

        resamplingF->reset();

        //beginning of the chain
        channelF->setProperty(channelF->name()+"/channelIndex", i);

        //so far end of the chain
        // for each channel
        saver->setFile(file);
        saver->compute(file); //and collect the result

        if (!wasPopulated) file->channel(i)->clear();
        emit tick();
    }
    saver->reset(); //сохраняем последний обработанный файл
    QString fileName = saver->getProperty(saver->name()+"/name").toString();
//    qDebug()<<fileName;

    if (fileName.isEmpty()) return false;
    newFiles << fileName;
    return true;
}
