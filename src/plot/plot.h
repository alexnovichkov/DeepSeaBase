#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>
#include "colorselector.h"

class QwtLegend;
class QwtPlotGrid;
class QwtPlotCanvas;
class QwtPlotOpenGLCanvas;
class Curve;
class Channel;
class FileDescriptor;

class ZoomStack;
class DragZoom;
class WheelZoom;
class AxisZoom;
class PlotZoom;
class QwtPlotZoomer;
class PlotTracker;
class QAction;
class QwtScaleEngine;
class QwtPlotMarker;
class Picker;
class CanvasEventFilter;
class AxisOverlay;
class PlotInfoOverlay;
class QPushButton;
class QCheckBox;
class QLabel;
class QDoubleSpinBox;
class TrackingPanel;
class QPrinter;
class CheckableLegend;
class Grid;
class PlotModel;
class QMenu;
class ChannelsMimeData;
class Cursors;

struct Range {
    void clear() {min = INFINITY; max = -INFINITY;}
    double min;
    double max;
};



#include <QWidget>



class Plot : public QwtPlot
{
    Q_OBJECT
public:
    enum class PlotType
    {
        Time,
        General,
        Octave,
        Spectrogram
    };

    enum InteractionMode {
        NoInteraction,
        ScalingInteraction,
        DataInteraction,
        LabelInteraction
    };

    explicit Plot(PlotType type, QWidget *parent = 0);
    virtual ~Plot();

    PlotModel *model() {return m;}
    PlotType type() const {return plotType;}

    virtual QAction *playAct() {return nullptr;}

    QwtAxisId xBottomAxis{QwtAxis::XBottom};
    QwtAxisId yLeftAxis{QwtAxis::YLeft};
    QwtAxisId yRightAxis{QwtAxis::YRight};

    bool sergeiMode = false;
    bool xScaleIsLogarithmic = false; //false = linear, true = logarithmic

    InteractionMode interactionMode = ScalingInteraction;

    bool hasCurves() const;
    int curvesCount(int type=-1) const;
    Range xRange() const;
    Range yLeftRange() const;
    Range yRightRange() const;
    QString axisTitleText(QwtAxisId id) const;

    //default implementation returns pos as QwtText,
    //reimplemented in spectrograms to add Z coordinate
    virtual QString pointCoordinates(const QPointF &pos);

    //default implementation returns LineCurve
    //reimplemented in other plot types
    virtual Curve * createCurve(const QString &legendName, Channel *channel);
    virtual void deleteCurve(Curve *curve, bool doReplot = true);
    //default implementation updates labels according to curves count on left and on right
    //reimplemented in spectrograms
    virtual void updateAxesLabels();
    //reimplemented in plot types
    virtual void plotChannel(Channel * ch, bool plotOnLeft, int fileIndex=0);

    virtual void onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels);
    void update();

    void updatePlottedIndexes();
    void updateCycled();
    void plotCurvesForDescriptor(FileDescriptor *d, int fileIndex=0);

    void cycleChannels(bool up);

    void updateTrackingPanel();

    void deleteCurvesForDescriptor(FileDescriptor *descriptor);
    void deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot = true);

    void switchLabelsVisibility();
    void prepareAxis(QwtAxisId axis);
    void setAxis(QwtAxisId axis, const QString &name);

    void setScale(QwtAxisId id, double min, double max, double step = 0);
    void removeLabels();
    void setInteractionMode(InteractionMode mode);
    void switchInteractionMode();
    void switchTrackingCursor();
    void toggleAutoscale(int axis, bool toggled);
    void autoscale(int axis = -1);
    /**
     * @brief recalculateScale пересчитывает границы графиков,
     * отдельно для левой или правой вертикальной оси
     * @param leftAxis
     */
    void recalculateScale(bool leftAxis);
protected:
    PlotModel *m = nullptr;
    ZoomStack *zoom = nullptr;
    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible = true;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    Grid *grid = nullptr;
    PlotTracker *tracker = nullptr;
    Picker *_picker = nullptr;
    QwtPlotCanvas *_canvas = nullptr;

    AxisOverlay *leftOverlay = nullptr;
    AxisOverlay *rightOverlay = nullptr;

    DragZoom *dragZoom = nullptr;
    WheelZoom *wheelZoom = nullptr;
    AxisZoom *axisZoom = nullptr;
    PlotZoom *plotZoom = nullptr;
    CanvasEventFilter *canvasFilter = nullptr;

    TrackingPanel *trackingPanel = nullptr;

    ColorSelector *colors = nullptr;
    Cursors *cursors = nullptr;


    //default implementation updates either left or right axis
    //reimplemented in Spectrogram
    virtual void updateBounds();
    //default implementation does nothing
    //reimplemented in spectrogram
    virtual void setRightScale(QwtAxisId id, double min, double max);
    virtual QMenu *createMenu(QwtAxisId axis, const QPoint &pos);
    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message=nullptr) const;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message=nullptr) const;

    void setInfoVisible(bool visible);
    QString yValuesPresentationSuffix(int yValuesPresentation) const;
public slots:
    void savePlot();
    void switchCursor();
    void copyToClipboard();
    void print();
    void updateLegends();

    void moveCurve(Curve *curve, int axis);
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
    //испускаем, когда бросаем каналы на график
    void needPlotChannels(bool plotOnLeft, const QVector<Channel*> &channels);
private slots:
    void editLegendItem(QwtPlotItem *item);
    void deleteCurveFromLegend(QwtPlotItem *item);
    void showContextMenu(const QPoint &pos, QwtAxisId axis);
    void fixCurve(QwtPlotItem* curve);
    void hoverAxis(QwtAxisId axis, int hover);
private:
    QColor getNextColor();

    void importPlot(const QString &fileName, const QSize &size, int resolution);
    void importPlot(QPrinter &printer, const QSize &size, int resolution);
//    void checkDuplicates(const QString name);
    void createLegend();

    PlotInfoOverlay *infoOverlay = nullptr;
    PlotType plotType = PlotType::General;
    // QWidget interface
public:
    void dropEvent(QDropEvent *event) override;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

#endif // PLOT_H
