#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
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

struct Range {
    void clear() {min = INFINITY; max = -INFINITY;}
    double min;
    double max;
};

class Plot : public QwtPlot
{
    Q_OBJECT
public:
    enum InteractionMode {
        NoInteraction,
        ScalingInteraction,
        DataInteraction
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
    bool switchInteractionMode();
public slots:
    void savePlot();
    void switchCursor();
    void copyToClipboard();
    void print();

    void onCurveChanged(Curve*);


signals:
//    void fileCreated(const QString &fileName, bool plot);
//    void fileChanged(const QString &fileName, bool plot);
    void curveChanged(Curve *curve);
    void curveDeleted(Curve *curve);
private slots:
    void editLegendItem(const QVariant &itemInfo, int index);
    void deleteGraph(const QVariant &info, int index);
private:
    void importPlot(const QString &fileName);
//    Curve *freeGraph;

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

    QwtChartZoom *zoom;



    Range x1;
    Range y1;
    Range y2;

    InteractionMode interactionMode;


};

#endif // PLOT_H
