// xlsxchart_p.h

#ifndef QXLSX_CHART_P_H
#define QXLSX_CHART_P_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <memory>

#include "xlsxabstractooxmlfile_p.h"
#include "xlsxchart.h"

QT_BEGIN_NAMESPACE_XLSX

class XlsxSeries
{
public:
    //At present, we care about number cell ranges only!
    QString numberDataSource_numRef; // yval, val
    QString axDataSource_numRef; // xval, cat
    QString headerH_numRef;
    QString headerV_numRef;
    bool    swapHeader = false;
    ShapeProperties shape;
    MarkerFormat marker;
    QList<int> axes; //2...

    Labels labels;
    void addLabel(int label, Label::ShowParameters show, Label::Position pos);
};

class ChartPrivate : public AbstractOOXmlFilePrivate
{
    Q_DECLARE_PUBLIC(Chart)

public:
    ChartPrivate(Chart *q, Chart::CreateFlag flag);
    ~ChartPrivate();

public:
    bool loadXmlChart(QXmlStreamReader &reader);
    bool loadXmlPlotArea(QXmlStreamReader &reader);
protected:
    bool loadXmlPlotAreaElement(QXmlStreamReader &reader);
public:
    bool loadXmlXxxChart(QXmlStreamReader &reader);
    bool loadXmlSer(QXmlStreamReader &reader);
    QString loadXmlNumRef(QXmlStreamReader &reader);
    QString loadXmlStrRef(QXmlStreamReader &reader);
    bool loadXmlChartTitle(QXmlStreamReader &reader);
    bool loadXmlChartLegend(QXmlStreamReader &reader);
protected:
    bool loadXmlChartTitleTx(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRich(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRichP(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRichP_R(QXmlStreamReader &reader);
protected:
    bool loadXmlAxisCatAx(QXmlStreamReader &reader);
    bool loadXmlAxisDateAx(QXmlStreamReader &reader);
    bool loadXmlAxisSerAx(QXmlStreamReader &reader);
    bool loadXmlAxisValAx(QXmlStreamReader &reader);
    bool loadXmlAxisEG_AxShared(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Scaling(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Overlay(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P_pPr(QXmlStreamReader &reader, Axis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P_R(QXmlStreamReader &reader, Axis* axis);

    QString readSubTree(QXmlStreamReader &reader);

public:
    void saveXmlChart(QXmlStreamWriter &writer) const;
    void saveXmlChartTitle(QXmlStreamWriter &writer) const;
    void saveXmlPieChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlBarChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlLineChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlScatterChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlAreaChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlDoughnutChart(QXmlStreamWriter &writer, QList<int> axes) const;
    void saveXmlSer(QXmlStreamWriter &writer, XlsxSeries *ser, int id) const;
    void saveXmlChartLegend(QXmlStreamWriter &writer) const;

public:
    Chart::ChartType chartType;
    QList< std::shared_ptr<XlsxSeries> > seriesList;
    QList< std::shared_ptr<Axis> > axisList;
    QString chartTitle;
    AbstractSheet* sheet;
    Chart::ChartAxisPos legendPos;
    bool legendOverlay;
    bool majorGridlinesEnabled;
    bool minorGridlinesEnabled;
    ShapeProperties plotAreaShapeProperties;
    ShapeProperties chartShapeProperties;

    QString layout;             // only for storing a readed file
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_CHART_P_H
