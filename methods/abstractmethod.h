#ifndef ABSTRACTMETHOD_H
#define ABSTRACTMETHOD_H

#include <QStringList>
#include <QObject>

class DfdFileDescriptor;
class Parameters;
class UffFileDescriptor;
class DfdChannel;
class Function;

#include "filedescriptor.h"

class AbstractMethod
{
public:
    explicit AbstractMethod(QList<DfdFileDescriptor *> &dataBase) {
        this->dataBase = dataBase;
    }
    /* индекс в methodCombo */
    virtual int id() = 0;
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, const Parameters &p) = 0;
    virtual QString methodDll() = 0;
    virtual int panelType() = 0;
    virtual QString methodName() = 0;
    virtual int dataType() = 0;
    //virtual QStringList settings(DfdFileDescriptor *dfd, int bandStrip) = 0;
    virtual Parameters parameters() = 0;
    virtual DescriptionList processData(const Parameters &p) = 0;

    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p) = 0;
    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p) = 0;
    virtual DfdChannel *createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd,
                       const QVector<double> &spectrum, Parameters &p, int i) = 0;
    virtual Function * addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i) = 0;
private:
    QList<DfdFileDescriptor *> dataBase;
};

struct Parameters
{
    double sampleRate; // частота дискретизации
    int averagingType; //0 - линейное
                       //1 - экспоненциальное
                       //2 - хранение максимума
    int windowType; //0 - прямоугольное
                    //1 - Бартлетта / треугольное
                    //2 - Хеннинга
                    //3 - Хемминга
                    //4 - Натолл
                    //5 - Гаусс
    int bufferSize; // sampleRate / 2^i, где i - номер частотного диапазона от 0 до 11

    int bandWidth;
    int initialBandStripNumber;
    int bandStrip; //полоса обработки, 0 = исх. частота дискрет.
                  //                  1 = fd/2
                  //                  n = fd/2^n
    double overlap; // перекрытие, 0 - 0.75

    int scaleType; //0 - линейная
                   //1 - экспоненциальная
    double threshold; // порог для спектра в дБ

    int averagesCount; // число усреднений

    int baseChannel; // опорный канал
    int activeChannel; // активный канал

    QVector<double> window; // коэффициенты оконной функции

    int fCount; // число отсчетов в частотной области

    bool useDeepSea;

    int panelType; //тип панели для обработки с помощью DeepSea
    QString methodName;
    QString methodDll;
    int dataType;
    bool saveAsComplex=false; //формат результата: комплексные числа

    int spectreType; // тип спектра
                     // 0 = мощности
                     // 1 = плотности мощности
                     // 2 = спектр СКЗ

    AbstractMethod *method;

    QString windowDescription() const {
        switch (windowType) {
            case 0: return "Прямоуг.";
            case 1: return "Бартлетта";
            case 2: return "Хеннинга";
            case 3: return "Хемминга";
            case 4: return "Натолл";
            case 5: return "Гаусс";
        }
        return "";
    }

    QString averaging(int avgType) const {
        switch (avgType) {
            case 0: return "линейное";
            case 1: return "экспоненциальное";
            case 2: return "хранение максимума";
        }
        return "";
    }
};

static QDebug operator<<(QDebug debug, const Parameters &p)
  {
      QDebugStateSaver saver(debug);
      debug << "sampleRate"<< p.sampleRate<<endl;
      debug <<  "averagingType"<< p.averagingType<<endl;
      debug <<  "windowType"<< p.windowType<<endl;
      debug <<  "bufferSize"<< p.bufferSize<<endl;
      debug <<  "bandWidth"<< p.bandWidth<<endl;
      debug <<  "initialBandStripNumber"<< p.initialBandStripNumber<<endl;
      debug <<  "bandStrip"<< p.bandStrip<<endl;
      debug <<  "overlap"<< p.overlap<<endl;
      debug <<  "scaleType"<< p.scaleType<<endl;
      debug <<  "threshold"<< p.threshold<<endl;
      debug << "averagesCount"<< p.averagesCount<<endl;
      debug <<"baseChannel"<<   p.baseChannel<<endl;
      debug <<"activeChannel"<<   p.activeChannel<<endl;
      debug <<"window"<<   p.window<<endl;
      debug <<"fCount"<<   p.fCount<<endl;
      debug <<"useDeepSea"<<   p.useDeepSea<<endl;
      debug <<"panelType"<<   p.panelType<<endl;
      debug <<"methodName"<<   p.methodName<<endl;
      debug <<"methodDll"<<   p.methodDll<<endl;
      debug << "dataType"<< p.dataType<<endl;
      debug << "saveAsComplex"<< p.saveAsComplex<<endl;
        return debug;
  }

#endif // ABSTRACTMETHOD_H

