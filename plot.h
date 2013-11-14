#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>


class QwtLegend;
class QwtPlotGrid;
class QwtPlotCanvas;
class Curve;
class DfdFileDescriptor;
class Channel;

class QwtChartZoom;
class QwtPlotZoomer;
class PlotPicker;
class CanvasPicker;

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
     * @brief hasFreeGraph
     * @return whether the plot has free graph plotted
     */
    bool hasFreeGraph() const;

    /**
     * @brief deleteGraphs
     * deletes all graphs on a plot
     */
    void deleteGraphs();
    /**
     * @brief deleteGraphs deletes all graphs on a plot, which represent DFDd with dfdGuid
     * @param dfdGuid
     */
    void deleteGraphs(const QString &dfdGuid);

    /**
     * @brief deleteGraph deletes plotted graph
     * @param dfd - DFD represented by a graph
     * @param channel
     */
    void deleteGraph(DfdFileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteGraph(Curve *graph, bool doReplot = true);

    bool plotChannel(DfdFileDescriptor *dfd, int channel, bool addToFixed, QColor *col);

    Curve *plotted(DfdFileDescriptor *dfd, int channel) const;

    /**
     * @brief updateLegends
     * updates legends with the dfd legend
     */
    void updateLegends();

    bool canBePlottedOnLeftAxis(Channel *ch, bool addToFixed);
    bool canBePlottedOnRightAxis(Channel *ch, bool addToFixed);

    void prepareAxis(int axis, const QString &name);

    void moveGraph(Curve *curve);

    void setInteractionMode(InteractionMode mode);
public slots:
    void savePlot();
    void switchCursor();
    void copyToClipboard();
    void print();

    void calculateMean();
    void switchInteractionMode();
signals:
    void fileCreated(const QString &fileName, bool plot);
    void fileChanged(const QString &fileName, bool plot);
private slots:
    void editLegendItem(const QVariant &itemInfo, int index);
    void pointSelected(const QPointF &pos);
private:
    void importPlot(const QString &fileName);
    QList<Curve *> graphs;
    Curve *freeGraph;

    QString xName;
    QString yLeftName;
    QString yRightName;
    QList<Curve *> leftGraphs;
    QList<Curve *> rightGraphs;

    QwtPlotGrid *grid;
    PlotPicker *picker;
    QwtPlotCanvas *canvas;
    CanvasPicker *canvasPicker;

    QwtChartZoom *zoom;


    Range x1;
    Range y1;
    Range y2;

    InteractionMode interactionMode;
};

#endif // PLOT_H
