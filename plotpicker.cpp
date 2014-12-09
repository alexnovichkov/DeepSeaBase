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
#include "pointlabel.h"
#include "logging.h"

PlotPicker::PlotPicker(QWidget *canvas) :
    QwtPlotPicker(canvas),
    d_selectedCurve( NULL ),
    d_selectedPoint( -1 )
{DD;
    plot = qobject_cast<QwtPlotCanvas*>(canvas)->plot();

    marker = 0;
    mode = Plot::ScalingInteraction;
    selectedLabel = 0;

    setStateMachine(new QwtPickerDragPointMachine);

    setTrackerMode(QwtPicker::AlwaysOn);
    setRubberBandPen(QPen(QColor(60,60,60), 0.5, Qt::DashLine));

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
                label = new PointLabel(QString("%1").arg(val.x()), plot);
                label->setPoint(d_selectedPoint);
                label->setOrigin(val);
                label->setYAxis(d_selectedCurve->yAxis());
                d_selectedCurve->addLabel(label);

                label->attach(plot);
                plot->replot();
            }
            else {
                selectedLabel = label;
                selectedLabel->setSelected(true);
               // d_currentPos = pos;
//                qDebug()<<"label at"<<d_selectedPoint;
            }

//            Marker *m = d_selectedCurve->findMarker(d_selectedPoint);
//            if (!m) {
//                m = d_selectedCurve->addMarker(d_selectedPoint);
//                m->attach(plot);
//            }
//            else {
//                highlightPoint(false);
//                switch (m->type) {
//                    case 0: {
//                        m->type = 1;
//                        m->setLabel(QwtText(QString("%1\n%2").arg(val.x()).arg(val.y(),0,'f',2)));
//                        break;
//                    }
//                    case 1: {
//                        m->type = 2;
//                        m->setLabel(QwtText(QString("%1").arg(val.y(),0,'f',2)));
//                        break;
//                    }
//                    case 2: {//удаляем
//                        d_selectedCurve->removeMarker(d_selectedPoint);
//                        plot->replot();
//                        break;
//                    }
//                    default: break;
//                }
//            }
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
    }
    else {
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
    if (mode != Plot::DataInteraction) return;

    if (selectedLabel) {
        selectedLabel->moveBy(pos-d_currentPos);
        d_currentPos = pos;
    }

    else if (d_selectedCurve) {

        double newY = plot->invTransform(d_selectedCurve->yAxis(), pos.y());

//        double diff = newY - d_selectedCurve->dfd->channels.at(d_selectedCurve->channel)->yValues[d_selectedPoint];

        d_selectedCurve->descriptor->channel(d_selectedCurve->channelIndex)->yValues()[d_selectedPoint] = newY;
        d_selectedCurve->descriptor->setDataChanged(true);

        highlightPoint(true);
    }
}

// Hightlight the selected point
void PlotPicker::highlightPoint( bool showIt )
{DD;
    if (showIt) {
        if (!d_selectedCurve)
            return;
        QPointF val = d_selectedCurve->sample(d_selectedPoint);
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
            marker->setLabel(QwtText(QString("%1").arg(val.x())));
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
