#include "xresponch1.h"

#include <QtWidgets>

#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "algorithms.h"
#include "logging.h"
#include "windowing.h"

FRFMethod::FRFMethod(QList<FileDescriptor *> &dataBase, QWidget *parent) : SpectreMethod(dataBase, parent)
{
    baseChannelSpin = new QSpinBox(this);
    baseChannelSpin->setRange(1, 1000);
    baseChannelSpin->setValue(2);

    forceWindowParameter = new QLineEdit(this);
    forceWindowParameter->setPlaceholderText("50%");
    forceWindowParameter->setEnabled(false);

    forceWindowCombo = new QComboBox(this);
    forceWindowCombo->addItems(Windowing::windowDescriptions());
    forceWindowCombo->setCurrentIndex(2);
    forceWindowCombo->setEditable(false);
    connect(forceWindowCombo, QOverload<int>::of(&QComboBox::activated), [=](int index){
        forceWindowParameter->setEnabled(Windowing::windowAcceptsParameter(static_cast<Windowing::WindowType>(index)));
    });

    if (QFormLayout *l = qobject_cast<QFormLayout *>(layout())) {
        l->addRow("Опорный канал", baseChannelSpin);
        l->insertRow(5, "Окно канала силы", forceWindowCombo);
        l->insertRow(6, "Пар-р окна канала силы", forceWindowParameter);
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
    p.baseChannel = baseChannelSpin->value();

    p.forceWindowType = forceWindowCombo->currentIndex();
    QString percentString = forceWindowParameter->text().trimmed();
    if (percentString.endsWith("%")) percentString.chop(1);
    bool ok;
    double percent = percentString.toDouble(&ok);
    if (ok && percent != 0.0) p.forceWindowPercent = percent;

    return p;
}

Channel *FRFMethod::addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{
    Channel *ch = SpectreMethod::addUffChannel(newUff, dfd, spectrumSize, p, i);

    QString name = ch->name();
    if (name.endsWith("/")) name.chop(1);
    ch->setName(name+"/Сила");
    return ch;
}
