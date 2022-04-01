#ifndef PICKER_H
#define PICKER_H

#include <QObject>

#include "plot.h"
#include "enums.h"


class Selectable;
class PointLabel;
class PointMarker;
class TrackingCursor;
class Curve;

class Picker: public QObject
{
    Q_OBJECT
public:
//    enum Mode {
//        ModeNone,
//        ModeDrag
//    };
    Picker(Plot *plot);
    inline bool isEnabled() const {return enabled;}
    inline void setEnabled(bool enabled) {this->enabled = enabled;}
    bool findObject(QMouseEvent *e);
    void startPick(QPoint startPos);
    void deselect();
    void proceedPick(QMouseEvent *e);
    void endPick(QMouseEvent *e);
    void procKeyboardEvent(int key);
    void showContextMenu(QMouseEvent *e);
signals:
    void removeNeeded(Selectable*);

//    void cursorMovedTo(QPointF newValue);
//    void cursorSelected(TrackingCursor *cursor);
//    void axisClicked(QPointF value, bool secondCursor);
private:
    Plot *plot;
//    Mode mode;
    bool enabled;
    QPoint startPosition;

    QPoint pos;

    Selectable *currentSelected = nullptr;
};

#endif // PICKER_H
