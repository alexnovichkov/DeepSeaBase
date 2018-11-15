#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <QVector>
#include <QColor>

#define COLORS_COUNT 32

class ColorSelector
{
public:
    static ColorSelector* instance();
    void drop();
    int colorsCount() const {return colors.size();}
    void addColor(const QColor &color);

    void setColor(const QColor &color, int index);

    void removeColor(int index);

    QColor getColor();

    QVector<QColor> getColors() const {return colors;}

    void resetState();

    void freeColor(const QColor &color);

private:
    ColorSelector();
    ColorSelector(const ColorSelector &);
    ColorSelector& operator=(const ColorSelector &);

    static ColorSelector* m_Instance;

    QVector<QColor> colors;
};

#endif // COLORSELECTOR_H
