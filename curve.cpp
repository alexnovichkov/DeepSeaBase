#include "curve.h"

class DfdData: public QwtSeriesData<QPointF>
{
public:
    DfdData(double x0, double xStep, const double *y, size_t size) :
        d_x0(x0), d_xStep(xStep), d_y(y), d_size(size)
    {}

    virtual QRectF boundingRect() const
    {
        if ( d_boundingRect.width() < 0 )
            d_boundingRect = qwtBoundingRect( *this );

        return d_boundingRect;
    }

    virtual size_t size() const
    {
        return d_size;
    }

    virtual QPointF sample( size_t i ) const
    {
        return QPointF( d_x0 + i * d_xStep, d_y[int( i )] );
    }
private:
    double d_x0;
    double d_xStep;
    const double *d_y;
    size_t d_size;
};

Curve::Curve(const QString &title, DfdFileDescriptor *dfd, int channel):
    QwtPlotCurve(title), dfd(dfd), channel(channel)
{
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
}

Curve::Curve(const QString &title) :
    QwtPlotCurve(title), dfd(0), channel(-1)
{
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
}

Curve::Curve(const QwtText &title) :
    QwtPlotCurve(title), dfd(0), channel(-1)
{
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
}

void Curve::setRawSamples(double x0, double xStep, const double *yData, int size)
{
    setData(new DfdData(x0, xStep, yData, size));
}

