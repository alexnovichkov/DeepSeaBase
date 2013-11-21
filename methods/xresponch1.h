#ifndef XRESPONCH1_H
#define XRESPONCH1_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;

class XresponcH1Method : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    XresponcH1Method(QWidget *parent = 0);

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
    QComboBox *resolutionCombo; // Разрешение 512, 1024, 2048, 4096, 8192
    QComboBox *windowCombo; //Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QComboBox *averCombo; // Усреднение линейное, экспоненциальное
    QComboBox *nAverCombo; // Кол. усреднений 1 2 4 8 16 32 64 128 256 1024 до конца интервала
    QComboBox *valuesCombo; // Величины измеряемые, вход АЦП
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
};

#endif // XRESPONCH1_H
