#ifndef CURSOR_H
#define CURSOR_H

#include <QObject>
#include <QColor>
#include <QPointF>

class Plot;

class Cursor : public QObject
{
    Q_OBJECT
public:
    enum class Type
    {
        Single,
        Double,
        DoubleReject,
        Harmonic,
        Peak
    };
    enum class Style
    {
        Vertical,
        Horizontal,
        Cross
    };

    Cursor(Type type, Style style, Plot *plot = nullptr);
    virtual ~Cursor() {}

    virtual void setColor(const QColor &color) {this->color = color;}
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2) = 0;
    virtual void moveTo(const QPointF &pos1) = 0;
    virtual void updatePos() = 0;
    virtual void attach() = 0;
    virtual void detach() = 0;

    void setSnapToValues(bool snap);
    QPointF correctedPos(QPointF oldPos);

protected:
    Type type;
    Style style;
    QColor color = QColor(Qt::black);
    Plot *plot = nullptr;
    bool snapToValues = false;
};

#endif // CURSOR_H
