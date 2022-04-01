#ifndef CURSORDOUBLE_H
#define CURSORDOUBLE_H

#include "cursor.h"
#include <qwt_plot_zoneitem.h>

class TrackingCursor;
class CursorLabel;

class Zone : public QwtPlotZoneItem
{
public:
    explicit Zone(QColor color);
    void setRange(const QPointF &p1, const QPointF &p2);
};

class CursorDouble : public Cursor
{
    Q_OBJECT
public:
    CursorDouble(Style style, bool reject, Plot *plot = nullptr);
    ~CursorDouble();

    virtual void setColor(const QColor &m_color) override;
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, TrackingCursor *source, bool silent=false) override;
    virtual void moveTo(Qt::Key key, int count, TrackingCursor *source, bool silent=false) override;
    virtual void updatePos() override;
    virtual void attach() override;
    virtual void detach() override;
    virtual bool contains(Selectable *selected) const override;
    virtual void update() override;
    virtual int dataCount(bool allData) const override;
    virtual QStringList dataHeader(bool allData) const override;
    virtual QList<double> data(int curve, bool allData) const override;
    virtual QPointF currentPosition() const override;
    QwtInterval interval() const;
//    virtual QStringList getValues() const override;
private:
    TrackingCursor *cursor1 = nullptr;
    TrackingCursor *cursor2 = nullptr;

    CursorLabel *xlabel1 = nullptr;
    CursorLabel *xlabel2 = nullptr;

    Zone *zone = nullptr;
};


#endif // CURSORDOUBLE_H
