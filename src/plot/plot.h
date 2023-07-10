#ifndef PLOT_H
#define PLOT_H

#include <math.h>

#include <QObject>

class Curve;
class Channel;
class FileDescriptor;
class ZoomStack;
class QAction;
class Picker;
class QPrinter;
class PlotModel;
class QMenu;
class Cursors;
class CursorBox;
class QCPPlot;
class PlayPanel;
class ColorSelector;

#include <QWidget>

#include "enums.h"
#include "selectable.h"


class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(Enums::PlotType type, QWidget *parent = 0);
    virtual ~Plot();

    /**
     * @brief model
     * @return модель, содержащая список построенных кривых
     */
    PlotModel *model() {return m;}
    /**
     * @brief type
     * @return тип графика (см. \a Enums::PlotType)
     */
    Enums::PlotType type() const {return plotType;}

    /**
     * @brief widget
     * @return виджет графика для размещения во вкладке графика
     */
    QWidget *widget() const;

    /**
     * @brief toolBarWidget
     * @return виджет с дополнительными кнопками для размещения на панели инструментов
     * во вкладке графика
     */
    virtual QWidget *toolBarWidget();

    /**
     * @brief impl
     * @return ссылка на виджет графика
     */
    QCPPlot *impl() const;

    /**
     * @brief screenToPlotCoordinates, plotToScreenCoordinates
     * @param axis ось координат
     * @param value значение на экране/графике
     * @return преобразование экранных координат в координаты на графике и обратно
     */
    double screenToPlotCoordinates(Enums::AxisType axis, double value) const;
    double plotToScreenCoordinates(Enums::AxisType axis, double value) const;

    /**
     * @brief plotRange
     * @param axis ось координат
     * @return отображаемый диапазон для оси axis
     */
    Range plotRange(Enums::AxisType axis) const;

    /**
     * @brief screenRange
     * @param axis ось координат
     * @return диапазон в пикселях для оси axis,
     */
    Range screenRange(Enums::AxisType axis) const;

    /**
     * @brief updateActions обновляет дополнительные кнопки на панели инструментов
     * во вкладке графика, \sa toolbarWidget
     * @param filesCount
     * @param channelsCount
     */
    virtual void updateActions(int filesCount, int channelsCount);

    /**
     * @brief sergeiMode переводит график в режим, позволяющий выбирать записи/каналы
     */
    bool sergeiMode = false;

    /**
     * @brief interactionMode задает режим взаимодействия с графиком:
     * ScalingInteraction - масштабирование графика
     * DataInteraction - передвижение точек на графике
     */
    Enums::InteractionMode interactionMode = Enums::InteractionMode::ScalingInteraction;

    /**
     * @brief curvesCount
     * @param type тип канала, для которого построена кривая (Descriptor::DataType)
     * @return количество кривых такого типа на графике
     * Если type=-1, то возвращается общее количество кривых на графике
     */
    int curvesCount(int type=-1) const;

    /**
     * @brief axisTitleText
     * @param id
     * @return текст/метка для данной оси
     */
    QString axisTitleText(Enums::AxisType id) const;

    /**
     * @brief canBePlottedOnLeftAxis, canBePlottedOnRightAxis
     * проверяют, можно ли построить канал \a ch на текущем графике на левой/правой оси
     * @param ch канал
     * @param message сообщение, которое надо вывести на экран при невозможности построить канал
     * @return true если канал можно построить
     */
    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message=nullptr) const;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message=nullptr) const;

    /**
     * @brief focusPlot вызывается из QCPPlot при перетаскивании каналов на график
     * для перевода фокуса на вкладку с графиком
     */
    void focusPlot();

    /**
     * @brief addSelectable добавляет выделяемый элемент (курсор, метку, кривую) в список
     * отслеживаемых выделяемых элементов
     * @param item выделяемый элемент
     */
    void addSelectable(Selectable *item);
    /**
     * @brief removeSelectable удаляет выделяемый элемент из списка отслеживаемых
     * @param item
     */
    void removeSelectable(Selectable *item);

    /**
     * @brief deselect убирает выделение со всех объектов на графике
     */
    void deselect();
    /**
     * @brief getSelectables
     * @return список выделяемых элементов (курсоры, метки, кривые)
     */
    QList<Selectable*> getSelectables() {return selectables;}
    /**
     * @brief findSelected ищет выделенный объект под курсором
     * @param pos позиция курсора
     * @return  выделенный объект + доп.информация
     */
    Selected findSelected(QPoint pos) const;

    //default implementation returns pos as QString,
    //reimplemented in spectrograms to add Z coordinate
    /**
     * @brief pointCoordinates координаты точки в текстовом формате для курсора мыши
     * @param pos координаты точки (в координатах осей графика)
     * @return x,y (для обычных кривых) , x,y,z (для сонограмм)
     */
    virtual QString pointCoordinates(const QPointF &pos);

    /**
     * @brief tickDistance возвращает расстояние между двумя смежными метками на оси
     * @param axisType тип оси - нижняя, левая, правая, цветовая
     * @return 0 если ось неравномерная, не 0 если ось линейная
     */
    double tickDistance(Enums::AxisType axisType) const;

    /**
     * @brief deleteCurve удаляет кривую с графика
     * @param curve кривая
     * @param doReplot задает немедленную перерисовку графика
     */
    virtual void deleteCurve(Curve *curve, bool doReplot = true);
    //default implementation updates labels according to curves count on left and on right
    //reimplemented in spectrograms
    virtual void updateAxesLabels();
    //reimplemented in plot types
    virtual void plotChannel(Channel * ch, bool plotOnLeft, int fileIndex=0);

    virtual void onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels);

    /**
     * @brief update обновляет оси, метки, легенду и перерисовывает график
     */
    void update();

    /**
     * @brief plotCurvesForDescriptor меняет кривые одной записи на кривые другой записи
     * при переходе между записями
     * @param d новая запись
     * @param fileIndex индекс новой записи в таблице записей
     */
    void plotCurvesForDescriptor(FileDescriptor *d, int fileIndex=0);

    /**
     * @brief cycleChannels сдвигает построенные каналы вверх/вниз, соответственно удаляя
     * кривые из начала/конца списка построенных каналов и добавляя в конец/начало
     * @param up флаг сдвига вверх/вниз
     */
    void cycleChannels(bool up);

    /**
     * @brief deleteCurveFromLegend удаляет не фиксированную кривую по запросу из легенды
     * @param curve
     */
    void deleteCurveFromLegend(Curve *curve);

    /**
     * @brief deleteCurvesForDescriptor удаляет кривые, построенные для записи \a descriptor
     * при закрытии вкладки или удалении записи
     * @param descriptor
     */
    void deleteCurvesForDescriptor(FileDescriptor *descriptor);

    /**
     * @brief deleteCurveForChannelIndex удаляет кривую по записи и индексу канала.
     * Используется при удалении каналов из записи или удалении кривой с нажатой Ctrl
     * @param descriptor запись
     * @param channel номер канала
     * @param doReplot
     */
    void deleteCurveForChannelIndex(FileDescriptor *descriptor, int channel, bool doReplot = true);

    /**
     * @brief deleteSelectedCurve удаляет выделенную кривую по запросу от неё самой.
     * (может быть небезопасно)
     * @param selected выделенный объект
     */
    void deleteSelectedCurve(Selectable *selected);

    /**
     * @brief switchLabelsVisibility переключает отображение меток на осях
     */
    void switchLabelsVisibility();

    /**
     * @brief setAxis задает название оси (только запоминает, настоящее задание названия
     * происходит в update() )
     * @param axis
     * @param name
     */
    void setAxis(Enums::AxisType axis, const QString &name);

    /**
     * @brief setScale задает диапазон для оси
     * @param id
     * @param min
     * @param max
     * @param step
     */
    void setScale(Enums::AxisType id, double min, double max, double step = 0);

    /**
     * @brief removeLabels удаляет все метки с графика
     */
    void removeLabels();

    /**
     * @brief switchInteractionMode переключает режим взаимодействия с графиком:
     * - data interaction / scale interaction
     */
    void switchInteractionMode();

    /**
     * @brief switchCursorBox показывает/прячет окно курсоров
     */
    void switchCursorBox();


    void toggleAutoscale(Enums::AxisType axis, bool toggled);
    void autoscale(Enums::AxisType axis = Enums::AxisType::atInvalid);
    void recalculateScale(Enums::AxisType axis);

    void saveSpectrum(double zVal);
    void saveThroughput(double xVal);

    void updateLabels();
    void removeCursor(Selectable *selected);

    /**
     * @brief showContextMenu показывает контекстное меню над осью axis
     * @param pos
     * @param axis
     */
    void showContextMenu(const QPoint &pos, Enums::AxisType axis);

    /**
     * @brief editLegendItem открывает диалог параметров кривой
     * @param curve
     */
    void editLegendItem(Curve *curve);

    ZoomStack *zoom = nullptr;
    Picker *picker = nullptr;
    QWidget *legend;
