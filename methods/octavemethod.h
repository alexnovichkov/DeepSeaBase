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
    explicit OctaveMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent = 0);

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
    virtual DescriptionList processData(const Parameters &p);

private:
    QSpinBox *resolutionSpin;//мин. количество отсчетов 512, 1024, 2048, 4096, 8192
    QComboBox *placeCombo;// выбор отсчетов СКЗ: все отсчеты, начало интервала, конец интервала, синхронные
    QComboBox *typeCombo;// Тип "1/3-октава", "октава"
    QComboBox *valuesCombo;// Величины измеряемые, вход АЦП
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
    //QComboBox *addProcCombo;//Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.

    // AbstractMethod interface
public:
    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual DfdChannel *createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i);
    virtual Function *addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, quint32 spectrumSize, Parameters &p, int i);
};

#endif // OCTAVEMETHOD_H
