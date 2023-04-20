#ifndef SELECTABLE_H
#define SELECTABLE_H

#include <QPoint>
#include <QList>
#include <QAction>
#include <QDebug>

class Selectable;

/**
 * @brief The SelectedPoint struct
 * Задает выделенную точку на кривой или сонограмме, в номерах отсчетов, по оси x (номер отсчета) и по оси z (номер блока)
 */
struct SelectedPoint {
    int x = -1;
    int z = -1;
    inline bool valid() const {return x!=-1 && z!=-1;}
    inline bool operator==(const SelectedPoint &other) {return other.x == x && other.z == z;}
};

/**
 * @brief The SamplePoint struct
 * Задает произвольную точку на канве графика
 */
struct SamplePoint
{
    double x = qQNaN();
    double y = qQNaN();
    double z = qQNaN();
    inline bool operator==(const SamplePoint &other) {
        return qFuzzyCompare(x+1.0, other.x+1.0)
                && qFuzzyCompare(y+1.0, other.y+1.0)
                && qFuzzyCompare(z+1.0, other.z+1.0);
    }
};

/**
 * @brief The Selected struct
 * Задает выделенный объект на канве:
 * object - указатель на объект
 * point - выделенная точка на объекте
 */
struct Selected
{
    Selectable *object = nullptr;
    SelectedPoint point;
    inline bool operator==(const Selected &other) {
        if (object != other.object) return false;
        if (point.valid() || other.point.valid()) return point==other.point;
        return false;
    }
    inline void clear() {object = nullptr; point.x=-1; point.z=-1;}
};

QDebug operator<<(QDebug, const Selected &);

/**
 * @brief The Selectable class
 * Абстрактный класс, выражающий выделяемый объект на канве.
 * Наследники: кривые, метки, курсоры и т.д.
 */
class Selectable
{
public:
    virtual ~Selectable() {}
    inline bool selected() const {return m_selected;}
    inline void setSelected(bool selected, SelectedPoint point) {
        if (m_selected != selected || updateAnyway())
        {
            m_selected = selected;
            updateSelection(point);
        }
    }
    virtual bool draggable() const = 0;
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) {Q_UNUSED(pos); Q_UNUSED(startPos);}
    virtual void moveLeft(int count = 1) {Q_UNUSED(count)}
    virtual void moveRight(int count = 1) {Q_UNUSED(count)}
    virtual void moveUp(int count = 1) {Q_UNUSED(count)}
    virtual void moveDown(int count = 1) {Q_UNUSED(count)}

    virtual void cycle() {} //changes the appearance of the object (f.e. of PointLabel)
    virtual void remove() {} //removes this object
    virtual void fix() {} //adds some fixed object to the plot (f.e. PointLabel for Curve)

    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY,
                            SelectedPoint *point) const = 0;
    virtual bool selectedAs(Selected *selected) const {return this == selected->object;}
    virtual QList<QAction*> actions() {return QList<QAction *>();}
protected:
    virtual void updateSelection(SelectedPoint point) = 0;
    virtual bool updateAnyway() const {return false;}
private:
    bool m_selected = false;
};

#endif // SELECTABLE_H
