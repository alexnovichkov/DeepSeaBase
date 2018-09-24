#include "xresponch1.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "algorithms.h"
#include "logging.h"

FRFMethod::FRFMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent) : SpectreMethod(dataBase, parent)
{
    saveAsComplexCheckBox = new QCheckBox("Сохранять комплексные значения", this);
    baseChannelSpin = new QSpinBox(this);
    baseChannelSpin->setRange(1, 1000);
    baseChannelSpin->setValue(1);

    if (QFormLayout *l = qobject_cast<QFormLayout *>(layout())) {
        l->addRow("Опорный канал", baseChannelSpin);
        l->addRow(saveAsComplexCheckBox);
    }

    connect(activeStripCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateResolution(int)));
    activeStripCombo->setCurrentIndex(0);
    updateResolution(0);
}


int FRFMethod::id()
{
    return 9;
}

QStringList FRFMethod::methodSettings(DfdFileDescriptor *dfd, const Parameters &p)
{
    QStringList spfFile;
    QString yName = "дБ";
    if (scaleCombo->currentText() != "в децибелах") {
        // TODO: реализовать правильный выбор единицы измерения
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=%1").arg(resolutionCombo->currentText());
    spfFile << "Wind="+windowCombo->currentText();
    spfFile << "TypeAver="+averCombo->currentText();

    int numberOfInd = dfd->channels.at(p.activeChannel)->samplesCount();
    double NumberOfAveraging = double(numberOfInd) / p.bufferSize / (1<<p.bandStrip);

    // at least 2 averaging
    if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
    spfFile << "Values=измеряемые";
    spfFile << "TypeScale="+scaleCombo->currentText();

    return spfFile;
}

QString FRFMethod::methodDll()
{
    return "XresponcH1.dll";
}

int FRFMethod::panelType()
{
    return 0;
}

QString FRFMethod::methodName()
{
    return "Передаточная ф-я H1";
}

int FRFMethod::dataType()
{
    return 147;
}

Parameters FRFMethod::parameters()
{
    Parameters p;
    p.sampleRate = sampleRate;
    p.averagingType = averCombo->currentIndex();

    const double po = pow(2.0, resolutionCombo->currentIndex());
    p.bufferSize = qRound(sampleRate / po); // размер блока
    p.windowType = windowCombo->currentIndex();
    p.scaleType = scaleCombo->currentIndex();
    p.saveAsComplex = saveAsComplexCheckBox->isChecked();
    p.baseChannel = baseChannelSpin->value();

    p.overlap = 1.0 * overlap->value() / 100;
    p.bandWidth = bandWidth;
    p.initialBandStripNumber = activeStripCombo->currentIndex();

    p.panelType = panelType();
    p.methodName = methodName();
    p.methodDll = methodDll();
    p.dataType = dataType();

    return p;
}

DescriptionList FRFMethod::processData(const Parameters &p)
{
    DescriptionList list;
    list.append({"PName", p.methodName});
    list.append({"BlockIn", QString::number(p.bufferSize)});
    list.append({"Wind", p.windowDescription()});
    list.append({"TypeAver", p.averaging(p.averagingType)});
    list.append({"pTime","(0000000000000000)"});
    return list;
}

Function *FRFMethod::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{
    Q_UNUSED(spectrumSize);

    Function *ch = SpectreMethod::addUffChannel(newUff, dfd, spectrumSize, p, i);

    ch->setName(ch->name()+"/Сила");
    return ch;
}