protected:
    QCPPlot *m_plot = nullptr;

    PlotModel *m = nullptr;

    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible = true;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    ColorSelector *colors = nullptr;
    Cursors *cursors = nullptr;
    CursorBox *cursorBox;

    QList<Selectable*> selectables;

    //default implementation updates either left or right axis
    //reimplemented in Spectrogram
    virtual void updateBounds();
    virtual QMenu *createMenu(Enums::AxisType axis, const QPoint &pos);

    QString yValuesPresentationSuffix(int yValuesPresentation) const;
    void updatePlottedIndexes();
public slots:
    void replot();
    void savePlot();
    void copyToClipboard(bool useDialog);
    void print();
    void updateLegends();

    void moveCurve(Curve *curve, Enums::AxisType axis);
    void deleteAllCurves(bool forceDeleteFixed = false);
signals:
    //испускаем, когда удаляем или добавляем графики
    void curvesCountChanged();//->MainWindow.updateActions
    void focusThisPlot();
    //испускаем, когда добавляем график
    void channelPlotted(Channel *ch); //<- MainWindow::addPlotArea
    //испускаем, когда удаляем кривую
    void curveDeleted(Channel *);

    void trackingPanelCloseRequested();
    void saveTimeSegment(const QVector<FileDescriptor*> &files, double from, double to);
    void saveHorizontalSlice(const QVector<double>& zVal);
    void saveVerticalSlice(const QVector<double>& xVal);
    //испускаем, когда бросаем каналы на график
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
private slots:

private:
    QColor getNextColor();
    Enums::PlotType plotType = Enums::PlotType::General;
    PlayPanel *playerPanel = nullptr;
    int colorMap = 0;
};

#endif // PLOT_H
