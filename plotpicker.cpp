#include "plotpicker.h"

#include <qwt_picker_machine.h>
#include <qpainter.h>
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_scale_map.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_directpainter.h>

#include "qwt_plot_textlabel.h"

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
//    selectedObject = -1;
    selectedMarker = 0;

    setStateMachine(new QwtPickerDragPointMachine);

    setTrackerMode(QwtPicker::AlwaysOn);
    setRubberBandPen(QPen(QColor(60,60,60), 0.5, Qt::DashLine));

    connect(this,SIGNAL(appended(QPoint)),this,SLOT(pointAppended(QPoint)));
    connect(this,SIGNAL(moved(QPoint)),this,SLOT(pointMoved(QPoint)));
//    connect(this,SIGNAL(selected(QPointF)),this,SLOT(pointSelected(QPointF)));
}

void PlotPicker::setMode(Plot::InteractionMode mode)
{
    this->mode = mode;

//    switch (mode) {
//        case Plot::ScalingInteraction: setStateMachine(new QwtPickerClickPointMachine);
//            break;
//        case Plot::DataInteraction: setStateMachine(new QwtPickerDragPointMachine);
//            break;
//        case Plot::NoInteraction: setStateMachine(0);
//            selectedObject = -1;
//            selectedMarker = 0;
//            break;
//    }

//    if (mode == Plot::ScalingInteraction) {
//        resetHighLighting();
//    }
}

void PlotPicker::widgetKeyReleaseEvent(QKeyEvent *e)
{
    const int key = e->key();
   // if (d_selectedPoint == -1) return;

    if (key == Qt::Key_Left) {
        if (d_selectedPoint > 0) {
            highlightPoint(false);
            d_selectedPoint--;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Right) {
        if (d_selectedPoint >=0 && d_selectedPoint < int(d_selectedCurve->dfd->NumInd-1)) {
            highlightPoint(false);
            d_selectedPoint++;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Space) {
        QwtPlotTextLabel *titleItem = new QwtPlotTextLabel();
        //titleItem->setXAxis();
        titleItem->setText( QwtText("text") );
        titleItem->attach( plot );

        if (d_selectedCurve) {
            QPointF val = d_selectedCurve->sample(d_selectedPoint);
            Marker *m = d_selectedCurve->findMarker(d_selectedPoint);
            if (!m) {
                m = d_selectedCurve->addMarker(d_selectedPoint);
                m->attach(plot);
            }
            else {
                highlightPoint(false);
                switch (m->type) {
                    case 0: {
                        m->type = 1;
                        m->setLabel(QwtText(QString("%1\n%2").arg(val.x()).arg(val.y(),0,'f',2)));
                        break;
                    }
                    case 1: {
                        m->type = 2;
                        m->setLabel(QwtText(QString("%1").arg(val.y(),0,'f',2)));
                        break;
                    }
                    case 2: {//удаляем
                        d_selectedCurve->removeMarker(d_selectedPoint);
                        plot->replot();
                        break;
                    }
                    default: break;
                }
            }
        }
    }
}

void PlotPicker::resetHighLighting()
{
    highlightPoint(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;
}

Curve * PlotPicker::findClosestPoint(const QPoint &pos, int &index) const
{
    Curve *curve = 0;
    double dist = 10e10;

    const QwtPlotItemList& itmList = plot->itemList();


    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
            Curve *c = static_cast<Curve *>( *it );

            double d;
            int idx = c->closestPoint( pos, &d );
            if ( d < dist ) {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    if (dist < 10)
        return curve;

    return 0;
}

Marker * PlotPicker::findMarker()
{
    Marker *m = 0;
    double dist = 10e10;


    Plot *p = static_cast<Plot*>(plot);
    if (p) {
        QList<Curve*> curves = p->curves();
        foreach(Curve *curve, curves) {
            const QPointF pos = QPointF(plot->invTransform(curve->xAxis(),trackerPosition().x()),
                                        plot->invTransform(curve->yAxis(),trackerPosition().y()));
            foreach(Marker *marker, curve->markers) {
                double d = QVector2D(marker->text->value()).distanceToPoint(QVector2D(pos));
                if (d < dist) {
                    dist = d;
                    m = marker;
                }
            }
        }
    }

    if (dist < 1)
        return m;

    return 0;
}

void PlotPicker::pointAppended(const QPoint &pos)
{
    resetHighLighting();

    Curve *curve = NULL;
    int index = -1;
    selectedMarker = 0;

    if ((curve = findClosestPoint(pos, index))) {
        d_selectedCurve = curve;
        d_selectedPoint = index;

        selectedMarker = findMarker();
        if (selectedMarker)  {
            //d_selectedCurve = curve;
            //d_selectedPoint = index;
        }
        else
            highlightPoint( true );
    }
}

void PlotPicker::pointMoved(const QPoint &pos)
{
    if (mode != Plot::DataInteraction) return;

    if (selectedMarker) {
        QwtPlotCanvas *canvas = qobject_cast<QwtPlotCanvas *>(plot->canvas());
        canvas->setCursor (Qt::PointingHandCursor);

//        double newX = plot->invTransform(d_selectedCurve->xAxis(), pos.x());
//        double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());


//        double diffX = newX - selectedMarker->value().x();
//        double diffY = newY - d_selectedCurve->dfd->channels.at(d_selectedCurve->channel)->yValues[d_selectedPoint];

        selectedMarker->moveTextTo(QPointF(plot->invTransform(d_selectedCurve->xAxis(), pos.x()),
                                 plot->invTransform(d_selectedCurve->yAxis(), pos.y())));
        return;
    }
    //canvas->setCursor (Qt::ArrowCursor);

    if (!d_selectedCurve)
        return;

    double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());

    double diff = newY - d_selectedCurve->dfd->channels.at(d_selectedCurve->channel)->yValues[d_selectedPoint];

    d_selectedCurve->dfd->channels.at(d_selectedCurve->channel)->yValues[d_selectedPoint] += diff;
    d_selectedCurve->dfd->rawFileChanged = true;

    highlightPoint(true);
}

// Hightlight the selected point
void PlotPicker::highlightPoint( bool showIt )
{
    if (showIt) {
        if (!d_selectedCurve)
            return;
        QPointF val = d_selectedCurve->sample(d_selectedPoint);
        Marker *m = d_selectedCurve->findMarker(d_selectedPoint);
        if (!marker) {
            marker = new QwtPlotMarker();
            marker->setLineStyle(QwtPlotMarker::NoLine);
            if (!m)
                marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                            Qt::gray,
                                            d_selectedCurve->pen().color(),
                                            QSize(8, 8))
                              );
            else
                marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                            Qt::black,
                                            d_selectedCurve->pen().color(),
                                            QSize(8, 8))
                              );
            marker->setLabelAlignment(Qt::AlignTop);
            marker->attach(plot);

        }
        marker->setValue(val);
        if (marker->label()==QwtText()) {
            if (!m)
                marker->setLabel(QwtText(QString("%1").arg(val.x())));
            else
                marker->setLabel(QwtText());
        }
    }
    else {
        delete marker;
        marker = 0;
    }


    QwtPlotCanvas *plotCanvas = qobject_cast<QwtPlotCanvas *>(plot->canvas());
    plotCanvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    plot->replot();
    plotCanvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, false );
}




QwtText PlotPicker::trackerTextF(const QPointF &pos) const
{
    QString text;
    text.sprintf( "%.2f, %.2f", pos.x(), pos.y());

    return QwtText( text );
}
