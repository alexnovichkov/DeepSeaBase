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

    /**
     * @brief parameters
     * @return список параметров функции
     */
    virtual QStringList parameters() const = 0;
    /**
     * @brief parametersDescription
     * @return описание параметров функции в формате JSON
     */
    QString parametersDescription() const;
    /**
     * @brief shouldParameterBeVisible
     * @param parameter название параметра
     * @return true если при данных сочетаниях параметров функции parameter должен быть виден
     */
    bool shouldParameterBeVisible(const QString &parameter) const;
    /**
     * @brief getParameter возвращает текущее значение параметра с учетом сопряженности функции
     *        (некоторые взаимные характеристики содержат только один комплект параметров)
     * @param parameter название параметра
     * @return значение параметра
     */
    QVariant getParameter(const QString &parameter) const;
    /**
     * @brief setParameter задает текущее значение параметра с учетом сопряженности функции
     *        (некоторые взаимные характеристики содержат только один комплект параметров)
     * @param parameter название параметра
     * @param val новое значение параметра
     */
    void setParameter(const QString &parameter, const QVariant &val);
    /**
     * @brief updateParameter слот, позволяющий связать изменение параметров одной функции и
     *        обновление параметров другой функции
     * @param parameter название параметра
     * @param val новое значение параметра
     * По умолчанию не делает ничего, переопределена только в ChannelFunction, frameCutterFunction, octaveFunction
     */
    virtual void updateParameter(const QString &parameter, const QVariant &val);
    /**
     * @brief pairWith позволяет связать две функции, используемые при расчете взаимной характеристики
     * @param slave функция, возвращающая не свои свойства, а свойства ведущей функции
     */
    void pairWith(AbstractFunction *slave);
    /**
     * @brief paired
     * @return true если функция связана с другой, то есть получает от неё значения параметров
     */
    bool paired() const {return m_master != nullptr;}
    /**
     * @brief getData возвращает результат расчета функции
     * @param id позволяет указать назначение данных. Пока что поддерживается input и triggerInput
     * @return данные, возможно, в переплетенном виде, если данные комплексные (re,im,re,im...)
     */
    QVector<double> getData(const QString &id);
    /**
     * @brief getFunctionDescription получает описание параметров расчета от предыдущей функции
     *        (в цепочке выполнения), а затем добавляет/перезаписывает специфичные параметры
     * @return QVarianMap со значениями параметров
     */
    virtual DataDescription getFunctionDescription() const;
    /**
     * @brief setInput задает предыдущее звено в цепи расчета
     * @param input предыдущее звено
     */
    void setInput(AbstractFunction *input);
    /**
     * @brief setInput2 задает предыдущее звено в цепи расчета для функций, принимающих данные
     *        из двух источников (взаимные характеристики)
     * @param input
     */
    void setInput2(AbstractFunction *input);
    /**
     * @brief setFile задает начальный файл и распространяет его для всей цепи расчета
     * @param file начальный файл, используется для первичной инициализации параметров алгоритма
     */
    void setFile(FileDescriptor *file);
    /**
     * @brief compute запускает расчет для данной функции
     * @param file текущий рассчитываемый файл
     * @return true если расчет окончился успешно
     */
    virtual bool compute(FileDescriptor *file) = 0;
    /**
     * @brief reset очищает внутреннее состояние функции, но не меняет параметры, заданные ранее
     */
    virtual void reset();
    /**
     * @brief resetData сбрасывает позицию в данных для расчета на начало
     */
    virtual void resetData();
signals:
    void parameterChanged(const QString &parameter, const QVariant &val);
    void attributeChanged(AbstractFunction *f, const QString &parameter, const QVariant &val, const QString &attribute);

    void tick(const QString &path);
    void message(const QString &s);
//public slots:
//    void updateParameter(const QString &parameter, const QVariant &val);
protected:
    virtual QString m_parameterDescription(const QString &parameter) const = 0;
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

    QVariant getParameter(AbstractFunction *f, const QString &parameter) const;
    void setParameter(AbstractFunction *f, const QString &parameter, const QVariant &val);

    void saveSettings();
    void restoreSettings();

    QList<FileDescriptor *> dataBase() const {return m_dataBase;}

    QList<AbstractFunction *> functions() const {return m_functions;}

    QStringList getNewFiles() const {return newFiles;}

    bool compute(FileDescriptor *file);
signals:
    void parameterChanged(const QString &parameter, const QVariant &val);

    void tick();
    void finished();
    void message(const QString &s);
public slots:
    void start();
protected:
    void finalize();
    virtual void resetChain() = 0;
    virtual void initChain(FileDescriptor *file) = 0;

    //override this method if algorithm is applicable to channel types
    //other than TimeResponse
    virtual bool applicableTo(Descriptor::DataType channelType);

    QList<FileDescriptor *> m_dataBase;
    /**
     * @brief m_functions включает те функции, которые нужно отобразить в окне расчета,
     * то есть те, которые имеют настраиваемые пользователем параметры
     */
    QList<AbstractFunction *> m_functions;
    /**
     * @brief m_chain включает первую и последнюю функции цепочки вычислений.
     * Порядок вычислений настраивается с помощью методов
     * AbstractFunction setInput() и setInput2().
     */
    QList<AbstractFunction *> m_chain;

    QStringList newFiles;
};

#endif // ABSTRACTFUNCTION_H
