#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>
#include <QAudio>

class QwtLegend;
class QwtPlotGrid;
class QwtPlotCanvas;
class Curve;
class Channel;
class FileDescriptor;

class ChartZoom;
class QwtPlotZoomer;
class PlotPicker;
class QAction;
class QwtScaleEngine;
class QwtPlotMarker;

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

    QList<Curve *> graphs;

    void update();

    /**
     * @brief hasGraphs
     * @return whether plot has any graphs
     */
    bool hasGraphs() const;
    /**
     * @brief graphsCount
     * @return count of graphs excluding freeGraph
     */
    int graphsCount() const {return graphs.size();}

    /**
     * @brief deleteGraphs
     * deletes all graphs on a plot
     */
    void deleteGraphs();
    /**
     * @brief deleteGraphs deletes all graphs on a plot, which represent DFDd with dfdGuid
     * @param dfdGuid
     */
    void deleteGraphs(FileDescriptor *descriptor);

    /**
     * @brief deleteGraph deletes plotted graph
     * @param dfd - DFD represented by a graph
     * @param channel
     */
    void deleteGraph(FileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteGraph(Curve *graph, bool doReplot = true);
    void deleteGraph(Channel *c, bool doReplot = true);

    bool plotChannel(FileDescriptor *descriptor, int channel, QColor *col, bool plotOnRight, int fileNumber);

    Curve *plotted(FileDescriptor *dfd, int channel) const;
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

    void prepareAxis(int axis);
    void setAxis(int axis, const QString &name);
    void moveToAxis(int axis, double min, double max);
    void updateAxesLabels();

    void removeLabels();




    void setInteractionMode(InteractionMode mode);
    void switchInteractionMode();
    void switchTrackingCursor();
    void switchPlayerVisibility();
    void toggleAutoscale(int axis, bool toggled);
    void autoscale(int axis);
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

    void moveGraph(Curve *curve, int axis);
signals:
    void curveChanged(Curve *curve);
    void curveDeleted(FileDescriptor *descriptor, int index);
    void trackingPanelCloseRequested();
    void playerPanelCloseRequested();
    void saveTimeSegment(const QList<FileDescriptor*> &files, double from, double to);
    void graphsChanged();
private slots:
    void editLegendItem(const QVariant &itemInfo, int index);
    void editLegendItem(QwtPlotItem *item);
    void deleteGraph(QwtPlotItem *item);
    void showContextMenu(const QPoint &pos, const int axis);
    void moveGraph(QwtPlotItem *curve);
private:
    void importPlot(const QString &fileName);
    bool hasDuplicateNames(const QString name) const;
    void checkDuplicates(const QString name);
    QString yValuesPresentationSuffix(int yValuesPresentation) const;


//    void playChannel(Channel *ch);


    // axis labels
    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible;
    int yValuesPresentationLeft;
    int yValuesPresentationRight;

    QList<Curve *> leftGraphs;
    QList<Curve *> rightGraphs;

    QwtPlotGrid *grid;
    PlotPicker *picker;
    QwtPlotCanvas *canvas;

    bool xScale; //false = linear, true = logarithmic
    bool y1Scale;//false = linear, true = logarithmic
    bool y2Scale;//false = linear, true = logarithmic

    ChartZoom *zoom;

    TrackingPanel *trackingPanel;
    PlayPanel *playerPanel;

    InteractionMode interactionMode;
};

#endif // PLOT_H
