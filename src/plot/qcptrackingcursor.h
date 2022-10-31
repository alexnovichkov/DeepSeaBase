#ifndef TRACKINGCURSOR_H
#define TRACKINGCURSOR_H

#include "qcustomplot.h"
#include <QVector>
#include "cursor.h"
#include "selectable.h"
#include "enums.h"

class QCPPlot;

class QCPTrackingCursor : public Selectable
{
public:
    explicit QCPTrackingCursor(const QColor &col, Cursor::Style type, Cursor *parent);
    void moveTo(const double xValue);
    void moveTo(const QPointF &value);

    double xValue() const;
    double yValue() const;
    QPointF value() const {return {xValue(), yValue()};}

    void detach();

    Enums::AxisType xAxis() const {return Enums::AxisType::atBottom;}
    Enums::AxisType yAxis() const {return Enums::AxisType::atLeft;}

    void setColor(const QColor &color);
    void setPen(const QPen &pen);

    Cursor::Style type = Cursor::Style::Vertical;
    Cursor *parent = nullptr;

    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;
    virtual void moveLeft(int count = 1) override;
    virtual void moveRight(int count = 1) override;
    virtual void moveUp(int count = 1) override;
    virtual void moveDown(int count = 1) override;
    virtual QList<QAction *> actions() override;
    virtual bool draggable() const override {return true;}
protected:
    virtual void updateSelection(SelectedPoint point) override;
private:

    QCPPlot *impl;
    QAction *energyAct;
    QAction *rmsAct;

    QCPItemStraightLine *horizontal;
    QCPItemStraightLine *vertical;
};

#endif // TRACKINGCURSOR_H
