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
#include "plot/plotmodel.h"

Picker::Picker(Plot *plot) : plot(plot)
{DD;
    mode = ModeNone;
}

bool Picker::findObject(QMouseEvent *e)
{DD;
    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ControlModifier) {
        pos = e->pos();

        //Ищем элемент под курсором мыши
        double minDist = qInf();
        Selectable *selected = nullptr;

        const auto allItems = plot->itemList();
        for (auto item: allItems) {
            if (auto selectable = dynamic_cast<Selectable*>(item)) {
                qDebug()<<item->rtti()<<item->title().text();
                double distx = 0.0;
                double disty = 0.0;
                if (selectable->underMouse(pos, &distx, &disty)) {
                    qDebug()<<"under mouse"<<distx<<disty;
                    double dist = sqrt(distx*distx+disty*disty);
                    if (!selected || dist < minDist) {
                        selected = selectable;
                        minDist = dist;
                    }
                }
            }
        }
        if (currentSelected && currentSelected != selected)
            currentSelected->setSelected(false);
        currentSelected = selected;


#if 0
        d_selectedCursors = findCursors(pos);

        if (d_selectedLabel)
            d_selectedLabel->setSelected(false);
        d_selectedLabel = findLabel(pos);

        for (auto cursor: qAsConst(d_selectedCursors))
            emit cursorSelected(cursor);

        if (!d_selectedCursors.isEmpty()) {

        }
        else if (d_selectedLabel) {
            d_selectedLabel->setSelected(true);
            d_currentPos = pos;
        }
        else {
            d_selectedCurve = findClosestPoint(pos, d_selectedPoint);
            d_currentPos = pos;
        }
#endif
    }
#if 0
    return (d_selectedCurve || !d_selectedCursors.isEmpty() || d_selectedLabel);
#endif
    return currentSelected != nullptr;
}

void Picker::startPick()
{
    if (currentSelected) currentSelected->setSelected(true);
}

void Picker::deselect()
{
    const auto allItems = plot->itemList();
    for (auto item: allItems) {
        if (auto selectable = dynamic_cast<Selectable*>(item)) {
            //if (selectable != currentSelected)
                selectable->setSelected(false);
        }
    }
}

void Picker::procKeyboardEvent(int key)
{DD;
//    switch (key) {
//        case Qt::Key_Left: {
//            if (d_selectedPoint > 0) {
//                d_selectedPoint--;
//                highlightPoint(true);
//            }
//            break;
//        }
//        case Qt::Key_Right: {
//            if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()-1) {
//                d_selectedPoint++;
//                highlightPoint(true);
//            }
//            break;
//        }
//        case Qt::Key_Space: {
//            if (d_selectedCurve && d_selectedPoint >=0 && d_selectedPoint < d_selectedCurve->samplesCount()) {
//                QPointF val = d_selectedCurve->samplePoint(d_selectedPoint);

//                PointLabel *label = d_selectedCurve->findLabel(d_selectedPoint);

//                if (!label) {
//                    label = new PointLabel(plot, d_selectedCurve);
//                    label->setPoint(d_selectedPoint);
//                    label->setOrigin(val);
//                    d_selectedCurve->addLabel(label);

//                    label->attach(plot);
//                }
//            }
//            else if (d_selectedLabel) {
//                d_selectedLabel->cycleMode();
//            }
//            break;
//        }
//        case Qt::Key_C: {
//            if (d_selectedLabel) {
//                d_selectedLabel->cycleMode();
//            }
//            break;
//        }
//        case Qt::Key_Delete: {
//            if (d_selectedLabel) {
//                Curve *c = d_selectedLabel->curve;
//                if (c) c->removeLabel(d_selectedLabel);
//                d_selectedLabel = 0;
//            }
//            break;
//        }
//    }
}

