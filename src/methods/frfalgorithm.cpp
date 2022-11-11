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
    channelF = new ChannelFunction(this, "channel");
    refChannelF = new RefChannelFunction(this, "refChannel");

    resamplingF = new ResamplingFunction(this, "resample");
    refResamplingF = new ResamplingFunction(this, "refResample");
    resamplingF->pairWith(refResamplingF);

    samplingF = new FrameCutterFunction(this, "cutter");
    refSamplingF = new FrameCutterFunction(this, "refCutter");
    samplingF->pairWith(refSamplingF);

    windowingF = new WindowingFunction(this, "window");
    refWindowingF = new RefWindowingFunction(this, "refWindow");

    averagingF = new AveragingFunction(this, "average");
    refAveragingF = new AveragingFunction(this, "refAverage");
    averagingF->pairWith(refAveragingF);

    fftF = new FftFunction(this, "fft");
    refFftF = new FftFunction(this, "refFft");
    fftF->pairWith(refFftF);

    gxyF = new GxyFunction(this, "gxy");
    refGxyF = new GxyFunction(this, "refGxy");
    frfF = new FrfFunction(this, "frf");
    saver = new SavingFunction(this, "saver");

    ////
    m_chain << channelF;
    m_chain << saver;

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
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep); //автоматически задает xStep для samplingF
    channelF->setFile(dataBase.constFirst());
    refChannelF->setFile(dataBase.constFirst());
    refChannelF->setParameter(refChannelF->name()+"/referenceChannelIndex", 1);

//    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            samplingF, SLOT(updateProperty(QString,QVariant)));
    connect(resamplingF, SIGNAL(propertyChanged(QString,QVariant)),
            refSamplingF, SLOT(updateProperty(QString,QVariant)));

    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            channelF, SLOT(updateProperty(QString,QVariant)));
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, SIGNAL(propertyChanged(QString,QVariant)),
            refChannelF, SLOT(updateProperty(QString,QVariant)));
}

QString FRFAlgorithm::description() const
{DD;
    return "Передаточная функция";
}

QString FRFAlgorithm::displayName() const
{DD;
    return "FRF";
}

void FRFAlgorithm::resetChain()
{DD;
    resamplingF->reset();
    samplingF->reset();
    windowingF->reset();
    frfF->reset();
    averagingF->reset();
    refResamplingF->reset();
    refSamplingF->reset();
    refWindowingF->reset();
    refAveragingF->reset();
}

void FRFAlgorithm::initChain(FileDescriptor *file)
{DD;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());  //автоматически задает xStep для samplingF

    int frfType = saver->getParameter("FRF/type").toInt();
    if (frfType == 1) {
        //compute H2, needs to readjust flows
        gxyF->setInput(fftF); //<- computes S_BB, A - reference channel
        gxyF->setInput2(fftF);
        refGxyF->setInput(fftF); //<- computes S_BA, A - reference channel
        refGxyF->setInput2(refFftF);
    }
    else {
        //compute H1, needs to readjust flows
        gxyF->setInput(refFftF); //<- computes S_AB, A - reference channel
        gxyF->setInput2(fftF);

        refGxyF->setInput(refFftF); //<- computes S_AA, A - reference channel
        refGxyF->setInput2(refFftF);
    }
}
