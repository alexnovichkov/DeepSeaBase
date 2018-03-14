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

#include "qwt_plot_textlabel.h"

#include <QtDebug>
#include <QtWidgets>
#include "curve.h"
#include "dfdfiledescriptor.h"
#include "pointlabel.h"
#include "logging.h"

PlotPicker::PlotPicker(QWidget *canvas) :
    QwtPlotPicker(canvas),
    d_selectedCurve( NULL ),
    d_selectedPoint( -1 )
{DD;
    plot = qobject_cast<QwtPlotCanvas*>(canvas)->plot();
    _showHarmonics = false;

    marker = 0;
    mode = Plot::ScalingInteraction;
    selectedLabel = 0;
    labelAdded=false;

    setStateMachine(new QwtPickerDragPointMachine);

    setTrackerMode(QwtPicker::AlwaysOn);
    setRubberBandPen(QPen(QColor(60,60,60), 0.5, Qt::DashLine));

    connect(this,SIGNAL(appended(QPoint)),this,SLOT(pointAppended(QPoint)));
    connect(this,SIGNAL(moved(QPoint)),this,SLOT(pointMoved(QPoint)));
}

PlotPicker::~PlotPicker()
{DD;
    foreach (QwtPlotMarker *d, _harmonics) {
        delete d;
    }
}

void PlotPicker::setMode(Plot::InteractionMode mode)
{DD;
    this->mode = mode;
}

void PlotPicker::widgetKeyReleaseEvent(QKeyEvent *e)
{DD;
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
        if (d_selectedPoint >=0 && d_selectedPoint < int(d_selectedCurve->channel->samplesCount()-1)) {
            highlightPoint(false);
            d_selectedPoint++;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Space) {
        if (d_selectedCurve) {
            QPointF val = d_selectedCurve->sample(d_selectedPoint);

            PointLabel *label = d_selectedCurve->findLabel(d_selectedPoint);

            if (!label) {
                label = new PointLabel(plot);
                label->setPoint(d_selectedPoint);
                label->setOrigin(val);
                label->setYAxis(d_selectedCurve->yAxis());
                d_selectedCurve->addLabel(label);

                label->attach(plot);
                plot->replot();
                selectedLabel = label;
                labelAdded = true;
            }
            else {
                selectedLabel = label;
                selectedLabel->setSelected(true);
                selectedLabel->cycleMode();
            }
            emit labelSelected(true);
        }
        else if (selectedLabel) {
            selectedLabel->cycleMode();
        }

    }
    else if (key == Qt::Key_C) {
        if (selectedLabel) {
            selectedLabel->cycleMode();
            plot->replot();
        }
    }
    else if (key == Qt::Key_Delete) {
        if (selectedLabel) {
            const QwtPlotItemList& itmList = plot->itemList();
            for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
                if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
                    Curve *c = static_cast<Curve *>( *it );
                    c->removeLabel(selectedLabel);
                }
            }
            selectedLabel = 0;
            emit labelSelected(false);
            labelAdded = false;
            plot->replot();
        }
    }
    else if (key == Qt::Key_H) {
        static_cast<Plot*>(plot)->switchLabelsVisibility();
    }
}

void PlotPicker::resetHighLighting()
{DD;
    highlightPoint(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;

    Plot *p = static_cast<Plot*>(plot);
    if (p) {
        foreach(Curve *c, p->graphs) {
            c->setPen(c->oldPen);
            c->setZ(20);
            foreach(PointLabel *label, c->labels)
                if (label) label->setSelected(false);
        }
    }

    selectedLabel = 0;
    emit labelSelected(false);
    labelAdded=false;
}

Curve * PlotPicker::findClosestPoint(const QPoint &pos, int &index) const
{DD;
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

/**
 * @brief PlotPicker::findLabel
 * @return the first PointLabel whose boundingRect contains
 * the current picker position
 */
PointLabel *PlotPicker::findLabel()
{DD;
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
            Curve *c = static_cast<Curve *>( *it );
            PointLabel *label = c->findLabel(this->trackerPosition(), c->yAxis());
            if (label) return label;
        }
    }

    return 0;
}

void PlotPicker::pointAppended(const QPoint &pos)
{DD;
    resetHighLighting();

    Curve *curve = NULL;
    int index = -1;

    selectedLabel = findLabel();
    if (selectedLabel) {
        selectedLabel->setSelected(true);
        d_currentPos = pos;
        emit labelSelected(true);
        labelAdded=true;
    }
    else {
        emit labelSelected(false);
        if ((curve = findClosestPoint(pos, index))) {
            d_selectedCurve = curve;
            d_selectedPoint = index;

            QPen pen = curve->pen();
            curve->oldPen = pen;
            pen.setWidth(2);
            curve->setPen(pen);
            curve->setZ(1000);

            highlightPoint(true);
        }
    }
}

