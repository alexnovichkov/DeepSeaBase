#ifndef SELECTABLE_H
#define SELECTABLE_H

#include <QPoint>

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
    virtual void moveToPos(QPoint pos) {Q_UNUSED(pos)};
    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const = 0;
protected:
    virtual void updateSelection() = 0;
    virtual bool updateAnyway() const {return false;}
private:
    bool m_selected = false;
};

#endif // SELECTABLE_H
