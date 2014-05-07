#include "curve.h"
#include "qwt_symbol.h"

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

Curve::~Curve()
{
    for (int i=0; i<markers.size(); ++i) {
        markers[i]->detach();
    }
    qDeleteAll(markers);
    markers.clear();
}

void Curve::setRawSamples(double x0, double xStep, const double *yData, int size)
{
    setData(new DfdData(x0, xStep, yData, size));
}

Marker *Curve::addMarker(int index)
{
    foreach(Marker *m, markers) {
        if (m->d_selectedPoint == index) return m;
    }



    Marker *m = new Marker(index);
    //m->symbol->setLineStyle(QwtPlotMarker::NoLine);
    m->text->setLineStyle(QwtPlotMarker::NoLine);
    m->text->setLabelAlignment(Qt::AlignTop);
    m->text->setSpacing(m->text->spacing()+4);
    m->text->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                               Qt::gray,
                               this->pen().color(),
                               QSize(8, 8))
                 );
    QwtText text = QwtText(QString("%1").arg(sample(index).x()));

    markers << m;
    m->setValue(sample(index));
    m->setLabel(text);
    return m;
}

void Curve::removeMarker(int index)
{
    for (int i = markers.size()-1; i >= 0; --i) {
        Marker *m = markers[i];
        if (m->d_selectedPoint == index) {
            m->detach();
            delete m;
            markers.removeAt(i);
        }
    }
}

Marker *Curve::findMarker(int index)
{
    foreach(Marker *m, markers) {
        if (m->d_selectedPoint == index) return m;
    }

    return 0;
}

Marker::Marker(int point)
    : /*symbol(new QwtPlotMarker()),*/ text(new QwtPlotMarker()), d_selectedPoint(point),  type(0)
{


}

void Marker::setValue(const QPointF &value)
{
    //symbol->setValue(value);
    text->setValue(value);
}

void Marker::moveTextTo(const QPointF &value)
{
    text->setValue(value);
}

void Marker::detach()
{
    //symbol->detach();
    text->detach();
}

void Marker::attach(QwtPlot *plot)
{
    //symbol->attach(plot);
    text->attach(plot);
}

void Marker::setLabel(const QwtText &label)
{
    text->setLabel(label);
}
