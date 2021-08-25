#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>
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


class Plot : public QwtPlot
{
    Q_OBJECT
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



    void update();

    /**
     * @brief hasCurves
     * @return whether plot has any curves
     */
    bool hasCurves() const;
    /**
     * @brief curvesCount
     * @return count of curves excluding freeCurve
     */
    int curvesCount() const {return curves.size();}
    int curvesCount(int type) const;

    /**
     * @brief deleteCurves deletes all curves on a plot from a descriptor
     * @param descriptor
     */
    void deleteCurvesForDescriptor(FileDescriptor *descriptor);

    /**
     * @brief deleteCurve deletes plotted curve
     * @param dfd - DFD represented by a curve
     * @param channel
     */
    void deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteCurve(Curve *curve, bool doReplot = true);

    bool plotCurve(Channel * ch, QColor *col, bool &plotOnRight, int fileNumber);

//    Curve *plotted(FileDescriptor *dfd, int channel) const;
    Curve *plotted(Channel *channel) const;

    Range xRange() const;
    Range yLeftRange() const;
    Range yRightRange() const;

    /**
     * @brief switchLabelsVisibility
     * hides / shows axis labels
     */
    void switchLabelsVisibility();

    bool canBePlottedOnLeftAxis(Channel *ch);
    bool canBePlottedOnRightAxis(Channel *ch);

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

    /**
     * @brief deleteAllCurves
     * deletes all curves on a plot
     */
    void deleteAllCurves(bool forceDeleteFixed = false);
signals:
    void curveChanged(Curve *curve);
    void curveDeleted(Channel *);
    void trackingPanelCloseRequested();
    void playerPanelCloseRequested();
    void saveTimeSegment(const QList<FileDescriptor*> &files, double from, double to);
    void curvesChanged();
    void curvesCountChanged();
    void updatePlotted();
    void needPlotChannels();
private slots:
    void editLegendItem(QwtPlotItem *item);
    void deleteCurveFromLegend(QwtPlotItem *item);
    void showContextMenu(const QPoint &pos, QwtAxisId axis);
    void fixCurve(QwtPlotItem* curve);
    void hoverAxis(QwtAxisId axis, int hover);
private:
    void importPlot(const QString &fileName, const QSize &size, int resolution);
    void importPlot(QPrinter &printer, const QSize &size, int resolution);
    bool hasDuplicateNames(const QString name) const;
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

    QwtPlotGrid *grid;
    PlotTracker *tracker;
    Picker *_picker;
    QwtPlotCanvas *_canvas;



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


    // QWidget interface
protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
};

#endif // PLOT_H
