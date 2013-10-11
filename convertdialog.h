#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>

#include "dfdfiledescriptor.h"

class AbstractMethod
{
public:
    virtual int id() = 0;
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip) = 0;
    virtual QString methodDll() = 0;
    virtual int panelType() = 0;
    virtual QString methodName() = 0;
    virtual int dataType() = 0;
    QList<DfdFileDescriptor *> *dataBase;
    int strip;
};

class QComboBox;
class QSpinBox;
class QStackedWidget;
class QProcess;

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

class ConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConvertDialog(QList<DfdFileDescriptor *> *dataBase, QWidget *parent = 0);
    ~ConvertDialog();
    QStringList getNewFiles() const {return newFiles;}
    QStringList getSpfFile(const QVector<int> &indexes, QString dir);
signals:
    
public slots:
    void methodChanged(int method);
private:
    QList<DfdFileDescriptor *> *dataBase;

    QStringList newFiles;

    int method; // 0-25

    QVector<int> channels;

    AbstractMethod *currentMethod;

    QComboBox *methodCombo;
    QSpinBox *activeChannelSpin;
    QSpinBox *baseChannelSpin;
    QStackedWidget *methodsStack;
    QComboBox *activeStripCombo;

    QProcess *process;

    // QDialog interface
public slots:
    virtual void accept();
};

#endif // CONVERTDIALOG_H
