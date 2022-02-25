#ifndef TRACKINGCURSOR_H
#define TRACKINGCURSOR_H

#include <qwt_plot_marker.h>
#include <QVector>
#include <qwt_text.h>

class TrackingCursor : public QwtPlotMarker
{
public:
    enum Type {
        Vertical=0,
        Horizontal=1,
        Cross=2
    };
    explicit TrackingCursor(const QColor &col, Type type = Vertical);
    void moveTo(const double xValue);
    void moveTo(const QPointF &value);
    void setYValues(const QVector<double> &yValues, const QVector<QColor> &colors);
    void setCurrent(bool current);

    void updateLabel();
    bool showYValues;
    QVector<double> yValues;
    QVector<QColor> colors;
    bool reject = false;
    bool current = false;
    Type type = Vertical;
    QwtText yLabel;
    QwtText xLabel;

    double xVal=0.0;
    double yVal=0.0;
    double zVal=0.0;

    // QwtPlotMarker interface
protected:
    void drawLabel(QPainter *painter, const QRectF &canvasRect, const QPointF &pos) const;
};

#endif // TRACKINGCURSOR_H
