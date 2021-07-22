#ifndef SPECTREMETHOD_H
#define SPECTREMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;
class FileDescriptor;
class QSpinBox;
class QCheckBox;
class QLineEdit;

class SpectreMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit SpectreMethod(QList<FileDescriptor *> &dataBase, QWidget *parent = 0);

    // AbstractMethod interface
public:
    virtual int id();
    virtual QStringList methodSettings(FileDescriptor *dfd, const Parameters &p);
    virtual Parameters parameters();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
    virtual DescriptionList processData(const Parameters &p);
private slots:
    void updateResolution(int);
private:
    friend class FRFMethod;
    QSpinBox *overlap;
    QComboBox *activeStripCombo;
    double bandWidth;
    double sampleRate;

    QComboBox *resolutionCombo;// Размер буфера  (разрешение)
    QComboBox *windowCombo;//Окно "Прямоуг.", "Бартлетта", "Хеннинга", "Хемминга", "Натолл", "Гаусс"
    QLineEdit *windowParameter;// Параметр для окна
    QComboBox *averCombo;// Усреднение линейное, экспоненциальное, хранение максимума
    QComboBox *nAverCombo;// Кол. усреднений 1 2 4 8 16 32 64 128 256 1024 до конца интервала
    QComboBox *typeCombo;// Тип спектра "мощности", "плотности мощн.", "спектр СКЗ"
    QComboBox *scaleCombo; //"Шкала" "линейная", "в децибелах"
    QComboBox *addProcCombo;//Доп. обработка нет, интегрир., дифференц., дв.интергир., дв.дифференц.
    QCheckBox *saveAsComplexCheckBox;

    // AbstractMethod interface
public:
//    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p);
//    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, FileDescriptor *dfd, Parameters &p);
//    virtual Channel *createDfdChannel(DfdFileDescriptor *newDfd, FileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i);
//    Channel * addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i);
};

#endif // SPECTREMETHOD_H
