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
    baseChannelSpin->setValue(2);

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
    Parameters p = SpectreMethod::parameters();

    p.saveAsComplex = saveAsComplexCheckBox->isChecked();
    p.baseChannel = baseChannelSpin->value();

    return p;
}

Function *FRFMethod::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{
    Function *ch = SpectreMethod::addUffChannel(newUff, dfd, spectrumSize, p, i);

    ch->setName(ch->name()+"/Сила");
    return ch;
}