void PlotPicker::pointMoved(const QPoint &pos)
{DD;
    if (selectedLabel) {//qDebug()<<"label moving";
        selectedLabel->moveBy(pos-d_currentPos);
        d_currentPos = pos;
    }

    else if (mode == Plot::DataInteraction) {

            if (d_selectedCurve) {

                double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());

                //        double diff = newY - d_selectedCurve->dfd->channels.at(d_selectedCurve->channel)->yValues[d_selectedPoint];

                d_selectedCurve->descriptor->channel(d_selectedCurve->channelIndex)->yValues()[d_selectedPoint] = newY;
                d_selectedCurve->descriptor->setDataChanged(true);

                highlightPoint(true);
            }
        }
//    else {
//        const QwtPlotItemList& itmList = plot->itemList();
//        for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
//            if (( *it )->rtti() == QwtPlotItem::Rtti_PlotMarker ) {
//                QwtPlotMarker *c = static_cast<QwtPlotMarker *>( *it );
//                if (c->lineStyle()==QwtPlotMarker::VLine)
//                    qDebug()<<c->xValue();

//            }
//        }
//    }
}

// Hightlight the selected point
void PlotPicker::highlightPoint(bool showIt)
{DD;
    if (showIt) {
        if (!d_selectedCurve)
            return;
        QPointF val = d_selectedCurve->sample(d_selectedPoint);
        emit updateTrackingCursor(val.x(),false);
        if (!marker) {
            marker = new QwtPlotMarker();
            marker->setLineStyle(QwtPlotMarker::NoLine);
            marker->setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                            Qt::gray,
                                            d_selectedCurve->pen().color(),
                                            QSize(8, 8))
                              );
            marker->setLabelAlignment(Qt::AlignTop);
            marker->setYAxis(d_selectedCurve->yAxis());
            marker->attach(plot);

        }
        marker->setValue(val);
        if (marker->label()==QwtText()) {
            marker->setLabel(QwtText(QString::number(val.x(),'f',2)));
        }

        if (_harmonics.isEmpty()) {
            for (int i=0; i<10; ++i) {
                QwtPlotMarker *d = new QwtPlotMarker();
                d->setLineStyle( QwtPlotMarker::VLine );
                d->setLinePen( Qt::black, 0, Qt::DashDotLine );
                //d->attach(plot);
                _harmonics.append(d);
            }
        }
        if (_showHarmonics) {
            for (int i=0; i<10; ++i) {
                _harmonics[i]->setValue(val.x()*(i+2),0.0);
                _harmonics[i]->attach(plot);
            }
        }

//        d_marker1 = new QwtPlotMarker();
//        d_marker1->setValue( 0.0, 0.0 );
//        d_marker1->setLineStyle( QwtPlotMarker::VLine );
//        d_marker1->setLabelAlignment( Qt::AlignRight | Qt::AlignBottom );
//        d
//        d_marker1->attach( this );

//        QwtPlotGrid *grid = new QwtPlotGrid;
//        grid->enableY(false);
//        QList<double> majorTicks;
//        for (int i=0; i<5; ++i) majorTicks << val.x()*i;
//        QList<double> minorTicks;
//        for (int i=0; i<20; ++i) minorTicks << val.x()*i/4;
//        QList<double> medTicks;
//        for (int i=0; i<10; ++i) medTicks << val.x()*i/2;
//      //  grid->setXDiv(QwtScaleDiv(0,0));
//        grid->attach(plot);
//        //plot->setAxisScaleDiv(QwtPlot::xBottom, QwtScaleDiv(majorTicks.first(), majorTicks.last(), minorTicks,medTicks,majorTicks));
//         grid->setXDiv(QwtScaleDiv(majorTicks.first(), majorTicks.last(), minorTicks,medTicks,majorTicks));

    }
    else {
        foreach (QwtPlotMarker *d, _harmonics) {
            d->detach();
        }
        delete marker;
        marker = 0;
    }


    QwtPlotCanvas *plotCanvas = qobject_cast<QwtPlotCanvas *>(plot->canvas());
    plotCanvas->setPaintAttribute(QwtPlotCanvas::ImmediatePaint, true);
    plot->replot();
    plotCanvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, false );
}


QwtText PlotPicker::trackerTextF(const QPointF &pos) const
{//DD;
    QString text;
    text.sprintf( "%.2f, %.2f", pos.x(), pos.y());

    return QwtText( text );
}
