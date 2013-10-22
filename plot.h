#ifndef PLOT_H
#define PLOT_H

#include <qwt_plot.h>
#include <qwt_plot_dict.h>
#include <math.h>


class QwtLegend;
class QwtPlotGrid;
class Curve;
class DfdFileDescriptor;

class QwtChartZoom;
class QWheelZoomSvc;
class QAxisZoomSvc;
class QwtPlotZoomer;

struct Range {
    void clear() {min = INFINITY; max = -INFINITY;}
    double min;
    double max;
};

class Plot : public QwtPlot
{
    Q_OBJECT
public:
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

    void deleteGraphs();
    void deleteGraphs(const QString &dfdGuid);

    /**
     * @brief deleteGraph deletes plotted graphs
     * @param dfd
     * @param channel
     */
    void deleteGraph(DfdFileDescriptor *dfd, int channel, bool doReplot = true);
    void deleteGraph(Curve *graph, bool doReplot = true);

    bool plotChannel(DfdFileDescriptor *dfd, int channel, bool addToFixed);

    Curve *plotted(DfdFileDescriptor *dfd, int channel) const;

    /**
     * @brief updateLegends
     * updates legends with the dfd legend
     */
    void updateLegends();
public slots:
    void savePlot();
private:
    QList<Curve *> graphs;
    Curve *freeGraph;

    QString xName;
    QString yLeftName;
    QString yRightName;
    QList<Curve *> leftGraphs;
    QList<Curve *> rightGraphs;

    QwtPlotGrid *grid;

//    QwtPlotZoomer *zoom;
    QwtChartZoom *zoom;
    QWheelZoomSvc *wheelZoom;
    QAxisZoomSvc *axisZoom;

    Range x1;
    Range y1;
    Range y2;
//    double xMax;
//    double xMin;
//    double yMax;
//    double yMin;
};

#endif // PLOT_H
