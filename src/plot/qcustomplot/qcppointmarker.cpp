#include "qcppointmarker.h"

#include "qcpplot.h"
#include "graph2d.h"
#include "data2d.h"
#include "plot/curve.h"
#include "plot/plot.h"

/*!
  Creates a tracer item and sets default values.

  The created item is automatically registered with \a parentPlot. This QCustomPlot instance takes
  ownership of the item, so do not delete it manually but use QCustomPlot::removeItem() instead.
*/
QCPTracer::QCPTracer(QCPPlot *parentPlot) :
  QCPAbstractItem(parentPlot),
  position(createPosition(QLatin1String("position"))),
  mSize(6),
  mStyle(tsCrosshair),
  mGraph(nullptr),
  mGraphIndex(0)
{
  position->setCoords(0, 0);

  setBrush(Qt::NoBrush);
  setSelectedBrush(Qt::NoBrush);
  setPen(QPen(Qt::black));
}

QCPTracer::~QCPTracer()
{
}

void QCPTracer::setPen(const QPen &pen)
{
  mPen = pen;
}

void QCPTracer::setSelectedPen(const QPen &pen)
{
  mSelectedPen = pen;
}

void QCPTracer::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

void QCPTracer::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

void QCPTracer::setSize(double size)
{
  mSize = size;
}

void QCPTracer::setStyle(QCPTracer::TracerStyle style)
{
  mStyle = style;
}

void QCPTracer::setGraph(Graph2D *graph)
{
  if (graph)
  {
    if (graph->parentPlot() == mParentPlot)
    {
      position->setType(QCPItemPosition::ptPlotCoords);
      position->setAxes(graph->keyAxis(), graph->valueAxis());
      mGraph = graph;
      updatePosition();
    } else
      qDebug() << Q_FUNC_INFO << "graph isn't in same QCustomPlot instance as this item";
  } else
  {
    mGraph = nullptr;
  }
}

void QCPTracer::setGraphIndex(int key)
{
  mGraphIndex = key;
}

double QCPTracer::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if (onlySelectable && !mSelectable)
    return -1;

  QPointF center(position->pixelPosition());
  double w = mSize/2.0;
  QRect clip = clipRect();
  switch (mStyle)
  {
    case tsNone: return -1;
    case tsPlus:
    {
      if (clipRect().intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        return qSqrt(qMin(QCPVector2D(pos).distanceSquaredToLine(center+QPointF(-w, 0), center+QPointF(w, 0)),
                          QCPVector2D(pos).distanceSquaredToLine(center+QPointF(0, -w), center+QPointF(0, w))));
      break;
    }
    case tsCrosshair:
    {
      return qSqrt(qMin(QCPVector2D(pos).distanceSquaredToLine(QCPVector2D(clip.left(), center.y()), QCPVector2D(clip.right(), center.y())),
                        QCPVector2D(pos).distanceSquaredToLine(QCPVector2D(center.x(), clip.top()), QCPVector2D(center.x(), clip.bottom()))));
    }
    case tsCircle:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {
        // distance to border:
        double centerDist = QCPVector2D(center-pos).length();
        double circleLine = w;
        double result = qAbs(centerDist-circleLine);
        // filled ellipse, allow click inside to count as hit:
        if (result > mParentPlot->selectionTolerance()*0.99 && mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0)
        {
          if (centerDist <= circleLine)
            result = mParentPlot->selectionTolerance()*0.99;
        }
        return result;
      }
      break;
    }
    case tsSquare:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {
        QRectF rect = QRectF(center-QPointF(w, w), center+QPointF(w, w));
        bool filledRect = mBrush.style() != Qt::NoBrush && mBrush.color().alpha() != 0;
        return rectDistance(rect, pos, filledRect);
      }
      break;
    }
  }
  return -1;
}

