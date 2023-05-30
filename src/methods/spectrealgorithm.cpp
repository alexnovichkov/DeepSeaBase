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
{DD;
    channelF = new ChannelFunction(this);
    resamplingF = new ResamplingFunction(this);
    resamplingF->setInput(channelF);
    samplingF = new FrameCutterFunction(this);
    samplingF->setInput(resamplingF);
    windowingF = new WindowingFunction(this);
    windowingF->setInput(samplingF);
    fftF = new FftFunction(this);
    fftF->setInput(windowingF);
    averagingF = new AveragingFunction(this);
    averagingF->setInput(fftF);
    saver = new SavingFunction(this);
    saver->setInput(averagingF);

    m_chain << channelF;
    m_chain << saver;

    m_functions << channelF;
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
    //averagingF отправляет сигнал об изменении ?/averagingType
    connect(averagingF, &AbstractFunction::parameterChanged, saver, &AbstractFunction::updateParameter);

    //начальные значения, которые будут использоваться в показе функций
    resamplingF->setParameter(resamplingF->name()+"/xStep", xStep);  //автоматически задает xStep для samplingF
    channelF->setFile(dataBase.constFirst());
}


QString SpectreAlgorithm::description() const
{DD;
    return "Спектр";
}


QString SpectreAlgorithm::displayName() const
{DD;
    return "FFT";
}

void SpectreAlgorithm::resetChain()
{DD;
    //        filteringF->reset();
    resamplingF->reset();
    samplingF->reset();
    windowingF->reset();
    fftF->reset();
    averagingF->reset();
}

void SpectreAlgorithm::initChain(FileDescriptor *file)
{DD;
    resamplingF->setParameter(resamplingF->name()+"/xStep", file->xStep());  //автоматически задает xStep для samplingF
}
