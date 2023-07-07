#ifndef SECONDARYPLOT_H
#define SECONDARYPLOT_H

#include <QObject>
#include <QMap>
#include <enums.h>

class QCPAxisRect;
class QCPGraph;
class QCPTextElement;
class QCPLegend;
class QCPPlot;
class QCPLayoutGrid;
class QCPAxis;
class Cursor;
class Curve;
class ColorSelector;

class SecondaryPlot : public QObject
{
    Q_OBJECT
public:
    explicit SecondaryPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
    virtual ~SecondaryPlot() = default;
    inline QCPLayoutGrid *layout() const {return m_layout;}
    void clear();
    void update();
    QCPAxis *axis(Enums::AxisType axis) const;

    void addCursor(Cursor *cursor);
    void removeCursor(Cursor *cursor);
    void setCurve(Curve* curve);

signals:
protected:
    virtual QVector<double> xData(Cursor *cursor) const = 0;
    virtual QVector<double> yData(Cursor *cursor) const = 0;
    virtual QString value(Cursor *cursor) const = 0;

    QCPAxisRect *m_axisRect = nullptr;
    QCPTextElement *m_title = nullptr;
    QMap<Cursor*, QCPGraph*> m_graphs;
    QCPLegend *m_legend = nullptr;
    QCPPlot *m_parent = nullptr;
    QCPLayoutGrid *m_layout = nullptr;
    Curve *m_curve = nullptr;
    ColorSelector *m_colorSelector = nullptr;
};

class SpectrePlot : public SecondaryPlot
{
    Q_OBJECT
public:
    explicit SpectrePlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
protected:
    virtual QVector<double> xData(Cursor *cursor) const override;
    virtual QVector<double> yData(Cursor *cursor) const override;
    virtual QString value(Cursor *cursor) const override;
};

class ThroughPlot : public SecondaryPlot
{
    Q_OBJECT
public:
    explicit ThroughPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
protected:
    virtual QVector<double> xData(Cursor *cursor) const override;
    virtual QVector<double> yData(Cursor *cursor) const override;
    virtual QString value(Cursor *cursor) const override;
};

#endif // SECONDARYPLOT_H
