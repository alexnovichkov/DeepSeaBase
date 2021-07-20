#ifndef PICKER_H
#define PICKER_H

#include <QObject>

#include "plot.h"
#include "enums.h"

class PointLabel;
class PointMarker;
class TrackingCursor;
class Curve;

class Picker: public QObject
{
    Q_OBJECT
public:
    enum Mode {
        ModeNone,
        ModeDrag
    };
    Picker(Plot *plot);
    inline void setMode(Plot::InteractionMode mode) {interactionMode = mode;}
    inline bool isEnabled() const {return enabled;}
    inline void setEnabled(bool enabled) {this->enabled = enabled;}
    bool findObject(QMouseEvent *e);
    void proceedPick(QMouseEvent *e);
    void endPick(QMouseEvent *e);
    void procKeyboardEvent(int key);
signals:
    void cursorMovedTo(QPointF newValue);
    void cursorSelected(TrackingCursor *cursor);
    void axisClicked(QPointF value, bool secondCursor);
private:





    QVector<TrackingCursor *> findCursors(const QPoint &pos);
    Curve *findClosestPoint(const QPoint &pos, int &index) const;
    PointLabel *findLabel(const QPoint &pos);

    void highlightPoint(bool enable);

    Plot *plot;
    Plot::InteractionMode interactionMode;
    Mode mode;
    bool enabled;

    int d_selectedPoint;
    Curve *d_selectedCurve;
    PointLabel *d_selectedLabel;
    QVector<TrackingCursor *> d_selectedCursors;

    QPoint d_currentPos;
    PointMarker *marker;

    QPoint pos;
};

#endif // PICKER_H
