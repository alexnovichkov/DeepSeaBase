#ifndef CURSORHARMONIC_H
#define CURSORHARMONIC_H

#include "cursor.h"

class QCPTrackingCursor;
class QCPAxisTag;
class Plot;

class QCPCursorHarmonic : public Cursor
{
    Q_OBJECT
public:
    QCPCursorHarmonic(Plot *plot = nullptr);
    ~QCPCursorHarmonic();

    virtual void setColor(const QColor &m_color) override;
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, bool silent=false) override;
    virtual void moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent=false) override;
    virtual void moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent=false) override;
    virtual void updatePos() override;
    virtual void attach() override;
    virtual void detach() override;
    virtual bool contains(Selectable *selected) const override;
    virtual void update() override;
    virtual int dataCount(bool allData) const override {Q_UNUSED(allData); return 1;}
    virtual QStringList dataHeader(bool allData) const override;
    virtual QList<double> data(int curve, bool allData) const override;
    virtual QPointF currentPosition() const override;
//    virtual QStringList getValues() const override;
private:
    Plot *plot;
    QCPTrackingCursor *cursor = nullptr;
    QList<QCPTrackingCursor *> cursors;

    QCPAxisTag *label = nullptr;
    QList<QCPAxisTag *> labels;
};


#endif // CURSORHARMONIC_H
