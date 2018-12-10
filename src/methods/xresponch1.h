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
    FRFMethod(QList<FileDescriptor *> &dataBase, QWidget *parent = 0);

    // AbstractMethod interface
public:
    virtual int id();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
    virtual Parameters parameters();
    virtual Function *addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i);

private:
    QSpinBox *baseChannelSpin;

    QComboBox *forceWindowCombo;//Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QLineEdit *forceWindowParameter;// Параметр для окна
};

#endif // XRESPONCH1_H
