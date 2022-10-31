#include "colormapfactory.h"
#include "logging.h"

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
