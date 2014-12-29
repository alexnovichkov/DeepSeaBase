#ifndef TIMEMETHOD_H
#define TIMEMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;

class TimeMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit TimeMethod(QWidget *parent = 0);

    virtual int id();
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip);
    virtual QStringList settings(DfdFileDescriptor *dfd, int bandStrip);
    virtual Parameters parameters();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
private:
//    QComboBox *rangeCombo;
    QComboBox *resolutionCombo;
//    QComboBox *windowCombo;
//    QComboBox *averCombo;
//    QComboBox *nAverCombo;
//    QComboBox *typeCombo;
//    QComboBox *valuesCombo;
//    QComboBox *scaleCombo;
//    QComboBox *addProcCombo;
};

#endif // TIMEMETHOD_H
