#ifndef SELECTABLE_H
#define SELECTABLE_H

#include <QPoint>
#include <QList>
#include <QAction>

class Selectable
{
public:
    virtual ~Selectable() {}
    inline bool selected() const {return m_selected;}
    inline void setSelected(bool selected) {
        if (m_selected != selected || updateAnyway())
        {
            m_selected = selected;
            updateSelection();
        }
    }
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) {Q_UNUSED(pos)}
    virtual void moveLeft(int count = 1) {Q_UNUSED(count)}
    virtual void moveRight(int count = 1) {Q_UNUSED(count)}
    virtual void moveUp(int count = 1) {Q_UNUSED(count)}
    virtual void moveDown(int count = 1) {Q_UNUSED(count)}

    virtual void cycle() {} //changes the appearance of the object (f.e. of PointLabel)
    virtual void remove() {} //removes this object
    virtual void fix() {} //adds some fixed object to the plot (f.e. PointLabel for Curve)

    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const = 0;
    virtual QList<QAction*> actions() {return QList<QAction *>();}
protected:
    virtual void updateSelection() = 0;
    virtual bool updateAnyway() const {return false;}
private:
    bool m_selected = false;
};

#endif // SELECTABLE_H
