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

struct SecondaryColors
{
    QColor color;
    bool taken;
};

class SecondaryPlot : public QObject
{
    Q_OBJECT
public:
    explicit SecondaryPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
    virtual ~SecondaryPlot() = default;
    inline QCPLayoutGrid *layout() const {return m_layout;}
    void clear();
    void update(const QVector<double> &xData, const QVector<double> &yData);
    virtual void update() = 0;
    QCPAxis *axis(Enums::AxisType axis) const;

    void addCursor(Cursor *cursor);
    void removeCursor(Cursor *cursor);
    void setCurve(Curve* curve);

signals:
protected:
    QColor firstFreeColor();
    void freeColor(QColor color);

    QCPAxisRect *m_axisRect = nullptr;
    QCPTextElement *m_title = nullptr;
    QMap<Cursor*, QCPGraph*> m_graphs;
    QCPLegend *m_legend = nullptr;
    QCPPlot *m_parent = nullptr;
    QCPLayoutGrid *m_layout = nullptr;
    Curve *m_curve = nullptr;
    SecondaryColors secondaryColors[12] =
      {{QColor(79,129,189),false}, {QColor(192,80,77),false}, {QColor(155,187,89),false},
       {QColor(128,100,162),false}, {QColor(75,172,198),false}, {QColor(247,150,70),false},
       {QColor(44,77,117),false}, {QColor(119,44,42),false}, {QColor(95,117,48),false},
       {QColor(77,59,98),false}, {QColor(39,106,124),false}, {QColor(182,87,8),false}};
};

class SpectrePlot : public SecondaryPlot
{
    Q_OBJECT
public:
    explicit SpectrePlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
    virtual void update() override;
};

class ThroughPlot : public SecondaryPlot
{
    Q_OBJECT
public:
    explicit ThroughPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout);
    virtual void update() override;
};

#endif // SECONDARYPLOT_H
