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

#include "qcpaxistag.h"
#include "plot/plot.h"
#include "plot/qcustomplot/qcpplot.h"
#include "plot/qcptrackingcursor.h"
#include "plot/plotmodel.h"
#include "plot/curve.h"
#include "qcpitemrichtext.h"

QCPAxisTag::QCPAxisTag(Plot *parent, QCPTrackingCursor *cursor, QCPAxis *parentAxis) :
  QObject(parentAxis), parent(parent), cursor(cursor), mAxis(parentAxis)
{
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

  // The text label is anchored at the arrow start (tail) and has its "position" aligned at the
  // left, and vertically centered to the text label box.
  mLabel = new QCPItemRichText(mAxis->parentPlot());
  mLabel->setLayer("overlay");
  mLabel->setClipToAxisRect(false);
  mLabel->setPadding(QMargins(3, 3, 3, 3));
  mLabel->setBrush(QBrush(QColor(255,255,255,230)));
//  mLabel->setPen(QPen(Qt::black));
  mLabel->setPositionAlignment(Qt::AlignLeft|Qt::AlignBottom);
  mLabel->position->setParentAnchor(mDummyTracer->position);
  mLabel->position->setCoords(2,-2);
}

QCPAxisTag::~QCPAxisTag()
{
  if (mDummyTracer)
    mDummyTracer->parentPlot()->removeItem(mDummyTracer);
  if (mLabel)
    mLabel->parentPlot()->removeItem(mLabel);
}

void QCPAxisTag::setPen(const QPen &pen)
{
  mLabel->setPen(pen);
}

void QCPAxisTag::setBrush(const QBrush &brush)
{
  mLabel->setBrush(brush);
}

void QCPAxisTag::setText(const QString &text)
{
  mLabel->setText(text);
}

QPen QCPAxisTag::pen() const
{
    return mLabel->pen();
}

QBrush QCPAxisTag::brush() const { return mLabel->brush(); }

QString QCPAxisTag::text() const { return mLabel->text(); }

void QCPAxisTag::updatePosition(double value)
{
    // since both the arrow and the text label are chained to the dummy tracer (via anchor
  // parent-child relationships) it is sufficient to update the dummy tracer coordinates. The
  // Horizontal coordinate type was set to ptAxisRectRatio so to keep it aligned at the right side
  // of the axis rect, it is always kept at 1. The vertical coordinate type was set to
  // ptPlotCoordinates of the passed parent axis, so the vertical coordinate is set to the new
  // value.
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
//    updateLabel(showLabels);
}

void QCPAxisTag::updateLabel(bool showValues)
{
    this->showLabels = showValues;
    QStringList label;
    char f = cursor->parent->format() == Cursor::Format::Fixed?'f':'e';
    if (showValues && mAxis->orientation() != Qt::Vertical) {
        auto list = parent->model()->curves();
        for (auto curve: list) {
            bool success = false;
            auto val = curve->channel->data()->YforXandZ(cursor->xValue(), cursor->yValue(), success);
            QString s = QString::number(success?val:qQNaN(), f, cursor->parent->digits());

            label << QString("<font color=%1>%2</font>")
                     .arg(curve->pen().color().name())
                     .arg(s);
        }
    }
    QString s = QString::number(mAxis->orientation()==Qt::Horizontal ? cursor->xValue():cursor->yValue(), f, cursor->parent->digits());
    label << QString("<b>%1</b>").arg(s);
    mLabel->setText(label.join("<br>")/*, QwtText::RichText*/);
}

void QCPAxisTag::detach()
{

}


