#include "frfalgorithm.h"
#include "fileformats/filedescriptor.h"
#include "channelfunction.h"
#include "filteringfunction.h"
#include "resamplingfunction.h"
#include "framecutterfunction.h"
#include "windowingfunction.h"
#include "averagingfunction.h"
#include "frffunction.h"
#include "fftfunction.h"
#include "savingfunction.h"
#include "apsfunction.h"
#include "logging.h"
#include "gxyfunction.h"

FRFAlgorithm::FRFAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD;
    channelF = new ChannelFunction(parent, "channel");
    refChannelF = new RefChannelFunction(parent, "refChannel");

    resamplingF = new ResamplingFunction(parent, "resample");
    refResamplingF = new ResamplingFunction(parent, "refResample");
    resamplingF->pairWith(refResamplingF);

    samplingF = new FrameCutterFunction(parent, "cutter");
    refSamplingF = new FrameCutterFunction(parent, "refCutter");
    samplingF->pairWith(refSamplingF);

    windowingF = new WindowingFunction(parent, "window");
    refWindowingF = new RefWindowingFunction(parent, "refWindow");

    averagingF = new AveragingFunction(parent, "average");
    refAveragingF = new AveragingFunction(parent, "refAverage");
    averagingF->pairWith(refAveragingF);

    fftF = new FftFunction(parent, "fft");
    refFftF = new FftFunction(parent, "refFft");
    fftF->pairWith(refFftF);

    gxyF = new GxyFunction(parent, "gxy");
    refGxyF = new GxyFunction(parent, "refGxy");

    frfF = new FrfFunction(parent, "frf");

    saver = new SavingFunction(parent, "saver");

    //first chain
    resamplingF->setInput(channelF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    fftF->setInput(windowingF);

    //second chain
    refResamplingF->setInput(refChannelF);
    refSamplingF->setInput(refResamplingF);
    refWindowingF->setInput(refSamplingF);
    refFftF->setInput(refWindowingF);

    //weave together, to compute H1 by default
    gxyF->setInput(refFftF); //<- computes S_AB, A - reference channel
    gxyF->setInput2(fftF);

    refGxyF->setInput(refFftF); //<- computes S_AA, A - reference channel
    refGxyF->setInput2(refFftF);

    //again two chains
    averagingF->setInput(gxyF);
    refAveragingF->setInput(/*apsF*/ refGxyF);

    frfF->setInput(averagingF);
    frfF->setInput2(refAveragingF);

    saver->setInput(frfF);

    m_functions << channelF;
    m_functions << refChannelF;

    m_functions << resamplingF;

    m_functions << samplingF;

    m_functions << windowingF;
    m_functions << refWindowingF;

    //m_functions << fftF;
    //m_functions << refFftF;

    m_functions << gxyF;
    m_functions << averagingF;
    m_functions << frfF;
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

    samplingF->setProperty(samplingF->name()+"/xStep", xStep);

    channelF->setFile(dataBase.constFirst());
    refChannelF->setFile(dataBase.constFirst());
    refChannelF->setProperty(refChannelF->name()+"/referenceChannelIndex", 1);

    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            samplingF, SLOT(updateProperty(QString,QVariant)));
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            refSamplingF, SLOT(updateProperty(QString,QVariant)));

    //перенаправляем сигналы от функций в интерфейс пользователя
    for (AbstractFunction *f: m_functions) {
        connect(f, SIGNAL(attributeChanged(QString,QVariant,QString)),SIGNAL(attributeChanged(QString,QVariant,QString)));
        connect(f, SIGNAL(tick()), this, SIGNAL(tick()));
    }
}

QString FRFAlgorithm::name() const
{DD;
    return "FRF";
}

QString FRFAlgorithm::description() const
{DD;
    return "Передаточная функция";
}

QString FRFAlgorithm::displayName() const
{DD;
    return "FRF";
}

bool FRFAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    saver->setProperty(saver->name()+"/destination", QFileInfo(file->fileName()).canonicalPath());
    saver->reset();

    resamplingF->setProperty(resamplingF->name()+"/xStep", file->xStep());
    samplingF->setProperty(samplingF->name()+"/xStep", file->xStep());

    int frfType = saver->getProperty("FRF/type").toInt();
    if (frfType == 1) {
        //compute H2, needs to readjust flows
        gxyF->setInput(fftF); //<- computes S_BB, A - reference channel
        gxyF->setInput2(fftF);
        refGxyF->setInput(fftF); //<- computes S_BA, A - reference channel
        refGxyF->setInput2(refFftF);
    }

    const int count = file->channelsCount();
    const int refChannel = refChannelF->getProperty("RefChannel/referenceChannelIndex").toInt()-1;
    const QStringList channels = channelF->getProperty("?/channels").toStringList();

    for (int i=0; i<count; ++i) {
        //beginning of the chain
        channelF->setProperty("Channel/channelIndex", i);

        if (!channels.isEmpty() && !channels.contains(QString::number(i+1))) {
            emit tick();
            continue;
        }

        if (refChannel == i) {
            emit tick();
            continue;
        }


        const bool wasPopulated = file->channel(i)->populated();

        resamplingF->reset();
        samplingF->reset();
        windowingF->reset();
        frfF->reset();
        averagingF->reset();
        refResamplingF->reset();
        refSamplingF->reset();
        refWindowingF->reset();
        refAveragingF->reset();




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
