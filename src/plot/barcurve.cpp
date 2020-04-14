#include "barcurve.h"

#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "qwt_plot.h"
#include "qwt_text.h"
#include "qwt_legend_data.h"
#include "qwt_scale_map.h"
#include "pointlabel.h"

BarCurve::BarCurve(const QString &title, FileDescriptor *descriptor, int channelIndex) :  QwtPlotHistogram(title),
    Curve(title, descriptor, channelIndex)
{DD;

    setLegendIconSize(QSize(16,8));

    histogramdata = new HistogramData(this->channel->data());
    setData(histogramdata);
    setStyle(QwtPlotHistogram::Outline);
}

QPointF BarCurve::samplePoint(int point) const
{
    return histogramdata->samplePoint(point);
}

void BarCurve::attachTo(QwtPlot *plot)
{
    QwtPlotHistogram::attach(plot);
}

QString BarCurve::title() const
{
    return QwtPlotHistogram::title().text();
}

void BarCurve::setTitle(const QString &title)
{
    QwtPlotHistogram::setTitle(title);
}

QwtAxisId BarCurve::yAxis() const
{
    return QwtPlotHistogram::yAxis();
}

void BarCurve::setYAxis(QwtAxisId axis)
{
    QwtPlotHistogram::setYAxis(axis);
    foreach (PointLabel *l, labels)
        l->setYAxis(axis);
}

QwtAxisId BarCurve::xAxis() const
{
    return QwtPlotHistogram::xAxis();
}

void BarCurve::setXAxis(QwtAxisId axis)
{
    QwtPlotHistogram::setXAxis(axis);
    foreach (PointLabel *l, labels)
        l->setXAxis(axis);
}

QPen BarCurve::pen() const
{
    return QwtPlotHistogram::pen();
}

void BarCurve::setPen(const QPen &pen)
{
    QwtPlotHistogram::setPen(pen);
}

QList<QwtLegendData> BarCurve::legendData() const
{
    QList<QwtLegendData> result = QwtPlotHistogram::legendData();
    QwtLegendData &data = result[0];
    data.setValue(QwtLegendData::UserRole+3, pen().color());
    data.setValue(QwtLegendData::TitleRole, title());
    if (duplicate)
        data.setValue(QwtLegendData::UserRole+1, fileNumber);
    data.setValue(QwtLegendData::UserRole+2, highlighted);
    data.setValue(QwtLegendData::UserRole+4, fixed);

    return result;
}

void BarCurve::highlight()
{
    Curve::highlight();
    setZ(1000);
    plot()->updateLegend(this);
}

void BarCurve::resetHighlighting()
{
    Curve::resetHighlighting();
    setZ(20);
}


/** HistogramData implementation */

HistogramData::HistogramData(DataHolder *data) : data(data)
{
    // по данным оси х определяем тип октавы, не обращаясь к типу файла,
    // так как там все равно нет нужных сведений

    // если это третьоктава, то хслед/хпред = [1.23..1.3]
    int i=0;
    while (data->xValue(i) == 0.0) i++;

    if (data->samplesCount() < i+3) return;

    double step1 = data->xValue(i+1)/data->xValue(i);
    double step2 = data->xValue(i+2)/data->xValue(i+1);

    if (step1 >= 1.23 && step1 <= 1.3 &&
        step2 >= 1.23 && step2 <= 1.3) octaveType = Octave3;
    if (step1 >= 1.9 && step1 <= 2.1 &&
        step2 >= 1.9 && step2 <= 2.1) octaveType = Octave1;
}

QRectF HistogramData::boundingRect() const
{
    if ( d_boundingRect.width() < 0 ) {
        if (data->xValuesFormat() == DataHolder::XValuesUniform) {
            d_boundingRect.setLeft( data->xMin() - data->xStep()/2.0 );
            d_boundingRect.setRight( data->xMax() + data->xStep()/2.0 );
        }
        else {
            if (octaveType == Octave3) {
                d_boundingRect.setLeft( data->xMin() / pow(10.0, 0.05) );
                d_boundingRect.setRight( data->xMax() * pow(10.0, 0.05) );
            }
            else if (octaveType == Octave1) {
                d_boundingRect.setLeft( data->xMin() / pow(2.0, 0.5) );
                d_boundingRect.setRight( data->xMax() * pow(2.0, 0.5) );
            }
            else {
                d_boundingRect.setLeft( data->xMin() );
                d_boundingRect.setRight( data->xMax() );
            }
        }
        d_boundingRect.setTop( qMin(data->yMin(), 0.0) );
        d_boundingRect.setBottom( qMax(data->yMax(), 0.0) );
    }

    return d_boundingRect;
}

