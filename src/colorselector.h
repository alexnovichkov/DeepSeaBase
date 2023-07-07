#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <QVector>
#include <QColor>
#include <QVariantList>
#include <QSet>

#define COLORS_COUNT 32

class ColorSelector
{
public:
    ColorSelector();

    int colorsCount() const {return COLORS_COUNT;}
//    void addColor(const QColor &color);

    void setColor(QColor color, int index);

//    void removeColor(int index);

    QColor getColor();
    QColor color(int index) const;

    QVariantList getColors() const;

//    void resetState();

    void freeColor(const QColor &color);

private:
    //QVector<QColor> colors;
    QVector<bool> usedColors;
};

#endif // COLORSELECTOR_H
