#include "colorselector.h"

#include <QtCore>
#include "logging.h"
#include "settings.h"

inline uint qHash(const QColor &key, uint seed)
{
    return qHash(key.rgba(), seed);
}

static QColor defaultColor = QColor(QRgb(0x00808080));

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

ColorSelector::ColorSelector()
{DD;
    QVariantList colors = se->getSetting("colors").toList();
    if (colors.isEmpty() || colors.size() < COLORS_COUNT) {
        //список пустой - создаем новый список и сохраняем его в настройках
        colors.clear();
        for (int i=0; i<colors.size() && i<COLORS_COUNT; ++i)
            colors << QColor(QRgb(colorsTable[i]));

        se->setSetting("colors", colors);
    }

    usedColors.fill(false, colors.size());
}

//void ColorSelector::addColor(const QColor &color)
//{DD;
//    colors.append(color);
//}

void ColorSelector::setColor(QColor color, int index)
{DD;
    QVariantList colors = se->getSetting("colors").toList();
    if (index>=0 && index < colors.size()) {
        colors[index] = color;
        se->setSetting("colors", colors);
    }
}

//void ColorSelector::removeColor(int index)
//{DD;
//    if (index>=0 && index<colors.size())
//        colors.remove(index);
//}

QColor ColorSelector::getColor()
{DD;
    QVariantList colors = se->getSetting("colors").toList();
    for (int i=0; i<usedColors.size(); i++) {
        bool &used = usedColors[i];
        if (!used) {
            used = true;
            return colors[i].value<QColor>();
        }
    }

    return defaultColor;
}

QColor ColorSelector::color(int index) const
{DD;
    QVariantList colors = se->getSetting("colors").toList();
    return colors.value(index).value<QColor>();
}

QVariantList ColorSelector::getColors() const
{DD;
    QVariantList vec = se->getSetting("colors").toList();

    return vec;
}

//void ColorSelector::resetState()
//{DD;
//    usedColors.fill(false, COLORS_COUNT);
//}

void ColorSelector::freeColor(const QColor &color)
{DD;
    QVariantList vec = se->getSetting("colors").toList();
    for (int i=0; i<usedColors.size(); ++i) {
        if (vec.value(i).value<QColor>() == color) {
            usedColors[i] = false;
            break;
        }
    }
}


