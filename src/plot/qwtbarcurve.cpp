#include "qwtbarcurve.h"

#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "qwt_plot.h"
#include "qwt_text.h"
#include "qwt_legend_data.h"
#include "qwt_scale_map.h"
#include "pointlabel.h"
#include "plot.h"
#include "qwtplotimpl.h"

QwtBarCurve::QwtBarCurve(const QString &title, Channel *channel) :  QwtPlotHistogram(title),
    Curve(title, channel)
{DDD;
    type = Type::Line;
    setLegendIconSize(QSize(16,8));

    histogramdata = new HistogramData(this->channel->data());
    setData(histogramdata);
    setStyle(QwtPlotHistogram::Outline);
}

SamplePoint QwtBarCurve::samplePoint(SelectedPoint point) const
{DDD;
    return histogramdata->samplePoint(point);
}

void QwtBarCurve::attachTo(QwtPlot *plot)
{DDD;
    QwtPlotHistogram::attach(plot);
}

QString QwtBarCurve::title() const
{DDD;
    return QwtPlotHistogram::title().text();
}

void QwtBarCurve::setTitle(const QString &title)
{DDD;
    QwtPlotHistogram::setTitle(title);
}

Enums::AxisType QwtBarCurve::yAxis() const
{DDD;
    return toAxisType(QwtPlotHistogram::yAxis());
}

void QwtBarCurve::setYAxis(Enums::AxisType axis)
{DDD;
    QwtPlotHistogram::setYAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setYAxis(toQwtAxisType(axis));
}

Enums::AxisType QwtBarCurve::xAxis() const
{DDD;
    return toAxisType(QwtPlotHistogram::xAxis());
}

void QwtBarCurve::setXAxis(Enums::AxisType axis)
{DDD;
    QwtPlotHistogram::setXAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setXAxis(toQwtAxisType(axis));
}

QPen QwtBarCurve::pen() const
{DDD;
    return QwtPlotHistogram::pen();
}

void QwtBarCurve::updatePen()
{DDD;
    auto p = oldPen;
    if (selected()) p.setWidth(2);
    QwtPlotHistogram::setPen(p);
}

QList<QwtLegendData> QwtBarCurve::legendData() const
{DDD;
    QList<QwtLegendData> result = QwtPlotHistogram::legendData();
    QwtLegendData &data = result[0];
    data.setValues(commonLegendData());
    return result;
}

void QwtBarCurve::updateSelection(SelectedPoint point)
{DDD;
    Curve::updateSelection(point);
    if (selected()) setZ(1000);
    else setZ(20);
    plot()->updateLegend(this);
}

/** HistogramData implementation */

HistogramData::HistogramData(DataHolder *data) : data(data)
{DDD;
    // по данным оси х определяем тип октавы, не обращаясь к типу файла,
    // так как там все равно нет нужных сведений

    // если это третьоктава, то хслед/хпред = [1.23..1.3]
    int i=0;
    while (data->xValue(i) <= 1.0) i++;

    if (data->samplesCount() < i+3) return;

    double step1 = data->xValue(i+1)/data->xValue(i);
    double step2 = data->xValue(i+2)/data->xValue(i+1);

    if (step1 >= 1.9 && step1 <= 2.1 &&
        step2 >= 1.9 && step2 <= 2.1) octaveType = Octave1;
    if (step1 >= 1.39 && step1 <= 1.43 &&
        step2 >= 1.39 && step2 <= 1.43) octaveType = Octave2;
    if (step1 >= 1.23 && step1 <= 1.3 &&
        step2 >= 1.23 && step2 <= 1.3) octaveType = Octave3;
    if (step1 >= 1.11 && step1 <= 1.14 &&
        step2 >= 1.11 && step2 <= 1.14) octaveType = Octave6;
    if (step1 >= 1.05 && step1 <= 1.07 &&
        step2 >= 1.05 && step2 <= 1.07) octaveType = Octave12;
    if (step1 >= 1.02 && step1 <= 1.04 &&
        step2 >= 1.02 && step2 <= 1.04) octaveType = Octave24;

    switch (octaveType) {
        case Octave1: factor = pow(10.0, 0.15); break;
        case Octave2: factor = pow(10.0, 0.075); break;
        case Octave3: factor = pow(10.0, 0.05); break;
        case Octave6: factor = pow(10.0, 0.025); break;
        case Octave12: factor = pow(10.0, 0.0125); break;
        case Octave24: factor = pow(10.0, 0.00625); break;
        default: break;
    }
}

