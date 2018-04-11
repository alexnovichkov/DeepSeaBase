#ifndef OCTAVEMETHOD_H
#define OCTAVEMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;
class QSpinBox;

class OctaveMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit OctaveMethod(QWidget *parent = 0);

    // AbstractMethod interface
public:
    virtual int id();
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, const Parameters &p);
    //virtual QStringList settings(DfdFileDescriptor *dfd, int strip);
    virtual Parameters parameters();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();

private:
    QSpinBox *resolutionSpin;//мин. количество отсчетов 512, 1024, 2048, 4096, 8192
    QComboBox *placeCombo;// выбор отсчетов СКЗ: все отсчеты, начало интервала, конец интервала, синхронные
    QComboBox *typeCombo;// Тип "1/3-октава", "октава"
    QComboBox *valuesCombo;// Величины измеряемые, вход АЦП
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
    //QComboBox *addProcCombo;//Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.
};

#endif // OCTAVEMETHOD_H
