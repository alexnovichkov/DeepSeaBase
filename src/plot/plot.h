#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>
#include "colorselector.h"
//#include <QAudio>

class QwtLegend;
class QwtPlotGrid;
class QwtPlotCanvas;
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

struct Range {
    void clear() {min = INFINITY; max = -INFINITY;}
    double min;
    double max;
};



#include <QWidget>
class QTreeWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class QDoubleSpinBox;
class TrackingPanel;
class PlayPanel;
class QPrinter;
class CheckableLegend;
class Grid;

class Plot : public QwtPlot
{
    Q_OBJECT
    struct PlottedIndex
    {
        int index = -1;
        bool onLeft = true;
    };

public:
    enum InteractionMode {
        NoInteraction,
        ScalingInteraction,
        DataInteraction,
        LabelInteraction
    };

    explicit Plot(QWidget *parent = 0);
    virtual ~Plot();

    QList<Curve *> curves;
    QwtAxisId xBottomAxis{QwtAxis::xBottom,0};
    QwtAxisId yLeftAxis{QwtAxis::yLeft,0};
    QwtAxisId yRightAxis{QwtAxis::yRight,0};
    bool spectrogram = false;
    bool sergeiMode = false;

    //Этот список хранит индексы каналов, которые имеют графики, в том случае,
    //если все эти каналы - из одной записи. Список обновляется при добавлении
    //или удалении кривых
    QVector<PlottedIndex> plottedIndexes;
    QVector<Channel*> plottedChannels() const;
    QVector<FileDescriptor*> plottedDescriptors() const;
    bool hasCurves() const;
    int curvesCount(int type=-1) const;
    Curve *plotted(Channel *channel) const;
    Range xRange() const;
    Range yLeftRange() const;
    Range yRightRange() const;
    QString axisTitleText(QwtAxisId id) const;
    bool canBePlottedOnLeftAxis(Channel *ch) const;
    bool canBePlottedOnRightAxis(Channel *ch) const;



    void updatePlottedIndexes();
    void plotCurvesForDescriptor(FileDescriptor *d);
    void plotChannel(Channel * ch, bool plotOnLeft);
    void update();

    void deleteCurvesForDescriptor(FileDescriptor *descriptor);
    void deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteCurve(Curve *curve, bool doReplot = true);

    void switchLabelsVisibility();
    void prepareAxis(QwtAxisId axis);
    void setAxis(QwtAxisId axis, const QString &name);
    void updateAxesLabels();
    void setScale(QwtAxisId id, double min, double max, double step = 0);
    void removeLabels();
    void setInteractionMode(InteractionMode mode);
    void switchInteractionMode();
    void switchTrackingCursor();
    void switchPlayerVisibility();
    void toggleAutoscale(int axis, bool toggled);
    void autoscale(int axis = -1);
    /**
     * @brief recalculateScale пересчитывает границы графиков,
     * отдельно для левой или правой вертикальной оси
     * @param leftAxis
     */
    void recalculateScale(bool leftAxis);
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
    void playerPanelCloseRequested();
    void saveTimeSegment(const QList<FileDescriptor*> &files, double from, double to);
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
    void checkDuplicates(const QString name);
    QString yValuesPresentationSuffix(int yValuesPresentation) const;
    void createLegend();


//    void playChannel(Channel *ch);


    // axis labels
    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    int colorMap = 0;

    QList<Curve *> leftCurves;
    QList<Curve *> rightCurves;

    Grid *grid;
    PlotTracker *tracker;
    Picker *_picker;
    QwtPlotCanvas *_canvas;

    AxisOverlay *leftOverlay;
    AxisOverlay *rightOverlay;

    bool xScaleIsLogarithmic = false; //false = linear, true = logarithmic

    ZoomStack *zoom = nullptr;
    DragZoom *dragZoom = nullptr;
    WheelZoom *wheelZoom = nullptr;
    AxisZoom *axisZoom = nullptr;
    PlotZoom *plotZoom = nullptr;
    CanvasEventFilter *canvasFilter = nullptr;

    TrackingPanel *trackingPanel;
    PlayPanel *playerPanel;

    InteractionMode interactionMode = ScalingInteraction;
    ColorSelector *colors;

    // QWidget interface
protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

#endif // PLOT_H