/* inherits documentation from base class */
void QCPTracer::draw(QCPPainter *painter)
{
  updatePosition();
  if (mStyle == tsNone)
    return;

  painter->setPen(mainPen());
  painter->setBrush(mainBrush());
  QPointF center(position->pixelPosition());
  double w = mSize/2.0;
  QRect clip = clipRect();
  switch (mStyle)
  {
    case tsNone: return;
    case tsPlus:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
      {
        painter->drawLine(QLineF(center+QPointF(-w, 0), center+QPointF(w, 0)));
        painter->drawLine(QLineF(center+QPointF(0, -w), center+QPointF(0, w)));
      }
      break;
    }
    case tsCrosshair:
    {
      if (center.y() > clip.top() && center.y() < clip.bottom())
        painter->drawLine(QLineF(clip.left(), center.y(), clip.right(), center.y()));
      if (center.x() > clip.left() && center.x() < clip.right())
        painter->drawLine(QLineF(center.x(), clip.top(), center.x(), clip.bottom()));
      break;
    }
    case tsCircle:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        painter->drawEllipse(center, w, w);
      break;
    }
    case tsSquare:
    {
      if (clip.intersects(QRectF(center-QPointF(w, w), center+QPointF(w, w)).toRect()))
        painter->drawRect(QRectF(center-QPointF(w, w), center+QPointF(w, w)));
      break;
    }
  }
}

void QCPTracer::updatePosition()
{
    if (mGraph) {
        if (mParentPlot->hasPlottable(mGraph)) {
            if (mGraphIndex < 0 || mGraphIndex >= mGraph->data()->size()) return;
            position->setCoords(mGraph->data()->mainKey(mGraphIndex), mGraph->data()->mainValue(mGraphIndex));
        }
        else qDebug() << Q_FUNC_INFO << "graph not contained in QCustomPlot instance (anymore)";
    }
}

QPen QCPTracer::mainPen() const
{
  return mSelected ? mSelectedPen : mPen;
}

QBrush QCPTracer::mainBrush() const
{
  return mSelected ? mSelectedBrush : mBrush;
}


QCPPointMarker::QCPPointMarker(PointLabel *parent)
    : parent(parent)
{

}

QCPPointMarker::~QCPPointMarker()
{
    setVisible(false);
}

void QCPPointMarker::setVisible(bool visible)
{
    if (marker) marker->setVisible(visible);
    if (text) text->setVisible(visible);
}

void QCPPointMarker::moveTo(int index)
{
    if (marker) {
        marker->setGraphIndex(index);
        marker->updatePosition();
    }
}

void QCPPointMarker::update()
{
    auto origin = parent->point();
    moveTo(origin.x);
    if (text) text->position->setCoords(parent->getDisplacement());
    //text->setText(QString::number(marker->position->key(),'f',2));
}


void QCPPointMarker::attachTo(Plot *plot)
{
    if (auto qcp = dynamic_cast<QCPPlot*>(plot->impl())) {
        marker = new QCPTracer(qcp);
        marker->setStyle(QCPTracer::tsSquare);
        marker->setSize(8);
        marker->setAntialiased(false);
        if (auto g = dynamic_cast<Graph2D*>(parent->curve())) {
            marker->setGraph(g);
            marker->setGraphIndex(0);
        }

        text = new QCPItemText(qcp);
        QColor col(Qt::white);
        col.setAlphaF(0.8);
        text->setBrush(col);
        text->setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        text->position->setParentAnchor(marker->position);
        text->position->setPixelPosition({0, -10});
    }
}

void QCPPointMarker::detachFrom(Plot *plot)
{
    if (auto qcp = dynamic_cast<QCPPlot*>(plot->impl())) {
        qcp->removeItem(text);
        qcp->removeItem(marker);
    }
}

void QCPPointMarker::setColor(const QColor &color)
{
    if (marker) {
        auto pen = marker->pen();
        pen.setColor(color);
        marker->setPen(pen);
    }
}

void QCPPointMarker::setBrush(const QBrush &brush)
{
    if (text) {
        text->setBrush(brush);
    }
}

void QCPPointMarker::setXAxis(Enums::AxisType axis)
{
    Q_UNUSED(axis);
}

void QCPPointMarker::setYAxis(Enums::AxisType axis)
{
    Q_UNUSED(axis);
}

void QCPPointMarker::setLabel(const QString &label)
{
    if (text) text->setText(label);
}

void QCPPointMarker::setBorder(const QPen &pen)
{
    if (text) text->setPen(pen);
}

QSizeF QCPPointMarker::textSize() const
{
    if (text) return QFontMetricsF(text->font()).size(Qt::TextSingleLine, text->text());
    return QSizeF();
}
