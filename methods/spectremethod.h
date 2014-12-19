#ifndef SPECTREMETHOD_H
#define SPECTREMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;

class SpectreMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit SpectreMethod(QWidget *parent = 0);

    // AbstractMethod interface
public:
    virtual int id();
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip);
    virtual QStringList settings(DfdFileDescriptor *dfd, int strip);
    virtual Parameters parameters(DfdFileDescriptor *dfd);
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();

private:
    int computeNumberOfAveraging(const QString &aver);
//    QComboBox *rangeCombo;
    QComboBox *resolutionCombo;// Разрешение 512, 1024, 2048, 4096, 8192
    QComboBox *windowCombo;//Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QComboBox *averCombo;// Усреднение линейное, экспоненциальное, хранение максимума
    QComboBox *nAverCombo;// Кол. усреднений 1 2 4 8 16 32 64 128 256 1024 до конца интервала
    QComboBox *typeCombo;// Тип спектра "мощности", "плотности мощн.", "спектр СКЗ"
    QComboBox *valuesCombo;// Величины измеряемые, вход АЦП
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
    QComboBox *addProcCombo;//Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.
};

#endif // SPECTREMETHOD_H
