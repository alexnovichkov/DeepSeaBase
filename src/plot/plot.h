#ifndef PLOT_H
#define PLOT_H

#include <math.h>
#include "colorselector.h"

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
class Selectable;
class QCPPlot;
class PlayPanel;

#include <QWidget>

#include "enums.h"



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

    Enums::InteractionMode interactionMode = Enums::InteractionMode::ScalingInteraction;

    /**
     * @brief curvesCount
     * @param type тип канала, для которого построена кривая (Descriptor::DataType)
     * @return количество кривых такого типа на графике
     */
    int curvesCount(int type=-1) const;

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
     * @brief getSelectables
     * @return список выделяемых элементов (курсоры, метки, кривые)
     */
    QList<Selectable*> getSelectables() {return selectables;}

    //default implementation returns pos as QString,
    //reimplemented in spectrograms to add Z coordinate
    /**
     * @brief pointCoordinates координаты точки в текстовом формате для курсора мыши
     * @param pos координаты точки (в координатах осей графика)
     * @return x,y (для обычных кривых) , x,y,z (для сонограмм)
     */
    virtual QString pointCoordinates(const QPointF &pos);

    virtual void deleteCurve(Curve *curve, bool doReplot = true);
    //default implementation updates labels according to curves count on left and on right
    //reimplemented in spectrograms
    virtual void updateAxesLabels();
    //reimplemented in plot types
    virtual void plotChannel(Channel * ch, bool plotOnLeft, int fileIndex=0);

    virtual void onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels);
    void update();

    void updatePlottedIndexes();
    void plotCurvesForDescriptor(FileDescriptor *d, int fileIndex=0);

    void cycleChannels(bool up);

    void deleteCurveFromLegend(Curve *curve);
    void deleteCurvesForDescriptor(FileDescriptor *descriptor);
    void deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteSelectedCurve(Selectable *selected);

    void switchLabelsVisibility();
    void setAxis(Enums::AxisType axis, const QString &name);

    void setScale(Enums::AxisType id, double min, double max, double step = 0);
    void removeLabels();
    void setInteractionMode(Enums::InteractionMode mode);
    void switchInteractionMode();
    void switchTrackingCursor();
    void toggleAutoscale(Enums::AxisType axis, bool toggled);
    void autoscale(Enums::AxisType axis = Enums::AxisType::atInvalid);
    void recalculateScale(Enums::AxisType axis);

    void saveSpectrum(double zVal);
    void saveThroughput(double xVal);

    void updateLabels();
    void removeCursor(Selectable *selected);
    void showContextMenu(const QPoint &pos, Enums::AxisType axis);
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
public slots:
    void replot();
    void savePlot();
    void switchCursor();
    void copyToClipboard();
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
    void saveHorizontalSlice(double zVal);
    void saveVerticalSlice(double xVal);
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