QRectF HistogramData::boundingRect() const
{DDD;
    QRectF d_boundingRect;
    if ( d_boundingRect.width() < 0 ) {
        if (data->xValuesFormat() == DataHolder::XValuesUniform) {
            d_boundingRect.setLeft( data->xMin() - data->xStep()/2.0 );
            d_boundingRect.setRight( data->xMax() + data->xStep()/2.0 );
        }
        else {
            d_boundingRect.setLeft( data->xMin() / factor );
            d_boundingRect.setRight( data->xMax() * factor );
        }
        d_boundingRect.setTop( qMin(data->yMin(), 0.0) );
        d_boundingRect.setBottom( qMax(data->yMax(), 0.0) );
    }

    return d_boundingRect;
}

size_t HistogramData::size() const
{DDD;
    return data->samplesCount();
}

QwtIntervalSample HistogramData::sample(size_t i) const
{DDD;
    // если шаг по оси х постоянный, возвращаем (x/step/2, x+step/2, y)
    if (data->xValuesFormat() == DataHolder::XValuesUniform)
        return QwtIntervalSample(data->yValue(i),
                                 data->xValue(i)-data->xStep()/2.0,
                                 data->xValue(i)+data->xStep()/2.0);

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х

    // левая граница
    double left = data->xValue(i);
    if (i>0)
        left = sqrt(data->xValue(i) * data->xValue(i-1));
    else
        left = data->xValue(i) / factor;

    // правая граница
    double right = data->xValue(i);
    if (i< (uint)(data->samplesCount()-1)) right = sqrt(data->xValue(i) * data->xValue(i+1));
    else
        right = data->xValue(i) * factor;

    return QwtIntervalSample(data->yValue(i), left, right);
}

SamplePoint HistogramData::samplePoint(SelectedPoint point) const
{DDD;
    return {data->xValue(point.x), data->yValue(point.x, point.z), qQNaN()};
}

double QwtBarCurve::xMin() const
{DDD;
    // если шаг по оси х постоянный, возвращаем x-step/2
    if (histogramdata->data->xValuesFormat() == DataHolder::XValuesUniform)
        return histogramdata->data->xMin() - histogramdata->data->xStep()/2.0;

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х
    if (histogramdata->octaveType == HistogramData::OctaveUnknown)
        return histogramdata->data->xMin();

    return histogramdata->data->xMin() / histogramdata->factor;
}

double QwtBarCurve::xMax() const
{DDD;
    // если шаг по оси х постоянный, возвращаем x-step/2
    if (histogramdata->data->xValuesFormat() == DataHolder::XValuesUniform)
        return histogramdata->data->xMax() + histogramdata->data->xStep()/2.0;

    // шаг по оси х не постоянный -> вероятнее всего октава или третьоктава.
    // xmin и xmax будут зависеть от значения х
    if (histogramdata->octaveType == HistogramData::OctaveUnknown)
        return histogramdata->data->xMax();

    return histogramdata->data->xMax() * histogramdata->factor;
}


SelectedPoint QwtBarCurve::closest(const QPoint &pos, double *dist1, double *dist2) const
{DDD;
    int index = -1;

    const size_t numSamples = channel->data()->samplesCount();
    if ( numSamples <= 0 )
        return {-1, -1};

    auto range = m_plot->plotRange(xAxis());

    int from = 0;
    int to = numSamples-1;
    evaluateScale(from, to, range.min, range.max);

    double dminx = qInf();
    double dminy = qInf();
    double dmin = qInf();

    for ( int i = from; i <= to; i++ ) {
        const auto sample = samplePoint( {i,0} );

        const double cx = qAbs(m_plot->plotToScreenCoordinates(xAxis(), sample.x ) - pos.x());
        const double cy = qAbs(m_plot->plotToScreenCoordinates(yAxis(), sample.y ) - pos.y());

        const double f = cx*cx + cy*cy;
        if ( f < dmin ) {
            index = i;
            dmin = f;
            dminx = cx;
            dminy = cy;
        }
    }
    if ( dist1 ) *dist1 = dminx;
    if ( dist2 ) *dist2 = dminy;

    return {index, 0};
}
