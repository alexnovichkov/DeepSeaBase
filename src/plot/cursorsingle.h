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

    virtual void setColor(const QColor &m_color) override;
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2) override;
    virtual void moveTo(const QPointF &pos1) override;
    virtual void moveTo(const QPointF &pos1, TrackingCursor *source) override;
    virtual void moveTo(Qt::Key key, int count, TrackingCursor *source) override;
    virtual void updatePos() override;
    virtual void attach() override;
    virtual void detach() override;
    virtual bool contains(Selectable *selected) const override;
    virtual void update() override;
    virtual int  dataCount(bool allData) const override {return 1;}
    virtual QStringList dataHeader(bool allData) const override;
    virtual QList<double> data(int curve, bool allData) const override;

//    virtual QStringList getValues() const override;
private:
    TrackingCursor *cursor;
    CursorLabel *xlabel = nullptr;
    CursorLabel *ylabel = nullptr;
};

#endif // CURSORSINGLE_H
