#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <qwt_plot_marker.h>
#include <math.h>


class QwtLegend;
class QwtPlotGrid;
class QwtPlotCanvas;
class Curve;
class Channel;
class FileDescriptor;

class QwtChartZoom;
class QwtPlotZoomer;
class PlotPicker;
class QAction;
class QwtScaleEngine;

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

class TrackingCursor : public QwtPlotMarker
{

};

class TrackingPanel:public QWidget
{
    Q_OBJECT
public:
    struct TrackInfo {
        QString name;
        QColor color;
        double xval, yval;
        //double xval2, yval2;
        double skz;
        double energy;
    };

    explicit TrackingPanel(QWidget *parent=0);
    void updateState(const QList<TrackInfo> &curves, bool second);
    void setX(double x, bool second);
    void setStep(double step);
protected:

signals:
    void switchHarmonics(bool on);
    void updateX(double value, bool second);
private:
    QTreeWidget *tree;
    QPushButton *button;
    QCheckBox *box;
    QCheckBox *box1;
    QCheckBox *harmonics;
    QLabel *x1Label;
    QLabel *x2Label;
    QDoubleSpinBox *x1Spin;
    QDoubleSpinBox *x2Spin;

    double mStep;
};

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

    QList<Curve *> curves() {return graphs;}

//    double minStep;
    QList<Curve *> graphs;

    void update();

    /**
     * @brief hasGraphs
     * @return whether plot has any graphs
     */
    bool hasGraphs() const;
    /**
     * @brief graphsCount
     * @return total count of graphs plotted
     */
    int totalGraphsCount() const;
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

    bool plotChannel(FileDescriptor *descriptor, int channel, QColor *col);

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

    /**
     * @brief updateLegends
     * updates legends with the dfd legend
     */
    void updateLegends();

    bool canBePlottedOnLeftAxis(Channel *ch);
    bool canBePlottedOnRightAxis(Channel *ch);

    void prepareAxis(int axis);
    void setAxis(int axis, const QString &name);
    void updateAxesLabels();

    void moveGraph(Curve *curve);

    void setInteractionMode(InteractionMode mode);
    void switchInteractionMode();
    void switchHarmonicsMode();
    void switchTrackingCursor();
public slots:
    void savePlot();
    void switchCursor();
    void copyToClipboard();
    void print();
signals:
    void curveChanged(Curve *curve);
    void curveDeleted(FileDescriptor *descriptor, int index);
private slots:
    void editLegendItem(const QVariant &itemInfo, int index);
    void deleteGraph(const QVariant &info, int index);
    void updateTrackingCursor(double xVal, bool second);
    void showContextMenu(const QPoint &pos, const int axis);
    void trackCursors(const QPoint &pos);
private:
    void importPlot(const QString &fileName);

    // axis labels
    QString xName;
    QString yLeftName;
    QString yRightName;
    bool axisLabelsVisible;

    QList<Curve *> leftGraphs;
    QList<Curve *> rightGraphs;

    QwtPlotGrid *grid;
    PlotPicker *picker;
    QwtPlotCanvas *canvas;

    bool xScale; //false = linear, true = logarithmic
    bool y1Scale;//false = linear, true = logarithmic
    bool y2Scale;//false = linear, true = logarithmic

    QwtChartZoom *zoom;

    TrackingPanel *trackingPanel;
    TrackingCursor *_trackingCursor;
    TrackingCursor *_trackingCursor1;
    bool _showTrackingCursor;

    Range x1;
    Range y1;
    Range y2;

    InteractionMode interactionMode;
};

#endif // PLOT_H
