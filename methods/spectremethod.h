#ifndef SPECTREMETHOD_H
#define SPECTREMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;
class DfdFileDescriptor;
class QSpinBox;

class SpectreMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit SpectreMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent = 0);

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
private slots:
    void updateResolution(int);
private:
//    QSpinBox *activeChannelSpin;
//    QSpinBox *baseChannelSpin;
    QSpinBox *overlap;
    QComboBox *activeStripCombo;
    double bandWidth;
    double sampleRate;

    QComboBox *resolutionCombo;// Размер буфера  (разрешение)
    QComboBox *windowCombo;//Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QComboBox *averCombo;// Усреднение линейное, экспоненциальное, хранение максимума
    QComboBox *nAverCombo;// Кол. усреднений 1 2 4 8 16 32 64 128 256 1024 до конца интервала
    QComboBox *typeCombo;// Тип спектра "мощности", "плотности мощн.", "спектр СКЗ"
    QComboBox *valuesCombo;// Величины измеряемые, вход АЦП
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
    QComboBox *addProcCombo;//Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.

    // AbstractMethod interface
public:
    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual DfdChannel *createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i);
    Function * addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i);
};

#endif // SPECTREMETHOD_H
