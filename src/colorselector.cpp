#include "colorselector.h"

#include <QtCore>

static QColor defaultColor = QColor(QRgb(0x00808080));

static QList<QColor> usedColors;

static uint colorsTable[32]={
    0x00000000,
    0x00ff6600,
    0x00008000,
    0x000000ff,
    0x00808080,
    0x00ff0000,
    0x0099cc00,
    0x00800080,
    0x00ffcc00, //240, 240, 192
    0x00ffff00,
    0x0000ff00,
    0x0000ffff, //0, 120, 0
    0x002923be,
    0x0084E16C,
    0x00d6ae52,
    0x009049f1,
    0x00f1bbe9,
    0x00ebb3a6,
    0x00db3c87,
    0x000c3e99,
    0x00245e0d,
    0x001c06b7,
    0x0047deb3,
    0x00124dc8,
    0x0043bb8b,
    0x00a61f03,
    0x005a7d09,
    0x0038251f,
    0x005dd4cb,
    0x00fc96f5,
    0x00453b13,
    0x000d890a
};

ColorSelector::ColorSelector(const QVariantList &list)
{
    if (list.isEmpty() || list.size() < COLORS_COUNT) {
        colors.resize(COLORS_COUNT);
        for (int i=0; i<colors.size() && i<COLORS_COUNT; ++i)
            colors[i] = QColor(QRgb(colorsTable[i]));
    }
    else {
        colors.resize(list.size());
        for (int i=0; i<list.size(); ++i) {
            QVariant v = list.at(i);
            colors[i] = v.value<QColor>();
        }
    }
}

void ColorSelector::addColor(const QColor &color)
{
    colors.append(color);
}

void ColorSelector::setColor(const QColor &color, int index)
{
    if (index>=0 && index<colors.size())
        colors[index] = color;
}

void ColorSelector::removeColor(int index)
{
    if (index>=0 && index<colors.size())
        colors.remove(index);
}

QColor ColorSelector::getColor()
{
    for (const QColor &c: colors) {
        if (!usedColors.contains(c)) {
            usedColors.append(c);
            return c;
        }
    }

    return defaultColor;
}

QColor ColorSelector::color(int index) const
{
    return colors.at(index);
}

QVariantList ColorSelector::getColors() const
{
    QVariantList vec;
    std::transform(colors.cbegin(), colors.cend(), std::back_inserter(vec), [](const QColor &color){
        QVariant v;
        v.setValue<QColor>(color);
        return v;
    });
    return vec;
}

void ColorSelector::resetState()
{
    usedColors.clear();
}

void ColorSelector::freeColor(const QColor &color)
{
    usedColors.removeAll(color);
}


