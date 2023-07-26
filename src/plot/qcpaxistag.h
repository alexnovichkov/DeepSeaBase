#ifndef QCPAXISTAG_H
#define QCPAXISTAG_H

#include <QObject>
#include "qcustomplot.h"
#include "plot/selectable.h"
#include "enums.h"

class QCPPlot;
class QCPTrackingCursor;
class QCPItemRichText;

class QCPAxisTag : public QObject, public Selectable
{
  Q_OBJECT
public:
  explicit QCPAxisTag(QCPPlot *plot, QCPTrackingCursor *cursor, Enums::AxisType parentAxis, QObject *parent = nullptr);
  virtual ~QCPAxisTag();

  // setters:
  void setPen(const QPen &pen);
  void setBrush(const QBrush &brush);
  void setText(const QString &text);

  // getters:
  QPen pen() const;
  QBrush brush() const;
  QString text() const;

  // other methods:
  void updatePosition(double value);
  void updateLabel(bool showValues, bool showPeaksInfo = false);

  void detach();

protected:
  QCPPlot *parent = nullptr;
  QCPTrackingCursor *cursor = nullptr;
  QCPAxis *mAxis = nullptr;

  QCPItemTracer *mDummyTracer = nullptr;
  QCPItemRichText *mLabel = nullptr;
  bool showLabels = false;

  // Selectable interface
public:
  virtual bool draggable() const override;
  virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;
  virtual QList<QAction *> actions() override;

protected:
  virtual void updateSelection(SelectedPoint point) override;
};

#endif // QCPAXISTAG_H
