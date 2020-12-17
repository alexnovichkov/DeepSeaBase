#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <QVector>
#include <QColor>
#include <QVariantList>

#define COLORS_COUNT 32

class ColorSelector
{
public:
    ColorSelector(const QVariantList &list);

    int colorsCount() const {return colors.size();}
    void addColor(const QColor &color);

    void setColor(const QColor &color, int index);

    void removeColor(int index);

    QColor getColor();
    QColor color(int index) const;

    QVariantList getColors() const;

    void resetState();

    void freeColor(const QColor &color);

private:
    QVector<QColor> colors;
};

#endif // COLORSELECTOR_H
