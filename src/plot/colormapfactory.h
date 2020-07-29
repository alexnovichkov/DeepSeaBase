#ifndef COLORMAPFACTORY_H
#define COLORMAPFACTORY_H

#include <qwt_color_map.h>

class ColorMapFactory
{
public:
    enum ColorMap {
        RGBMap = 0,
        GreyMap = 1,
        GreyInverseMap = 2,
        SymmetricMap = 3,
        HueMap = 4,
        SteppingMap = 5
    };

    static QStringList names();

    static QwtColorMap *map(int index);
};

#endif // COLORMAPFACTORY_H
