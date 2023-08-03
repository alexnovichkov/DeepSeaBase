#ifndef QCPPLOT_H
#define QCPPLOT_H

#include "qcustomplot.h"
#include "enums.h"
#include "imagerenderdialog.h"
#include "enums.h"
#include "selectable.h"

class Plot;
class CanvasEventFilter;
class QCPCheckableLegend;
class MouseCoordinates;
class QCPAxisOverlay;
class QCPInfoOverlay;
class Curve;
class Channel;
class Selected;
class Selectable;
class QCPFlowLegend;
class SecondaryPlot;
class Cursor;
class PlotModel;
class ColorSelector;
class Cursors;
class CursorBox;
class ZoomStack;
class Picker;
class PlayPanel;
class FileDescriptor;

QCPAxis::AxisType toQcpAxis(Enums::AxisType type);
Enums::AxisType fromQcpAxis(QCPAxis::AxisType);

class QCPPlot : public QCustomPlot
{
    friend  class MouseCoordinates;
    Q_OBJECT
public:
    QCPPlot(Enums::PlotType type, QWidget *parent = nullptr);
    ~QCPPlot();

    /**
     * @brief toolBarWidget
     * @return виджет с дополнительными кнопками для размещения на панели инструментов
     * во вкладке графика
     */
    virtual QWidget *toolBarWidget();

    /**
     * @brief legend
     * @return виджет легенды для размещения в plotArea
     */
    QWidget *legendWidget();

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
     * @brief axisTitle
     * @param id
     * @return текст/метка для данной оси
     */
    QString axisTitle(Enums::AxisType id) const;

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
    void deleteCurvesForDescriptor(FileDescriptor *descriptor, QVector<int> indexes);

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

    void startZoom(QMouseEvent *event);
    void proceedZoom(QMouseEvent *event);
    void endZoom(QMouseEvent *event);
    void cancelZoom();

    void addCursorToSecondaryPlots(Cursor *cursor);
    void removeCursorFromSecondaryPlots(Cursor *cursor);
    void updateSecondaryPlots();
    void setCurrentCurve(Curve *curve);

    QCPAxis *axis(Enums::AxisType axis) const;
    Enums::AxisType axisType(QCPAxis *axis) const;

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
     * @brief setAxis задает название оси (только запоминает, настоящее задание названия
     * происходит в update() )
     * @param axis
     * @param name
     */
    void setAxis(Enums::AxisType axis, const QString &name);

    /**
     * @brief sergeiMode переводит график в режим, позволяющий выбирать записи/каналы
     */
    bool sergeiMode = false;

    //reimplemented in plot types
    virtual void plotChannel(Channel * ch, bool plotOnLeft, int fileIndex=0);

private:
    void addZoom();
    void axisDoubleClicked(QCPAxis *axis);
    void setSecondaryPlotsVisible(bool visible);
    QColor getNextColor();

    QCPCheckableLegend *checkableLegend = nullptr;
    ZoomStack *zoom = nullptr;
    Picker *picker = nullptr;
    Enums::PlotType plotType = Enums::PlotType::General;
    CanvasEventFilter *canvasFilter = nullptr;

    QSharedPointer<QCPAxisTicker> linTicker;
    QSharedPointer<QCPAxisTickerLog> logTicker;
    QSharedPointer<QCPAxisTicker> octaveTicker;

    QMap<Enums::AxisType, AxisParameters> axisParameters;

    QCursor oldCursor;
    MouseCoordinates *mouseCoordinates;
    QCPInfoOverlay *infoOverlay = nullptr;
    QCPAxisOverlay *leftOverlay = nullptr;
    QCPAxisOverlay *rightOverlay = nullptr;
    QCPColorScale *colorScale = nullptr;

    SecondaryPlot *spectrePlot = nullptr;
    SecondaryPlot *throughPlot = nullptr;
    QCPLayoutGrid *subLayout = nullptr;

    PlayPanel *playerPanel = nullptr;
    int colorMap = 0;

    // PlotInterface interface
public:
    void setEventFilter(CanvasEventFilter *filter);
    QCPAxis *eventTargetAxis(QEvent *event);
    void createLegend();
    void replot();
    void updateLegend();
    QPoint localCursorPosition(const QPoint &globalCursorPosition) const;
    void setAxisScale(Enums::AxisType axisType, Enums::AxisScale scale);
    Enums::AxisScale axisScale(Enums::AxisType axisType) const;
    void setTimeAxisScale(Enums::AxisType axisType, int scale);

    void setAxisRange(Enums::AxisType axis, double min, double max);
    void setInfoVisible(bool visible);
    void enableAxis(Enums::AxisType axis, bool enable);
    bool axisEnabled(Enums::AxisType axis);
    void setAxisTitle(Enums::AxisType axis, const QString &title);
    void enableColorBar(Enums::AxisType axis, bool enable);
    void setColorMap(int colorMap, Curve *curve);
    void setColorBarTitle(const QString &title);
    void importPlot(ImageRenderDialog::PlotRenderOptions options);
    void importPlot(QPrinter &printer, ImageRenderDialog::PlotRenderOptions options);
    Curve *createCurve(Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis);
    bool isCurve(Selectable* item) const;
protected:
    PlotModel *m = nullptr;
    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible = true;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    QScopedPointer<ColorSelector> colors;
    Cursors *cursors = nullptr;
    CursorBox *cursorBox = nullptr;

    QList<Selectable*> selectables;

    //default implementation updates either left or right axis
    //reimplemented in Spectrogram
    virtual void updateBounds();
    virtual QMenu *createMenu(Enums::AxisType axis, const QPoint &pos);

    QString yValuesPresentationSuffix(int yValuesPresentation) const;
    void updatePlottedIndexes();

//    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;

public slots:
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
    //обрабатывает двойной щелчок по канве, в зависимости от опций "canvasDoubleClick" и "canvasDoubleClickCursor"
    //либо создает новый курсор, либо передвигает курсор плеера
    void canvasDoubleClicked(const QPoint &position);
};

#endif // QCPPLOT_H
