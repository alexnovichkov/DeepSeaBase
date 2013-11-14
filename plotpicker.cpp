#include "plotpicker.h"

#include <qwt_picker_machine.h>
#include <qpainter.h>
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_scale_map.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_directpainter.h>
#include <qwt_plot_marker.h>

#include <QtDebug>
#include <QtWidgets>
#include "curve.h"
#include "dfdfiledescriptor.h"

PlotPicker::PlotPicker(QWidget *canvas) :
    QwtPlotPicker(canvas),
    d_selectedCurve( NULL ),
    d_selectedPoint( -1 )
{
    plot = qobject_cast<QwtPlotCanvas*>(canvas)->plot();

    marker = 0;
    mode = Plot::ScalingInteraction;

    setStateMachine(new QwtPickerDragPointMachine);
    setTrackerMode(QwtPicker::AlwaysOn);
    setRubberBandPen(QPen(QColor(60,60,60), 0.5, Qt::DashLine));

    connect(this,SIGNAL(appended(QPoint)),this,SLOT(pointAppended(QPoint)));
    connect(this,SIGNAL(moved(QPoint)),this,SLOT(pointMoved(QPoint)));
}

void PlotPicker::setMode(Plot::InteractionMode mode)
{
    this->mode = mode;

    if (mode == Plot::ScalingInteraction) {
        resetHighLighting();
    }
}

void PlotPicker::resetHighLighting()
{
    highlightPoint(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;
}

void PlotPicker::pointAppended(const QPoint &pos)
{
    if (mode != Plot::DataInteraction) return;

    QwtPlotCurve *curve = NULL;
    double dist = 10e10;
    int index = -1;

    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
            QwtPlotCurve *c = static_cast<QwtPlotCurve *>( *it );

            double d;
            int idx = c->closestPoint( pos, &d );
            if ( d < dist ) {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    resetHighLighting();

    if ( curve && dist < 10 ) {// 10 pixels tolerance
        d_selectedCurve = curve;
        d_selectedPoint = index;
        highlightPoint( true );
    }
}

void PlotPicker::pointMoved(const QPoint &pos)
{
    if (mode != Plot::DataInteraction) return;

    if (!d_selectedCurve)
        return;

    Curve *curve = dynamic_cast<Curve*>(d_selectedCurve);
    if (!curve) return;


    double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());

    double diff = newY - curve->dfd->channels.at(curve->channel)->yValues[d_selectedPoint];

    if (qApp->keyboardModifiers() & Qt::ControlModifier) {
//        for (int i=0; i<curve->dfd->channels.at(curve->channel)->NumInd; ++i)
//            curve->dfd->channels.at(curve->channel)->yValues[i] += diff;
    }
    else {
        curve->dfd->channels.at(curve->channel)->yValues[d_selectedPoint] += diff;
    }

    highlightPoint( true );
}

// Hightlight the selected point
void PlotPicker::highlightPoint( bool showIt )
{
    delete marker;
    marker = 0;

    if (showIt) {
        if (!d_selectedCurve)
            return;

        marker = new QwtPlotMarker();
        marker->setLineStyle(QwtPlotMarker::NoLine);
        marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                        Qt::gray,
                                        d_selectedCurve->pen().color(),
                                        QSize(8, 8))
                          );
        marker->attach(plot);

        marker->setValue(d_selectedCurve->sample(d_selectedPoint));
    }


    QwtPlotCanvas *plotCanvas = qobject_cast<QwtPlotCanvas *>(plot->canvas());
    plotCanvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    plot->replot();
    plotCanvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, false );
}


