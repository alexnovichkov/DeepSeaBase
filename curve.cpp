#include "curve.h"
#include "qwt_symbol.h"
#include "pointlabel.h"

#include "filedescriptor.h"

class DfdData: public QwtSeriesData<QPointF>
{
public:
    DfdData(double x0, double xStep, const double *y, size_t size) :
        d_x0(x0), d_xStep(xStep), d_y(y), d_x(0), d_size(size)
    {}
    DfdData(const double *x, const double *y, size_t size) :
        d_x0(x[0]), d_xStep(x[1]-x[0]), d_y(y), d_x(x), d_size(size)
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
        if (d_x) return QPointF(d_x[int(i)], d_y[int(i)]);
        return QPointF( d_x0 + i * d_xStep, d_y[int( i )] );
    }
private:
    double d_x0;
    double d_xStep;
    const double *d_y;
    const double *d_x;
    size_t d_size;
};

Curve::Curve(const QString &title, FileDescriptor *descriptor, int channelIndex):
    QwtPlotCurve(title), descriptor(descriptor), channelIndex(channelIndex)
{
    channel = descriptor->channel(channelIndex);
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
    if (channel->xValues().isEmpty())
        setRawSamples(channel->xBegin(), descriptor->xStep(), channel->yValues().data(), channel->samplesCount());
    else
        setRawSamples(channel->xValues().data(), channel->yValues().data(), channel->samplesCount());
}

Curve::Curve(const QString &title) :
    QwtPlotCurve(title), descriptor(0), channelIndex(-1), channel(0)
{
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
}

Curve::Curve(const QwtText &title) :
    QwtPlotCurve(title), descriptor(0), channelIndex(-1), channel(0)
{
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setRenderHint(QwtPlotItem::RenderAntialiased);
}

Curve::~Curve()
{
    foreach(PointLabel *l, labels) l->detach();
    qDeleteAll(labels);
}

void Curve::setRawSamples(double x0, double xStep, const double *yData, int size)
{
    setData(new DfdData(x0, xStep, yData, size));
}

void Curve::setRawSamples(const double *xData, const double *yData, int size)
{
    setData(new DfdData(xData, yData, size));
}

void Curve::addLabel(PointLabel *label)
{
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{
    if (labels.contains(label)) {
        labels.removeAll(label);
        label->detach();
        delete label;
    }
}

PointLabel *Curve::findLabel(const QPoint &pos, int yAxis)
{
    foreach (PointLabel *l, labels)
        if (l->contains(pos, yAxis))
            return l;

    return 0;
}

PointLabel *Curve::findLabel(const int point)
{
    foreach (PointLabel *l, labels)
        if (l->point() == point)
            return l;

    return 0;
}
