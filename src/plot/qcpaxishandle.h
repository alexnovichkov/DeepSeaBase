#ifndef QCPAXISHANDLE_H
#define QCPAXISHANDLE_H

#include <QObject>
#include "qcustomplot.h"
#include "plot/selectable.h"
#include "enums.h"

class Plot;
class QCPTrackingCursor;
class QCPItemRichText;

class QCPAxisHandle : public QObject, public Selectable
{
  Q_OBJECT
public:
  explicit QCPAxisHandle(Plot *parent, QCPTrackingCursor *cursor, Enums::AxisType parentAxis);
  virtual ~QCPAxisHandle();

  // setters:
  void setPen(const QPen &pen);

  // getters:
  QPen pen() const;

  // other methods:
  void updatePosition(double value);
  void detach();

protected:
  Plot *parent = nullptr;
  QCPTrackingCursor *cursor = nullptr;
  QCPAxis *mAxis = nullptr;
  QCPItemLine *mArrow = nullptr;
  QCPItemTracer *mDummyTracer = nullptr;

  // Selectable interface
public:
  virtual bool draggable() const override;
  virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;
  virtual QList<QAction *> actions() override;

protected:
  virtual void updateSelection(SelectedPoint point) override;
};

#endif // QCPAXISTAG_H
