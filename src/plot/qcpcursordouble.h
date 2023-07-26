#ifndef QCPCURSORDOUBLE_H
#define QCPCURSORDOUBLE_H

#include "cursor.h"

class QCPTrackingCursor;
class CursorLabel;
class QCPAxisTag;
class QCPZone;

class QCPCursorDouble : public Cursor
{
    Q_OBJECT
public:
    explicit QCPCursorDouble(Style style, bool reject, QCPPlot *plot = nullptr);
    ~QCPCursorDouble();
    virtual void setColor(const QColor &color) override;
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent=false) override;
    virtual void moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent=false) override;
    virtual void updatePos() override;
//    virtual void attach() override;
    virtual void detach() override;
    virtual bool contains(Selectable *selected) const override;
    virtual void update() override;
    virtual int  dataCount(bool allData) const override;
    virtual QStringList dataHeader(bool allData) const override;
    virtual QList<double> data(int curve, bool allData) const override;
    virtual QPointF currentPosition() const override;
    QPair<double, double> interval() const;

//    virtual QStringList getValues() const override;
private:
    QCPTrackingCursor *m_cursor1 = nullptr;
    QCPTrackingCursor *m_cursor2 = nullptr;
    QCPAxisTag *m_axisTag1 = nullptr;
    QCPAxisTag *m_axisTag2 = nullptr;
    QCPPlot *m_plot = nullptr;
    QCPZone *m_zone = nullptr;
};

#endif // QCPCURSORDOUBLE_H
