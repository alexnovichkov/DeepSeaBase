#include "qcptracer.h"

#include "qcpplot.h"
#include "graph2d.h"
#include "data2d.h"

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
      LOG(DEBUG) << Q_FUNC_INFO << "graph isn't in same QCustomPlot instance as this item";
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
        else LOG(DEBUG) << Q_FUNC_INFO << "graph not contained in QCustomPlot instance (anymore)";
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
