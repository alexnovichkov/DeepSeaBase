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
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
private:
    int computeNumberOfAveraging(const QString &aver);
//    QComboBox *rangeCombo;
    QComboBox *resolutionCombo;
    QComboBox *windowCombo;
    QComboBox *averCombo;
    QComboBox *nAverCombo;
    QComboBox *typeCombo;
    QComboBox *valuesCombo;
    QComboBox *scaleCombo;
    QComboBox *addProcCombo;

    QString typeScale; //"Шкала" "линейная", "в децибелах"
   // int resolution; // Разрешение 512, 1024, 2048, 4096, 8192
    QString wind; //Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QString typeAver; // Усреднение линейное, экспоненциальное, хранение максимума
    int nAver; // Кол. усреднений 1 2 4 8 16 32 64 128 256 1024 до конца интервала
    QString typeProc; // Тип спектра "мощности", "плотности мощн.", "спектр СКЗ"
    QString values; // Величины измеряемые, вход АЦП
    QString addProc; //Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.
};

#endif // SPECTREMETHOD_H