size_t HistogramData::size() const
{
    return data->samplesCount();
}

QwtIntervalSample HistogramData::sample(size_t i) const
{
    // если шаг по оси х постоянный, возвращаем (x/step/2, x+step/2, y)
    if (data->xValuesFormat() == DataHolder::XValuesUniform)
        return QwtIntervalSample(data->yValue(i),
                                 data->xValue(i)-data->xStep()/2.0,
                                 data->xValue(i)+data->xStep()/2.0);

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х

    // левая граница
    double left = data->xValue(i);
    if (i>0) left = sqrt(data->xValue(i) * data->xValue(i-1));
    else {
        if (octaveType == Octave3) {
            left = data->xValue(i) / pow(10.0, 0.05);
        }
        else if (octaveType == Octave1) {
            left = data->xValue(i) / pow(2.0, 0.5);
        }
    }

    // правая граница
    double right = data->xValue(i);
    if (i< (uint)(data->samplesCount()-1)) right = sqrt(data->xValue(i) * data->xValue(i+1));
    else {
        if (octaveType == Octave3) {
            right = data->xValue(i) * pow(10.0, 0.05);
        }
        else if (octaveType == Octave1) {
            right = data->xValue(i) * pow(2.0, 0.5);
        }
    }

    return QwtIntervalSample(data->yValue(i), left, right);
}

QPointF HistogramData::samplePoint(size_t i) const
{
    return QPointF(data->xValue(i), data->yValue(i));
}

double BarCurve::xMin() const
{
    // если шаг по оси х постоянный, возвращаем x-step/2
    if (histogramdata->data->xValuesFormat() == DataHolder::XValuesUniform)
        return histogramdata->data->xMin() - histogramdata->data->xStep()/2.0;

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х
    if (histogramdata->octaveType == HistogramData::Octave3) {
        return histogramdata->data->xMin() / pow(10.0, 0.05);
    }
    else if (histogramdata->octaveType == HistogramData::Octave1) {
        return histogramdata->data->xMin() / pow(2.0, 0.5);
    }

    return histogramdata->data->xMin();
}

double BarCurve::xMax() const
{
    // если шаг по оси х постоянный, возвращаем x-step/2
    if (histogramdata->data->xValuesFormat() == DataHolder::XValuesUniform)
        return histogramdata->data->xMax() + histogramdata->data->xStep()/2.0;

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х
    if (histogramdata->octaveType == HistogramData::Octave3) {
        return histogramdata->data->xMax() * pow(10.0, 0.05);
    }
    else if (histogramdata->octaveType == HistogramData::Octave1) {
        return histogramdata->data->xMax() * pow(2.0, 0.5);
    }

    return histogramdata->data->xMax();
}


int BarCurve::closest(const QPoint &pos, double *dist) const
{
    int index = -1;

    const size_t numSamples = channel->samplesCount();
    if ( numSamples <= 0 )
        return -1;

    const QwtScaleMap xMap = plot()->canvasMap( xAxis() );
    const QwtScaleMap yMap = plot()->canvasMap( yAxis() );

    int from = 0;
    int to = numSamples-1;
    evaluateScale(from, to, xMap);


    double dmin = 1.0e10;

    for ( int i = from; i <= to; i++ ) {
        const QPointF sample = samplePoint( i );

        const double cx = xMap.transform( sample.x() ) - pos.x();
        const double cy = yMap.transform( sample.y() ) - pos.y();

        const double f = cx*cx + cy*cy;
        if ( f < dmin ) {
            index = i;
            dmin = f;
        }
    }
    if ( dist )
        *dist = qSqrt( dmin );

    return index;
}
