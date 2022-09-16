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
    //describes the priority of selecting objects
    enum class PickPriority {
        PickFirst, //mouse click immediately picks objects if any under mouse
        PickLast //Picking only if no dragging or selecting occurred
    };
    enum class SelectedObject {
        NewObject,
        AlreadySelected,
        Nothing
    };

    Picker(Plot *plot);
    inline bool isEnabled() const {return enabled;}
    inline void setEnabled(bool enabled) {this->enabled = enabled;}
    Selectable *findObject(QMouseEvent *e);
    void startPick(QPoint startPos, Selectable *selected);
    void deselect();
    void proceedPick(QMouseEvent *e);
    void endPick(QMouseEvent *e);
    void procKeyboardEvent(int key);
    void showContextMenu(QMouseEvent *e);
    inline bool alreadySelected(Selectable *selected) {return selected && currentSelected == selected; }

    void setPickPriority(PickPriority priority) {m_priority = priority;}
    inline PickPriority pickPriority() const {return m_priority;}
signals:
    void removeNeeded(Selectable*);

//    void cursorMovedTo(QPointF newValue);
//    void cursorSelected(TrackingCursor *cursor);
//    void axisClicked(QPointF value, bool secondCursor);
private:
    Plot *plot;
    bool enabled;
    QPoint startPosition;

    QPoint pos;

    Selectable *currentSelected = nullptr;
    PickPriority m_priority = PickPriority::PickFirst;
};

#endif // PICKER_H
