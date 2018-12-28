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
#include "trackingpanel.h"

PlotPicker::PlotPicker(QWidget *canvas) :
    QwtPlotPicker(canvas),
    d_selectedPoint( -1 ),
    d_selectedCurve( NULL )
{DD;
    plot = qobject_cast<QwtPlotCanvas*>(canvas)->plot();

    marker = 0;
    mode = Plot::ScalingInteraction;
    d_selectedLabel = 0;
    defaultCursor = plot->canvas()->cursor();

    d_selectedCursor = 0;

    setStateMachine(new QwtPickerDragPointMachine);
    setTrackerMode(QwtPicker::AlwaysOn);

    connect(this,SIGNAL(appended(QPoint)),this,SLOT(pointAppended(QPoint)));
    connect(this,SIGNAL(moved(QPoint)),this,SLOT(pointMoved(QPoint)));
}

PlotPicker::~PlotPicker()
{DD;

}

void PlotPicker::setMode(Plot::InteractionMode mode)
{DD;
    this->mode = mode;
}

void PlotPicker::widgetKeyReleaseEvent(QKeyEvent *e)
{DD;
    const int key = e->key();

    if (key == Qt::Key_Left) {
        if (d_selectedPoint > 0) {
            highlightPoint(false);
            d_selectedPoint--;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Right) {
        if (d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()-1) {
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
            }
        }
        else if (d_selectedLabel) {
            d_selectedLabel->cycleMode();
        }

    }
    else if (key == Qt::Key_C) {
        if (d_selectedLabel) {
            d_selectedLabel->cycleMode();
            //plot->replot();
        }
    }
    else if (key == Qt::Key_Delete) {
        if (d_selectedLabel) {
            const QwtPlotItemList& itmList = plot->itemList();
            for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
                if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
                    Curve *c = static_cast<Curve *>( *it );
                    c->removeLabel(d_selectedLabel);
                }
            }
            d_selectedLabel = 0;
            emit labelSelected(false);
            plot->replot();
        }
    }
    else if (key == Qt::Key_H) {
        static_cast<Plot*>(plot)->switchLabelsVisibility();
    }
}

void PlotPicker::widgetKeyPressEvent(QKeyEvent *e)
{
    const int key = e->key();
   // if (d_selectedPoint == -1) return;

    // блокируем срабатывание клавиш, чтобы не сдвигался курсор мыши
    if (key == Qt::Key_Left || key == Qt::Key_Right) {

    }
    else QwtPlotPicker::widgetKeyPressEvent(e);
}

void PlotPicker::resetHighLighting()
{DDD;
    highlightPoint(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;
    d_selectedLabel = 0;
    d_selectedCursor = 0;

    Plot *p = static_cast<Plot*>(plot);
    if (p) {
        foreach(Curve *c, p->graphs) {
            c->resetHighlighting();
        }
    }
    plot->updateLegend();

    emit cursorSelected(d_selectedCursor);
    emit labelSelected(false);
}

Curve * PlotPicker::findClosestPoint(const QPoint &pos, int &index) const
{DDD;
    Curve *curve = 0;
    double dist = 10e10;

    const QwtPlotItemList& itmList = plot->itemList(QwtPlotItem::Rtti_PlotCurve);
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        Curve *c = static_cast<Curve *>( *it );

        double d;
        int idx = c->closest( pos, &d );
//        int idx = c->closestPoint( pos, &d );
        if ( d < dist ) {
            curve = c;
            index = idx;
            dist = d;
        }
    }

    if (dist < 10 && index >=0)
        return curve;

    return 0;
}

/**
 * @brief PlotPicker::findLabel
 * @return the first PointLabel whose boundingRect contains
 * the current picker position
 */
PointLabel *PlotPicker::findLabel()
{DDD;
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve ) {
            if (Curve *c = static_cast<Curve *>( *it )) {
                PointLabel *label = c->findLabel(this->trackerPosition(), c->yAxis());
                if (label) return label;
            }
        }
    }

    return 0;
}

QwtPlotMarker *PlotPicker::findCursor(const QPoint &pos)
{DDD;
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (( *it )->rtti() == QwtPlotItem::Rtti_PlotMarker ) {
            if (TrackingCursor *c = static_cast<TrackingCursor *>( *it )) {
                int newX = (int)(plot->transform(QwtPlot::xBottom, c->xValue()));
                if (qAbs(newX-pos.x())<=5) {
                    return c;
                }
            }
        }
    }

    return 0;
}

void PlotPicker::pointAppended(const QPoint &pos)
{DDD;
    resetHighLighting();

    Curve *curve = NULL;
    int index = -1;

    d_selectedLabel = findLabel();
    d_selectedCursor = findCursor(pos);

    //обновляем состояние курсоров дискрет. selectedCursor ==0 если ни одного курсора не выделено
    emit cursorSelected(d_selectedCursor);

    if (d_selectedCursor) {
        d_currentPos = pos;
        emit labelSelected(true);
    }
    else if (d_selectedLabel) {
        d_selectedLabel->setSelected(true);
        d_currentPos = pos;
        emit labelSelected(true);
    }
    else {
        emit labelSelected(mode == Plot::DataInteraction);
        if ((curve = findClosestPoint(pos, index))) {
            d_selectedCurve = curve;
            d_selectedPoint = index;

            d_selectedCurve->highlight();
            highlightPoint(true);
        }
    }
}

void PlotPicker::pointMoved(const QPoint &pos)
{DDD;
    if (d_selectedLabel) {//qDebug()<<"label moving";
        d_selectedLabel->moveBy(pos-d_currentPos);
        d_currentPos = pos;
    }
    else if (d_selectedCursor) {
        emit cursorMovedTo(d_selectedCursor, plot->invTransform(QwtPlot::xBottom, pos.x()));
        d_currentPos = pos;
    }

    else if (mode == Plot::DataInteraction) {
        if (d_selectedCurve) {
            double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());

            if (d_selectedCurve->descriptor->channel(d_selectedCurve->channelIndex)->data()->setYValue(d_selectedPoint, newY))
                d_selectedCurve->descriptor->setDataChanged(true);
            highlightPoint(true);
        }
    }
}

// Hightlight the selected point
void PlotPicker::highlightPoint(bool showIt)
{DDD;
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
{//DD;
    QString text;
    text.sprintf( "%.2f, %.2f", pos.x(), pos.y());

    return QwtText( text );
}
