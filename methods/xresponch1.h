#ifndef XRESPONCH1_H
#define XRESPONCH1_H

#include <QWidget>
#include "spectremethod.h"

class QComboBox;
class QCheckBox;
class QSpinBox;

class FRFMethod : public SpectreMethod
{
    Q_OBJECT
public:
    FRFMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent = 0);

    // AbstractMethod interface
public:
    virtual int id();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
    virtual Parameters parameters();
    virtual Function *addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i);

private:
    QSpinBox *baseChannelSpin;
};

#endif // XRESPONCH1_H
