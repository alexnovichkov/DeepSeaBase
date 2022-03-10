#ifndef TRACKINGCURSOR_H
#define TRACKINGCURSOR_H

#include <qwt_plot_marker.h>
#include <QVector>
#include <qwt_text.h>
#include "cursor.h"
#include "selectable.h"

class TrackingCursor : public QwtPlotMarker, public Selectable
{
public:
    explicit TrackingCursor(const QColor &col, Cursor::Style type, Cursor *parent);
    void moveTo(const double xValue);
    void moveTo(const QPointF &value);
    void setYValues(const QVector<double> &yValues, const QVector<QColor> &colors);
    void updateLabel();
    bool showYValues;
    QVector<double> yValues;
    QVector<QColor> colors;
    bool reject = false;

    Cursor::Style type = Cursor::Style::Vertical;
    QwtText yLabel;
    QwtText xLabel;

    Cursor *parent = nullptr;

    double xVal=0.0;
    double yVal=0.0;
    double zVal=0.0;

    bool showLabel = true;

    // QwtPlotMarker interface
protected:
    void drawLabel(QPainter *painter, const QRectF &canvasRect, const QPointF &pos) const override;
    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const override;
    virtual void moveToPos(QPoint pos) override;
protected:
    virtual void updateSelection() override;
};

#endif // TRACKINGCURSOR_H
