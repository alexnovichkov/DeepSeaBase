#include "colormapfactory.h"
#include <qwt_interval.h>
#include "logging.h"

class LinearColorMap: public QwtLinearColorMap
{
public:
    LinearColorMap(): QwtLinearColorMap(Qt::black, Qt::white)
    {DDD;
        setFormat(QwtColorMap::RGB);
    }
};

class LinearInverseColorMap: public QwtLinearColorMap
{
public:
    LinearInverseColorMap(): QwtLinearColorMap(Qt::white, Qt::black)
    {DDD;
        setFormat(QwtColorMap::RGB);
    }
};

class SymmetricColorMap: public QwtLinearColorMap
{
public:
    SymmetricColorMap(): QwtLinearColorMap(Qt::blue, Qt::red)
    {DDD;
        setFormat(QwtColorMap::RGB);
        addColorStop( 0.5, Qt::white );
    }
};

class RGBColorMap: public QwtLinearColorMap
{
public:
    RGBColorMap():  QwtLinearColorMap(QColor(2,0,174), QColor(217,53,43))
    {DDD;
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
    {DDD;
        setHueInterval(0, 300);
    }
};

class SymmetricColorGradient : public QCPColorGradient
{
public:
    SymmetricColorGradient()
    {
        setColorStopAt(0, Qt::blue);
        setColorStopAt(0.5, Qt::white);
        setColorStopAt(1, Qt::red);
    }
};

class RGBColorGradient : public QCPColorGradient
{
public:
    RGBColorGradient()
    {
        setColorStopAt( 0,      QColor(2,0,174));
        setColorStopAt( 0.0625, QColor(0,30,255));
        setColorStopAt( 0.125,  QColor(12,126,251));
        setColorStopAt( 0.1875, QColor(2,165,242));
        setColorStopAt( 0.25,   QColor(0,239,247));
        setColorStopAt( 0.3125, QColor(128,226,250));
        setColorStopAt( 0.375,  QColor(0,187,126));
        setColorStopAt( 0.4375, QColor(0,187,0));
        setColorStopAt( 0.5,    QColor(26,246,36));
        setColorStopAt( 0.5625, QColor(183,255,0));
        setColorStopAt( 0.625,  QColor(254,255,0));
        setColorStopAt( 0.6875, QColor(254,213,86));
        setColorStopAt( 0.75,   QColor(222,145,27));
        setColorStopAt( 0.8125, QColor(252,165,0));
        setColorStopAt( 0.875,  QColor(254,110,0));
        setColorStopAt( 1,      QColor(217,53,43));
    }
};

class SteppingColorGradient : public QCPColorGradient
{
public:
    SteppingColorGradient()
    {
        for (int i=0; i<13; ++i)
            setColorStopAt( double(i)/12,   QColor(colors[i]));
        setLevelCount(13);
    }
private:
    QRgb colors[13] = {
        0xffffffff,
        0xffaaC6C3,
        0xff84A2FF,
        0xff4265FF,
        0xff00FFFF,
        0xff29FF52,
        0xff00AA00,
        0xff005500,
        0xffFFFF00,
        0xffFF7D00,
        0xffFF2829,
        0xffAD0000,
        0xffff29ff
    };
};

class SteppingColorMap: public QwtColorMap
{
public:
    SteppingColorMap() : QwtColorMap(QwtColorMap::RGB)
    {DDD;
    }
    QRgb rgb(const QwtInterval &interval, double value) const override
    {DDD;
        const double width = interval.width();
        if (width <= 0.0)
            return 0u;

        if (value < interval.minValue()) return 0xffffffff;
        if (value > interval.maxValue()) return 0xffff29ff;

        const int ratio = int(( value - interval.minValue() ) / width * 13.0);
        //qDebug()<<interval<<value<<ratio<<colors[ratio];
        return colors[ratio];
    }
private:
    QRgb colors[13] = {
        0xffffffff,
        0xffaaC6C3,
        0xff84A2FF,
        0xff4265FF,
        0xff00FFFF,
        0xff29FF52,
        0xff00AA00,
        0xff005500,
        0xffFFFF00,
        0xffFF7D00,
        0xffFF2829,
        0xffAD0000,
        0xffff29ff
    };
};

QStringList ColorMapFactory::names()
{DDD;
    QStringList l;
    l << "RGB";
    l << "Серая";
    l << "Серая обратная";
    l << "Симметричная";
    l << "Hue";
    l << "Ступенчатая";
    return l;
}

QwtColorMap *ColorMapFactory::map(int index)
{DDD;
    switch (index) {
    case RGBMap: return new RGBColorMap();
    case GreyMap : return new LinearColorMap();
    case GreyInverseMap : return new LinearInverseColorMap();
    case SymmetricMap : return new SymmetricColorMap();
    case HueMap : return new HueColorMap();
    case SteppingMap : return new SteppingColorMap();
    default: ;
    }
    return new RGBColorMap();
}

QCPColorGradient ColorMapFactory::gradient(int index)
{
    switch (index) {
    case RGBMap: return RGBColorGradient();
    case GreyMap : return QCPColorGradient(QCPColorGradient::gpGrayscale);
    case GreyInverseMap : return QCPColorGradient(QCPColorGradient::gpGrayscale).inverted();
    case SymmetricMap : return SymmetricColorGradient();
    case HueMap : return QCPColorGradient(QCPColorGradient::gpHues);
    case SteppingMap : return SteppingColorGradient();
    default: ;
    }
    return RGBColorGradient();
}
