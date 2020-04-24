#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "qwt_scale_map.h"
#include "curve.h"
#include "trackingpanel.h"
#include "pointmarker.h"
#include "pointlabel.h"
#include "fileformats/filedescriptor.h"

Picker::Picker(Plot *plot) : plot(plot)
{
    interactionMode = Plot::ScalingInteraction;
    mode = ModeNone;

    d_selectedPoint = -1;
    d_selectedCurve = 0;
    d_selectedLabel = 0;
    d_selectedCursor = 0;

//    d_currentPos;
    marker = 0;

    plot->canvas()->installEventFilter(this);
}

bool Picker::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == plot->canvas() && enabled) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::MouseButtonDblClick)
            procMouseEvent(event);
        else if (event->type() == QEvent::KeyPress) {
            procKeyboardEvent(event);
        }
    }
    return QObject::eventFilter(watched,event);
}

void Picker::procMouseEvent(QEvent *e)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(e);

    switch (mEvent->type())  {
        case QEvent::MouseButtonPress:
            startPick(mEvent);
            break;
        case QEvent::MouseMove:
            proceedPick(mEvent);
            break;
        case QEvent::MouseButtonRelease:
            endPick(mEvent);
            break;
        default: ;
    }
}

void Picker::procKeyboardEvent(QEvent *e)
{
    QKeyEvent *event = static_cast<QKeyEvent *>(e);

    const int key = event->key();

    if (key == Qt::Key_Left) {
        if (d_selectedCursor)
            emit moveCursor(false);

        if (d_selectedPoint > 0) {
            d_selectedPoint--;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Right) {
        if (d_selectedCursor)
            emit moveCursor(true);

        if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()-1) {
            d_selectedPoint++;
            highlightPoint(true);
        }
    }
    else if (key == Qt::Key_Space) {
        if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()) {
            QPointF val = d_selectedCurve->samplePoint(d_selectedPoint);

            PointLabel *label = d_selectedCurve->findLabel(d_selectedPoint);

            if (!label) {
                label = new PointLabel(plot, d_selectedCurve);
                label->setPoint(d_selectedPoint);
                label->setOrigin(val);
                d_selectedCurve->addLabel(label);

                label->attach(plot);
//                plot->replot();
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
            Curve *c = d_selectedLabel->curve;
            if (c) c->removeLabel(d_selectedLabel);
            d_selectedLabel = 0;
            emit setZoomEnabled(true);
//            plot->replot();
        }
    }
    else if (key == Qt::Key_H) {
        plot->switchLabelsVisibility();
    }
}

void Picker::startPick(QMouseEvent *e)
{
    if (mode == ModeNone) {
        if (e->button() == Qt::LeftButton && ((e->modifiers() == Qt::NoModifier)
            || (e->modifiers() == Qt::ControlModifier))) {
            pos = e->pos();
            mode = ModeDrag;

            d_selectedCursor = findCursor(pos);

            if (d_selectedLabel)
                d_selectedLabel->setSelected(false);
            d_selectedLabel = findLabel(pos);

            emit cursorSelected(d_selectedCursor);
            if (d_selectedCursor) {
                emit setZoomEnabled(false);
            }
            else if (d_selectedLabel) {
                d_selectedLabel->setSelected(true);
                d_currentPos = pos;
                emit setZoomEnabled(false);
            }
            else {
                d_selectedCurve = findClosestPoint(pos, d_selectedPoint);
                d_currentPos = pos;
                if (interactionMode == Plot::DataInteraction)
                    emit setZoomEnabled(false);
            }
        }
    }
}

void Picker::proceedPick(QMouseEvent *e)
{
    if (mode == ModeDrag) {
        QPoint currentPos = e->pos();

        if (d_selectedCursor) {
            emit cursorMovedTo(plot->invTransform(QwtAxis::xBottom, currentPos.x()));
            d_currentPos = currentPos;
        }
        else if (d_selectedLabel) {
            d_selectedLabel->moveBy(currentPos-d_currentPos);
            d_currentPos = currentPos;
        }
        else if (interactionMode == Plot::DataInteraction) {
            if (d_selectedCurve && d_selectedPoint > -1) {
                double newY = plot->invTransform(d_selectedCurve->yAxis(), currentPos.y());
                if (d_selectedCurve->descriptor->channel(d_selectedCurve->channelIndex)->data()->setYValue(d_selectedPoint, newY)) {
                    d_selectedCurve->channel->setDataChanged(true);
                    d_selectedCurve->descriptor->setDataChanged(true);
                }
                highlightPoint(true);
            }
        }
        plot->replot();
    }
}

void Picker::endPick(QMouseEvent *e)
{
    if (mode == ModeDrag) {
        if (e->button() == Qt::LeftButton &&
            ((e->modifiers() == Qt::NoModifier) || (e->modifiers() == Qt::ControlModifier))) {
            QPoint endPos = e->pos();

            if (endPos == pos) {
                highlightPoint(false);
                foreach(Curve *c, plot->curves) {
                    c->resetHighlighting();
                }


                //одинарный клик мышью
                if (!d_selectedCursor) {
                    emit xAxisClicked(plot->canvasMap(QwtAxis::xBottom).invTransform(endPos.x()),
                                      e->modifiers() & Qt::ControlModifier);
                    d_selectedCursor = findCursor(endPos);
                }

                if (d_selectedCurve && d_selectedPoint > -1) {
                    d_selectedCurve->highlight();
                    highlightPoint(true);
                }

                if (d_selectedLabel) {
                    d_selectedLabel->setSelected(true);
                }
            }
            else {
                //протащили какой-то объект, надо бросить
            }

            emit setZoomEnabled(true);
            mode = ModeNone;
        }

        plot->replot();
    }
}

QwtPlotMarker *Picker::findCursor(const QPoint &pos)
{
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (TrackingCursor *c = dynamic_cast<TrackingCursor *>(*it )) {
            int newX = (int)(plot->transform(QwtAxis::xBottom, c->xValue()));
            if (qAbs(newX-pos.x())<=5) {
                return c;
            }
        }
    }

    return 0;
}

Curve *Picker::findClosestPoint(const QPoint &pos, int &index) const
{
    Curve *curve = 0;
    double dist = 10e10;
    index = -1;

    const QwtPlotItemList &itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (Curve *c = dynamic_cast<Curve *>(*it )) {
            double d;
            int idx = c->closest( pos, &d );
            if ( d < dist ) {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    if (dist < 10 && index >=0)
        return curve;

    return 0;
}

PointLabel *Picker::findLabel(const QPoint &pos)
{
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
        if (Curve *c = dynamic_cast<Curve *>(*it )) {
            PointLabel *label = c->findLabel(pos);
            if (label) return label;
        }
    }

    return 0;
}

void Picker::highlightPoint(bool enable)
{
    if (marker) marker->detach();
    delete marker;
    marker = 0;

    if (enable) {
        if (!d_selectedCurve) return;

        QPointF val = d_selectedCurve->samplePoint(d_selectedPoint);
        if (!marker) {
            marker = new PointMarker(d_selectedCurve->pen().color(), d_selectedCurve->yAxis());
            marker->attach(plot);
            marker->moveTo(val);
            marker->show();
        }
    }
}

