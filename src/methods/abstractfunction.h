#ifndef ABSTRACTFUNCTION_H
#define ABSTRACTFUNCTION_H

#include <QObject>
#include <QDateTime>

#include "fileformats/filedescriptor.h"

/***
 * Описание глобальных параметров
 * ?/channels - список обрабатываемых каналов (для ProcChansList) 1,2,3,7,8,9
 *              отдает ChannelFunction
 * ?/blockSize - число отсчетов в порции данных для канала
 *               равно числу отсчетов в канале, если канал не разбит на блоки
 *               меньше числа отсчетов, если канал разбит на блоки (при отсутствии усреднения, например)
 * ?/octaveFormat - октавный формат. 0 - не октавный, n - n-октавный, отдает только октавная функция
 * ?/windowDescription - оконная функция, строка, отдает WindowingFunction
 * ?/windowType        - оконная функция, число, отдает WindowingFunction
 * ?/weighting - тип взвешивания, no / A / B / C / D / unknown
 * ?/amplitudeScaling - 0=unknown, 1=half-peak, 2=peak, 3=RMS
 * ?/normalization - тип нормализации, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                               //3=Units squared seconds per Hz (ESD)
 * ?/logref - пороговое значение для перевода из U в dB
 * ?/functionDescription - функция, строка, отдает та функция, которая позже всех в цепи
 * ?/functionType - функция, число, отдает та функция, которая позже всех в цепи
 * ?/averaging - описание усреднения, отдает AveragingFunction
 * ?/averagingType - тип усреднения, отдает AveragingFunction
 * ?/processData - dfd- Process
 * ?/dataType - dfd - DataType
 * ?/dataFormat - complex / real / amplitude / amplitudeDb / imaginary / phase
 * ?/abscissaEven - true/false
 * ?/abscissaData - вектор, отдается пустой вектор, если шаг по абсциссе постоянный
 *                  непустой вектор будет отдавать только функция, возвращающая непостоянный шаг (OctaveFunction)
 * ?/xType - тип данных по оси X
 * ?/xName - единица измерения по оси Х
 * ?/xStep
 * ?/xBegin
 * ?/yType - тип данных по оси Y
 * ?/yName - единица измерения по оси Y
 * ?/zType - тип данных по оси Z
 * ?/zName - единица измерения по оси Z
 * ?/zStep
 * ?/zBegin
 * ?/zData - вектор, отдается пустой вектор, если шаг по оси z постоянный
 * ?/zCount - количество блоков
*/

class AbstractFunction : public QObject
{
    Q_OBJECT
public:
    explicit AbstractFunction(QObject *parent = nullptr, const QString &name=QString());
    virtual ~AbstractFunction();

    virtual QString name() const = 0;
    QString debugName() const {return _name;}
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
    QString parametersDescription() const;
    bool parameterShowsFor(const QString &parameter) const;

    virtual QString parameterDescription(const QString &parameter) const = 0;


    QVariant getParameter(const QString &parameter) const;
    void setParameter(const QString &parameter, const QVariant &val);
    virtual void updateParameter(const QString &parameter, const QVariant &val);

    void pairWith(AbstractFunction *slave);
    bool paired() const {return m_master != nullptr;}

    virtual QVector<double> getData(const QString &id);

    virtual DataDescription getFunctionDescription() const;

    void setInput(AbstractFunction *input);
    void setInput2(AbstractFunction *input);
    void setFile(FileDescriptor *file);

    // по умолчанию не делает ничего
    virtual bool compute(FileDescriptor *file) = 0;

    // очищает внутреннее состояние функции, но не меняет параметры, заданные ранее
    virtual void reset();
    // сбрасывает позицию в данных для расчета на начало
    virtual void resetData();
signals:
    void parameterChanged(const QString &parameter, const QVariant &val);
    void attributeChanged(AbstractFunction *f, const QString &parameter, const QVariant &val, const QString &attribute);

    void tick(const QString &path);
    void message(const QString &s);
//public slots:
//    void updateParameter(const QString &parameter, const QVariant &val);
protected:
    virtual QStringList parameters() const = 0;
    virtual QVariant m_getParameter(const QString &parameter) const = 0;
    virtual void m_setParameter(const QString &parameter, const QVariant &val) = 0;
    /**
     * @brief m_parameterShowsFor по умолчанию показываем все параметры
     * @param parameter - название параметра без ?/ или FRF/
     * @return true если параметр следует показать
     */
    virtual bool m_parameterShowsFor(const QString &parameter) const {Q_UNUSED(parameter); return true;}

//    QList<FileDescriptor *> m_dataBase;
    AbstractFunction *m_input = nullptr;
    AbstractFunction *m_input2 = nullptr;

    AbstractFunction *m_master = nullptr;
    AbstractFunction *m_slave = nullptr;
    FileDescriptor *m_file = nullptr;
    QString _name; //метка для отладки
    QVector<double> output;
    QVector<double> triggerData;
};

class AbstractAlgorithm : public QObject
{
    Q_OBJECT
public:
    explicit AbstractAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);
    virtual ~AbstractAlgorithm();
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
    bool parameterShowsFor(AbstractFunction *f, const QString &parameter) const;

    QVariant getParameter(AbstractFunction *f, const QString &parameter) const;
    void setParameter(AbstractFunction *f, const QString &parameter, const QVariant &val);

    void saveSettings();
    void restoreSettings();

    QList<FileDescriptor *> dataBase() const {return m_dataBase;}

    QList<AbstractFunction *> functions() const {return m_functions;}

    QStringList getNewFiles() const {return newFiles;}

    bool compute(FileDescriptor *file);

    // очищает внутреннее состояние алгоритма, но не меняет параметры, заданные ранее
    virtual void reset();
signals:
    void parameterChanged(const QString &parameter, const QVariant &val);

    void tick();
    void tick(const QString &path);
    void finished();
    void message(const QString &s);
public slots:
    virtual void start();
protected:
    void finalize();
    virtual void resetChain() = 0;
    virtual void initChain(FileDescriptor *file) = 0;

    //override this method if algorithm is applicable to channel types
    //other than TimeResponse
    virtual bool applicableTo(Descriptor::DataType channelType);

    QList<FileDescriptor *> m_dataBase;
    QList<AbstractFunction *> m_functions; //список функций для отображения в окне расчета
    QList<AbstractFunction *> m_chain; //начало и конец цепи вычислений
    QStringList newFiles;
};

#endif // ABSTRACTFUNCTION_H
