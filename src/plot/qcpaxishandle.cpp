/***************************************************************************
**                                                                        **
**  QCustomPlot, an easy to use, modern plotting widget for Qt            **
**  Copyright (C) 2011-2021 Emanuel Eichhammer                            **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Emanuel Eichhammer                                   **
**  Website/Contact: http://www.qcustomplot.com/                          **
**             Date: 29.03.21                                             **
**          Version: 2.1.0                                                **
****************************************************************************/

#include "qcpaxishandle.h"
#include "plot/plot.h"
#include "plot/qcpplot.h"
#include "plot/qcptrackingcursor.h"
#include "plot/plotmodel.h"
#include "plot/curve.h"
#include "qcpitemrichtext.h"

QCPAxisHandle::QCPAxisHandle(QCPPlot *plot, QCPTrackingCursor *cursor, Enums::AxisType parentAxis, QObject *parent) :
    QObject(parent), plot(plot), cursor(cursor)
{
    mAxis = plot->axis(parentAxis);
    mDummyTracer = new QCPItemTracer(mAxis->parentPlot());
    mDummyTracer->setVisible(false);

    switch (mAxis->axisType()) {
        case QCPAxis::atRight:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setAxisRect(mAxis->axisRect());
            mDummyTracer->position->setAxes(0, mAxis);
            mDummyTracer->position->setCoords(1, 0);
            break;
        case QCPAxis::atLeft:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setAxisRect(mAxis->axisRect());
            mDummyTracer->position->setAxes(0, mAxis);
            mDummyTracer->position->setCoords(0, 0);
            break;
        case QCPAxis::atBottom:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setAxisRect(mAxis->axisRect());
            mDummyTracer->position->setAxes(mAxis, 0);
            mDummyTracer->position->setCoords(0, 1);
            break;
        case QCPAxis::atTop:
            mDummyTracer->position->setTypeX(QCPItemPosition::ptPlotCoords);
            mDummyTracer->position->setTypeY(QCPItemPosition::ptAxisRectRatio);
            mDummyTracer->position->setAxisRect(mAxis->axisRect());
            mDummyTracer->position->setAxes(mAxis, 0);
            mDummyTracer->position->setCoords(0, 0);
            break;
    }

    // the arrow end (head) is set to move along with the dummy tracer by setting it as its parent
    // anchor. Its coordinate system (setCoords) is thus pixels, and this is how the needed horizontal
    // offset for the tag of the second y axis is achieved. This horizontal offset gets dynamically
    // updated in AxisTag::updatePosition. the arrow "start" is simply set to have the "end" as parent
    // anchor. It is given a horizontal offset to the right, which results in a 15 pixel long arrow.
    mArrow = new QCPItemLine(mAxis->parentPlot());
    mArrow->setLayer("overlay");
    mArrow->setClipToAxisRect(false);
    mArrow->setHead(QCPLineEnding::esFlatArrow);
    mArrow->end->setParentAnchor(mDummyTracer->position);
    mArrow->start->setParentAnchor(mArrow->end);
    mArrow->start->setCoords(0, 15);

    plot->addSelectable(this);
}

QCPAxisHandle::~QCPAxisHandle()
{
    plot->removeSelectable(this);
}

void QCPAxisHandle::setPen(const QPen &pen)
{
    mArrow->setPen(pen);
}

QPen QCPAxisHandle::pen() const
{
    return mArrow->pen();
}

void QCPAxisHandle::updatePosition(double value)
{
    switch (mAxis->axisType()) {
        case QCPAxis::atRight:
            mDummyTracer->position->setCoords(1, value);
            break;
        case QCPAxis::atLeft:
            mDummyTracer->position->setCoords(0, value);
            break;
        case QCPAxis::atBottom:
            mDummyTracer->position->setCoords(value, 1);
            break;
        case QCPAxis::atTop:
            mDummyTracer->position->setCoords(value, 0);
            break;
    }
}

void QCPAxisHandle::detach()
{
    auto plot = mAxis->parentPlot();
    plot->removeItem(mArrow);
    plot->removeItem(mDummyTracer);
//    plot->layer("overlay")->replot();
}

bool QCPAxisHandle::draggable() const
{
    return true;
}

bool QCPAxisHandle::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{
    Q_UNUSED(point);
    Q_UNUSED(pos);
    Q_UNUSED(distanceX);
    Q_UNUSED(distanceY);
    return false;
}

QList<QAction *> QCPAxisHandle::actions()
{
    return cursor->actions();
}

void QCPAxisHandle::updateSelection(SelectedPoint point)
{
    Q_UNUSED(point);
    if (selected()) {
        mArrow->setPen(QPen(Qt::darkGray, 0.5, Qt::SolidLine));
    }
    else {
        mArrow->setPen(QPen(Qt::NoPen));
    }
}
