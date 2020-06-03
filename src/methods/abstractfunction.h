#ifndef ABSTRACTFUNCTION_H
#define ABSTRACTFUNCTION_H

#include <QObject>
#include <QDateTime>

class FileDescriptor;

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
 * ?/weightingType - тип взвешивания, 0=no, 1=A wei, 2=B wei, 3=C wei, 4=D wei
 * ?/amplitudeScaling - 0=unknown, 1=half-peak, 2=peak, 3=RMS
 * ?/normalization - тип нормализации, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                               //3=Units squared seconds per Hz (ESD)
 * ?/threshold - logref
 * ?/functionDescription - функция, строка, отдает та функция, которая позже всех в цепи
 * ?/functionType - функция, число, отдает та функция, которая позже всех в цепи
 * ?/averaging - описание усреднения, отдает AveragingFunction
 * ?/averagingType - тип усреднения, отдает AveragingFunction
 * ?/processData - dfd- Process
 * ?/dataType - dfd - DataType
 * ?/dataFormat - complex / real / amplitude / amplitudeDb / imaginary / phase
 * ?/abscissaEven - true/false
 * ?/abscissaData - вектор, отдается пустой вектор, если шаг по абсциссе постоянный
 *                  только функция OctaveFunction будет отдавать непустой вектор
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
    explicit AbstractFunction(QObject *parent = nullptr);
    virtual ~AbstractFunction() {}
    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
    QString propertiesDescription() const;
    virtual QStringList properties() const = 0;
    virtual QString propertyDescription(const QString &property) const = 0;
    virtual bool propertyShowsFor(const QString &property) const;

    virtual QVariant getProperty(const QString &property) const = 0;
    virtual void setProperty(const QString &property, const QVariant &val) = 0;

    virtual QVector<double> getData(const QString &id) = 0;

    void setInput(AbstractFunction *input) {m_input = input;}
    void setFile(FileDescriptor *file);

    // по умолчанию не делает ничего
    virtual bool compute(FileDescriptor *file) = 0;

    // очищает внутреннее состояние функции, но не меняет параметры, заданные ранее
    virtual void reset();
signals:
    void propertyChanged(const QString &property, const QVariant &val);
    void attributeChanged(const QString &property, const QVariant &val, const QString &attribute);

    void tick();
    void tick(const QString &path);
    void message(const QString &s);
public slots:
    virtual void updateProperty(const QString &property, const QVariant &val);
protected:
//    QList<FileDescriptor *> m_dataBase;
    AbstractFunction *m_input;
    FileDescriptor *m_file = 0;
};

class AbstractAlgorithm : public QObject
{
    Q_OBJECT
public:
    explicit AbstractAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);
    virtual ~AbstractAlgorithm() {}
    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
//    virtual QString propertiesDescription() const;
//    virtual QStringList properties() const = 0;
//    virtual QString propertyDescription(const QString &property) const = 0;
    bool propertyShowsFor(const QString &property) const;

    QVariant getProperty(const QString &property) const;
    void setProperty(const QString &property, const QVariant &val);

//    virtual QVector<double> getData(const QString &id) = 0;

    QList<FileDescriptor *> dataBase() const {return m_dataBase;}

    QList<AbstractFunction *> functions() const {return m_functions;}

    QStringList getNewFiles() const {return newFiles;}

    // по умолчанию не делает ничего
    virtual bool compute(FileDescriptor *file) = 0;

    // очищает внутреннее состояние функции, но не меняет параметры, заданные ранее
    virtual void reset();
signals:
    void propertyChanged(const QString &property, const QVariant &val);
    void attributeChanged(const QString &property, const QVariant &val, const QString &attribute);

    void tick();
    void tick(const QString &path);
    void finished();
    void message(const QString &s);
public slots:
    virtual void start();
protected:
    void finalize();

    QList<FileDescriptor *> m_dataBase;
    QList<AbstractFunction *> m_functions;

    QStringList newFiles;
    QDateTime dt;
//    QString tempFolderName;
};

#endif // ABSTRACTFUNCTION_H
