#ifndef ABSTRACTMETHOD_H
#define ABSTRACTMETHOD_H

#include <QStringList>

class DfdFileDescriptor;

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
    int blockSize; // от 512 до 8192
    int bandStrip; //полоса обработки, 0 = исх. частота дискрет.
                  //                  1 = fd/2
                  //                  n = fd/2^n
    double overlap; // перекрытие, 0 - 0.75

    int scaleType; //0 - линейная
                   //1 - экспоненциальная
    double threshold; // порог для спектра в дБ

    int averagesCount; // число усреднений

    int baseChannel; // опорный канал

    QVector<double> window; // коэффициенты оконной функции

    int fCount; // число отсчетов в частотной области

    //double *x; //
    //quint32 xSize;
};

class AbstractMethod
{
public:
    /* индекс в methodCombo */
    virtual int id() = 0;
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip) = 0;
    virtual QString methodDll() = 0;
    virtual int panelType() = 0;
    virtual QString methodName() = 0;
    virtual int dataType() = 0;
    virtual QStringList settings(DfdFileDescriptor *dfd, int bandStrip) = 0;
    virtual Parameters parameters(DfdFileDescriptor *dfd) = 0;
};

#endif // ABSTRACTMETHOD_H
