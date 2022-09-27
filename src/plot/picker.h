#ifndef PICKER_H
#define PICKER_H

#include <QObject>

#include "plot.h"
#include "enums.h"
#include "selectable.h"

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

    Picker(Plot *plot);
    inline bool isEnabled() const {return enabled;}
    inline void setEnabled(bool enabled) {this->enabled = enabled;}
    Selected findObject(QMouseEvent *e);
    void startPick(QPoint startPos, Selected selected);
    void deselect();
    void proceedPick(QMouseEvent *e);
    void endPick(QMouseEvent *e);
    void procKeyboardEvent(int key);
    void showContextMenu(QMouseEvent *e);
    bool alreadySelected(Selected selected);

    void setPickPriority(PickPriority priority) {m_priority = priority;}
    inline PickPriority pickPriority() const {return m_priority;}
signals:
    void removeNeeded(Selectable*);
private:
    Plot *plot;
    bool enabled;
    QPoint startPosition;

    QPoint pos;

    Selected currentSelected;
    PickPriority m_priority = PickPriority::PickFirst;
};

#endif // PICKER_H
