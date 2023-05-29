#include "spectrealgorithm.h"

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

SpectreAlgorithm::SpectreAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) :
    AbstractAlgorithm(dataBase, parent)
{DD0;
    channelF = new ChannelFunction(this);
//    filteringF = new FilteringFunction(this);
    resamplingF = new ResamplingFunction(this);
    samplingF = new FrameCutterFunction(this);
    windowingF = new WindowingFunction(this);
    averagingF = new AveragingFunction(this);
    fftF = new FftFunction(this);
    saver = new SavingFunction(this);

    m_chain << channelF;
    m_chain << saver;

//    filteringF->setInput(channelF);
    resamplingF->setInput(channelF);
    samplingF->setInput(resamplingF);
    windowingF->setInput(samplingF);
    fftF->setInput(windowingF);
    averagingF->setInput(fftF);
    saver->setInput(averagingF);

    m_functions << channelF;
//    m_functions << filteringF;
    m_functions << resamplingF;
    m_functions << samplingF;
    m_functions << windowingF;
    m_functions << fftF;
    m_functions << averagingF;
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

    //resamplingF отправляет сигнал об изменении "?/xStep"
    connect(resamplingF, &AbstractFunction::parameterChanged, samplingF, &AbstractFunction::updateParameter);
    //samplingF отправляет сигнал об изменении "?/triggerChannel"
    connect(samplingF, &AbstractFunction::parameterChanged, channelF, &AbstractFunction::updateParameter);

    //fftF отправляет сигнал об изменении ?/dataFormat
    connect(fftF, &AbstractFunction::parameterChanged, saver, &AbstractFunction::updateParameter);

//    connect(resamplingF, SIGNAL(parameterChanged(QString,QVariant)),
//            samplingF, SLOT(updateParameter(QString,QVariant)));
//    connect(samplingF, SIGNAL(parameterChanged(QString,QVariant)),
//            channelF, SLOT(updateParameter(QString,QVariant)));

    //начальные значения, которые будут использоваться в показе функций
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);  //автоматически задает xStep для samplingF
    channelF->setFile(dataBase.constFirst());
}


QString SpectreAlgorithm::description() const
{DD0;
    return "Спектр";
}


QString SpectreAlgorithm::displayName() const
{DD0;
    return "FFT";
}

void SpectreAlgorithm::resetChain()
{DD0;
    //        filteringF->reset();
    resamplingF->reset();
    samplingF->reset();
    windowingF->reset();
    fftF->reset();
    averagingF->reset();
}

void SpectreAlgorithm::initChain(FileDescriptor *file)
{DD0;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());  //автоматически задает xStep для samplingF
}
