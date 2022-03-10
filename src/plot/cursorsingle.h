#ifndef CURSORSINGLE_H
#define CURSORSINGLE_H

#include "cursor.h"

class TrackingCursor;
class CursorLabel;

class CursorSingle : public Cursor
{
    Q_OBJECT
public:
    CursorSingle(Style style, Plot *plot = nullptr);
    ~CursorSingle();

    virtual void setColor(const QColor &color) override;
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2) override;
    virtual void moveTo(const QPointF &pos1) override;
    virtual void updatePos() override;
    virtual void attach() override;
    virtual void detach() override;
private:
    TrackingCursor *cursor;
    CursorLabel *xlabel = nullptr;
    CursorLabel *ylabel = nullptr;
};

#endif // CURSORSINGLE_H
