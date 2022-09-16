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

    Cursor::Style type = Cursor::Style::Vertical;
    Cursor *parent = nullptr;

    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const override;
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;
    virtual void moveLeft(int count = 1) override;
    virtual void moveRight(int count = 1) override;
    virtual void moveUp(int count = 1) override;
    virtual void moveDown(int count = 1) override;
    virtual QList<QAction *> actions() override;
    virtual bool draggable() const override {return true;}
protected:
    virtual void updateSelection() override;
private:
    QAction *energyAct;
    QAction *rmsAct;
};

#endif // TRACKINGCURSOR_H