void Picker::proceedPick(QMouseEvent *e)
{DD;
    if (!enabled || !currentSelected) return;

    if (currentSelected) currentSelected->moveToPos(e->pos());

//    if (!d_selectedCursors.isEmpty()) {
//        for (auto c: qAsConst(d_selectedCursors))
//            emit cursorMovedTo({plot->invTransform(c->xAxis(), currentPos.x()),
//                                plot->invTransform(c->yAxis(), currentPos.y())});
//        d_currentPos = currentPos;
//    }
//    else if (d_selectedLabel) {
//        d_selectedLabel->moveBy(currentPos-d_currentPos);
//        d_currentPos = currentPos;
//    }

    plot->replot();
}

void Picker::endPick(QMouseEvent *e)
{DD;
    if (!enabled) return;
    QPoint endPos = e->pos();
    if (endPos == pos) { //одинарный клик мышью

//        //сбрасываем подсветку кривых
//        highlightPoint(false);
//        plot->model()->resetHighlighting();
//        plot->updateTrackingPanel(); //need this to update xStep for spins


//        //одинарный клик мышью
//        if (d_selectedCursors.isEmpty()) {
//            //ищем, были ли выделены курсоры. Если были, то
//            d_selectedCursors = findCursors(endPos);
//            if (!d_selectedCursors.isEmpty())
//                emit axisClicked({plot->canvasMap(d_selectedCursors.first()->xAxis()).invTransform(endPos.x()),
//                                  plot->canvasMap(d_selectedCursors.first()->yAxis()).invTransform(endPos.y())},
//                                 e->modifiers() & Qt::ControlModifier);
//            else emit axisClicked({plot->canvasMap(QwtAxis::XBottom).invTransform(endPos.x()),
//                                   plot->canvasMap(QwtAxis::YLeft).invTransform(endPos.y())},
//                                  e->modifiers() & Qt::ControlModifier);

//        }

//        if (d_selectedCurve && d_selectedPoint > -1) {
//            d_selectedCurve->setSelected(true);
//            highlightPoint(true);
//            plot->updateTrackingPanel(); //need this to update xStep for spins
//        }

//        if (d_selectedLabel) {
//            d_selectedLabel->setSelected(true);
//        }
    }
    else {
        //протащили какой-то объект, надо бросить

    }

    plot->replot();
}

//QVector<TrackingCursor *> Picker::findCursors(const QPoint &pos)
//{DD;
//    QVector<TrackingCursor *> result;

//    const QwtPlotItemList& itmList = plot->itemList();

//    for (auto it: itmList) {
//        if (auto *c = dynamic_cast<TrackingCursor *>(it)) {
//            int newX = (int)(plot->transform(c->xAxis(), c->xValue()));
//            int newY = (int)(plot->transform(c->yAxis(), c->yValue()));
//            if (qAbs(newX-pos.x())<=5 && (c->type == Cursor::Style::Vertical || c->type == Cursor::Style::Cross)) {
//                result << c;
//            }
//            if (qAbs(newY-pos.y())<=5 && (c->type == Cursor::Style::Horizontal || c->type == Cursor::Style::Cross)) {
//                if (!result.contains(c)) result << c;
//            }
//        }
//    }

//    return result;
//}

Curve *Picker::findClosestPoint(const QPoint &pos, int &index) const
{DD;
    Curve *curve = nullptr;
    double dist = 10e10;
    index = -1;

    const QwtPlotItemList &itmList = plot->itemList();
    for (auto it: itmList) {
        if (Curve *c = dynamic_cast<Curve *>(it)) {
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
{DD;
    const QwtPlotItemList& itmList = plot->itemList();
    for (auto it: itmList) {
        if (Curve *c = dynamic_cast<Curve *>(it)) {
            PointLabel *label = c->findLabel(pos);
            if (label) return label;
        }
    }

    return nullptr;
}

//void Picker::highlightPoint(bool enable)
//{DD;
//    if (marker) marker->detach();
//    delete marker;
//    marker = nullptr;

//    if (enable) {
//        auto selectedCurve = dynamic_cast<Curve*>(currentSelected);

//        if (!selectedCurve) return;

//        QPointF val = selectedCurve->samplePoint(d_selectedPoint);
//        if (!marker) {
//            marker = new PointMarker(d_selectedCurve->pen().color(), d_selectedCurve->yAxis());
//        }
//        marker->attach(plot);
//        marker->moveTo(val);
//        marker->show();
//    }
//}

