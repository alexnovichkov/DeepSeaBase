#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "qwt_scale_map.h"
#include "curve.h"
//#include "trackingpanel.h"
#include "trackingcursor.h"
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
    marker = 0;
}

bool Picker::findObject(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && ((e->modifiers() == Qt::NoModifier)
                                          || (e->modifiers() == Qt::ControlModifier))) {
        pos = e->pos();

        d_selectedCursor = findCursor(pos);

        if (d_selectedLabel)
            d_selectedLabel->setSelected(false);
        d_selectedLabel = findLabel(pos);

        emit cursorSelected(d_selectedCursor);
        if (d_selectedCursor) {
        }
        else if (d_selectedLabel) {
            d_selectedLabel->setSelected(true);
            d_currentPos = pos;
        }
        else {
            d_selectedCurve = findClosestPoint(pos, d_selectedPoint);
            d_currentPos = pos;
        }
    }
    return (d_selectedCurve || d_selectedCursor || d_selectedLabel);
}

void Picker::procKeyboardEvent(int key)
{
    switch (key) {
        case Qt::Key_Left: {
            if (d_selectedPoint > 0) {
                d_selectedPoint--;
                highlightPoint(true);
            }
            break;
        }
        case Qt::Key_Right: {
            if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()-1) {
                d_selectedPoint++;
                highlightPoint(true);
            }
            break;
        }
        case Qt::Key_Space: {
            if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()) {
                QPointF val = d_selectedCurve->samplePoint(d_selectedPoint);

                PointLabel *label = d_selectedCurve->findLabel(d_selectedPoint);

                if (!label) {
                    label = new PointLabel(plot, d_selectedCurve);
                    label->setPoint(d_selectedPoint);
                    label->setOrigin(val);
                    d_selectedCurve->addLabel(label);

                    label->attach(plot);
                }
            }
            else if (d_selectedLabel) {
                d_selectedLabel->cycleMode();
            }
            break;
        }
        case Qt::Key_C: {
            if (d_selectedLabel) {
                d_selectedLabel->cycleMode();
            }
            break;
        }
        case Qt::Key_Delete: {
            if (d_selectedLabel) {
                Curve *c = d_selectedLabel->curve;
                if (c) c->removeLabel(d_selectedLabel);
                d_selectedLabel = 0;
            }
            break;
        }
    }
}

void Picker::proceedPick(QMouseEvent *e)
{
    if (!enabled) return;

    QPoint currentPos = e->pos();

    if (d_selectedCursor) {
        emit cursorMovedTo({plot->invTransform(d_selectedCursor->xAxis(), currentPos.x()),
                            plot->invTransform(d_selectedCursor->yAxis(), currentPos.y())});
        d_currentPos = currentPos;
    }
    else if (d_selectedLabel) {
        d_selectedLabel->moveBy(currentPos-d_currentPos);
        d_currentPos = currentPos;
    }
    else if (interactionMode == Plot::DataInteraction) {
        if (d_selectedCurve && d_selectedPoint > -1) {
            double newY = plot->invTransform(d_selectedCurve->yAxis(), currentPos.y());
            if (d_selectedCurve->channel->data()->setYValue(d_selectedPoint, newY)) {
                d_selectedCurve->channel->setDataChanged(true);
                d_selectedCurve->channel->descriptor()->setDataChanged(true);
                d_selectedCurve->resetCashedData();
            }
            highlightPoint(true);
        }
    }
    plot->replot();
}

void Picker::endPick(QMouseEvent *e)
{
    if (!enabled) return;
    QPoint endPos = e->pos();
    if (endPos == pos) {
        //сбрасываем подсветку кривых
        highlightPoint(false);
        for(Curve *c: plot->curves) {
            c->resetHighlighting();
        }

        //одинарный клик мышью
        if (!d_selectedCursor) {
            d_selectedCursor = findCursor(endPos);
            if (d_selectedCursor)
                emit axisClicked({plot->canvasMap(d_selectedCursor->xAxis()).invTransform(endPos.x()),
                                  plot->canvasMap(d_selectedCursor->yAxis()).invTransform(endPos.y())},
                                 e->modifiers() & Qt::ControlModifier);
            else emit axisClicked({plot->canvasMap(QwtAxis::xBottom).invTransform(endPos.x()),
                                   plot->canvasMap(QwtAxis::yLeft).invTransform(endPos.y())},
                                  e->modifiers() & Qt::ControlModifier);

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

    plot->replot();
}

TrackingCursor *Picker::findCursor(const QPoint &pos)
{
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.constBegin(); it != itmList.constEnd(); ++it) {
        if (auto *c = dynamic_cast<TrackingCursor *>(*it )) {
            int newX = (int)(plot->transform(c->xAxis(), c->xValue()));
            int newY = (int)(plot->transform(c->yAxis(), c->yValue()));
            if (qAbs(newX-pos.x())<=5 && (c->type == TrackingCursor::Vertical || c->type == TrackingCursor::Cross)) {
                return c;
            }
            if (qAbs(newY-pos.y())<=5 && (c->type == TrackingCursor::Horizontal || c->type == TrackingCursor::Cross)) {
                return c;
            }
        }
    }

    return nullptr;
}

Curve *Picker::findClosestPoint(const QPoint &pos, int &index) const
{
    Curve *curve = nullptr;
    double dist = 10e10;
    index = -1;

    const QwtPlotItemList &itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.constBegin(); it != itmList.constEnd(); ++it) {
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

    return nullptr;
}

PointLabel *Picker::findLabel(const QPoint &pos)
{
    const QwtPlotItemList& itmList = plot->itemList();
    for (QwtPlotItemIterator it = itmList.constBegin(); it != itmList.constEnd(); ++it) {
        if (Curve *c = dynamic_cast<Curve *>(*it )) {
            PointLabel *label = c->findLabel(pos);
            if (label) return label;
        }
    }

    return nullptr;
}

void Picker::highlightPoint(bool enable)
{
    if (marker) marker->detach();
    delete marker;
    marker = nullptr;

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

