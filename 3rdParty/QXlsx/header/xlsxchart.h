// xlsxchart.h

#ifndef QXLSX_CHART_H
#define QXLSX_CHART_H

#include <QtGlobal>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "xlsxabstractooxmlfile.h"
#include "xlsxshapeproperties.h"
#include "xlsxlineformat.h"
#include "xlsxmarkerformat.h"
#include "xlsxaxis.h"
#include "xlsxlabel.h"

QT_BEGIN_NAMESPACE_XLSX

class AbstractSheet;
class Worksheet;
class ChartPrivate;
class CellRange;
class DrawingAnchor;

class QXLSX_EXPORT Chart : public AbstractOOXmlFile
{
    Q_DECLARE_PRIVATE(Chart)
public:
    enum ChartType { // 16 type of chart (ECMA 376)
        CT_NoStatementChart = 0, // Zero is internally used for unknown types
        CT_AreaChart, CT_Area3DChart, CT_LineChart,
        CT_Line3DChart, CT_StockChart, CT_RadarChart,
        CT_ScatterChart, CT_PieChart, CT_Pie3DChart,
        CT_DoughnutChart, CT_BarChart, CT_Bar3DChart,
        CT_OfPieChart, CT_SurfaceChart, CT_Surface3DChart,
        CT_BubbleChart,
    };

    enum ChartAxisPos { None = (-1), Left = 0, Right, Top, Bottom  };
private:
    friend class AbstractSheet;
    friend class Worksheet;
    friend class Chartsheet;
    friend class DrawingAnchor;
    friend class LineFormat;
private:
    Chart(AbstractSheet *parent, CreateFlag flag);
public:
    ~Chart();
public:
    void addSeries(const CellRange &range, AbstractSheet *sheet = NULL, bool headerH = false, bool headerV = false, bool swapHeaders = false);
    void addSeries(const CellRange &keyRange, const CellRange &valRange, AbstractSheet *sheet = NULL, bool headerH = false);
    void setChartType(ChartType type_);
    void setChartLineFormat(const LineFormat &format);
    void setPlotAreaLineFormat(const LineFormat &format);

    Axis *addAxis(Axis::Type type_, Axis::Position pos, QString title = QString());
    Axis *axis(int id);

    void setChartTitle(QString strchartTitle);
    void setChartLegend(Chart::ChartAxisPos legendPos, bool overlap = false);
    void setGridlinesEnable(bool majorGridlinesEnable = false, bool minorGridlinesEnable = false);
    void setSeriesLineFormat(int series, const LineFormat &format);
    void setSeriesShape(int series, const Shape &shape);
    void setSeriesMarkerFormat(int series, const MarkerFormat &format);
    void setSeriesAxes(int series, QList<int> axesIds);
    void setSeriesLabels(int series, QVector<int> labels, QXlsx::Label::ShowParameters show, QXlsx::Label::Position pos);
public:
    bool loadFromXmlFile(QIODevice *device) override;
    void saveToXmlFile(QIODevice *device) const override;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_CHART_H
