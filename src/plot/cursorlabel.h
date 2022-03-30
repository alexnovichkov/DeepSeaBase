#ifndef CURSORLABEL_H
#define CURSORLABEL_H

#include "qwt_plot_item.h"
#include "qwt_text.h"
#include <QPointF>
#include "selectable.h"

class Plot;
class TrackingCursor;

class CursorLabel : public QwtPlotItem, public Selectable
{
public:
    enum class Axis
    {
        XAxis,
        YAxis
    };
    CursorLabel(Plot *parent, TrackingCursor *cursor);
    void updateAlignment();
    void setAxis(Axis axis);
    void updateLabel(bool showValues);
private:
    Plot *m_plot;
    TrackingCursor *m_cursor;
    QwtText m_label;
    Axis m_axis = Axis::XAxis;

    // QwtPlotItem interface
public:
    virtual int rtti() const override;
    virtual void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const override;

    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const override;
    virtual QList<QAction *> actions() override; //не имеет собственных действий, берет из курсора
protected:
    virtual void updateSelection() override;
};

#endif // CURSORLABEL_H
