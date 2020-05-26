#include "colormapfactory.h"

class LinearColorMap: public QwtLinearColorMap
{
public:
    LinearColorMap(): QwtLinearColorMap(Qt::black, Qt::white)
    {
        setFormat(QwtColorMap::RGB);
    }
};

class SymmetricColorMap: public QwtLinearColorMap
{
public:
    SymmetricColorMap(): QwtLinearColorMap(Qt::blue, Qt::red)
    {
        setFormat(QwtColorMap::RGB);
        addColorStop( 0.5, Qt::white );
    }
};

class RGBColorMap: public QwtLinearColorMap
{
public:
    RGBColorMap():  QwtLinearColorMap(QColor(2,0,174), QColor(217,53,43))
    {
        setFormat(QwtColorMap::RGB);
        addColorStop( 0.0625, QColor(0,30,255));
        addColorStop( 0.125,  QColor(12,126,251));
        addColorStop( 0.1875, QColor(2,165,242));
        addColorStop( 0.25,   QColor(0,239,247));
        addColorStop( 0.3125, QColor(128,226,250));
        addColorStop( 0.375,  QColor(0,187,126));
        addColorStop( 0.4375, QColor(0,187,0));
        addColorStop( 0.5,    QColor(26,246,36));
        addColorStop( 0.5625, QColor(183,255,0));
        addColorStop( 0.625,  QColor(254,255,0));
        addColorStop( 0.6875, QColor(254,213,86));
        addColorStop( 0.75,   QColor(222,145,27));
        addColorStop( 0.8125, QColor(252,165,0));
        addColorStop( 0.875,  QColor(254,110,0));
    }
};

class HueColorMap: public QwtHueColorMap
{
public:
    HueColorMap() : QwtHueColorMap(QwtColorMap::RGB)
    {
        setHueInterval(0, 300);
    }
};

QStringList ColorMapFactory::names()
{
    QStringList l;
    l << "RGB";
    l << "Серая";
    l << "Симметричная";
    l << "Hue";
    return l;
}

QwtColorMap *ColorMapFactory::map(int index)
{
    switch (index) {
        case RGBMap: return new RGBColorMap();
        case GreyMap : return new LinearColorMap();
        case SymmetricMap : return new SymmetricColorMap();
        case HueMap : return new HueColorMap();
        default: ;
    }
    return new RGBColorMap();
}
