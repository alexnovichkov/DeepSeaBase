#ifndef QCPTRACER_H
#define QCPTRACER_H

#include "qcustomplot.h"

class QCPPlot;
class Graph2D;

class QCPTracer : public QCPAbstractItem
{
  Q_OBJECT
  Q_PROPERTY(QPen pen READ pen WRITE setPen)
  Q_PROPERTY(QPen selectedPen READ selectedPen WRITE setSelectedPen)
  Q_PROPERTY(QBrush brush READ brush WRITE setBrush)
  Q_PROPERTY(QBrush selectedBrush READ selectedBrush WRITE setSelectedBrush)
  Q_PROPERTY(double size READ size WRITE setSize)
  Q_PROPERTY(TracerStyle style READ style WRITE setStyle)
  Q_PROPERTY(Graph2D* graph READ graph WRITE setGraph)
  Q_PROPERTY(int graphIndex READ graphIndex WRITE setGraphIndex)
public:
  enum TracerStyle { tsNone        ///< The tracer is not visible
                     ,tsPlus       ///< A plus shaped crosshair with limited size
                     ,tsCrosshair  ///< A plus shaped crosshair which spans the complete axis rect
                     ,tsCircle     ///< A circle
                     ,tsSquare     ///< A square
                   };
  Q_ENUMS(TracerStyle)

  explicit QCPTracer(QCPPlot *parentPlot);
  virtual ~QCPTracer() Q_DECL_OVERRIDE;

  // getters:
  QPen pen() const { return mPen; }
  QPen selectedPen() const { return mSelectedPen; }
  QBrush brush() const { return mBrush; }
  QBrush selectedBrush() const { return mSelectedBrush; }
  double size() const { return mSize; }
  TracerStyle style() const { return mStyle; }
  Graph2D *graph() const { return mGraph; }
//  double graphKey() const { return mGraphKey; }
  double graphIndex() const { return mGraphIndex; }

  // setters;
  void setPen(const QPen &pen);
  void setSelectedPen(const QPen &pen);
  void setBrush(const QBrush &brush);
  void setSelectedBrush(const QBrush &brush);
  void setSize(double size);
  void setStyle(TracerStyle style);
  void setGraph(Graph2D *graph);
//  void setGraphKey(double key);
  void setGraphIndex(int index);

  // reimplemented virtual methods:
  virtual double selectTest(const QPointF &pos, bool onlySelectable, QVariant *details=nullptr) const Q_DECL_OVERRIDE;

  // non-virtual methods:
  void updatePosition();

  QCPItemPosition * const position;

protected:
  // property members:
  QPen mPen, mSelectedPen;
  QBrush mBrush, mSelectedBrush;
  double mSize;
  TracerStyle mStyle;
  Graph2D *mGraph;
//  double mGraphKey;
  int mGraphIndex;

  // reimplemented virtual methods:
  virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;

  // non-virtual methods:
  QPen mainPen() const;
  QBrush mainBrush() const;
};

#endif // QCPTRACER_H
