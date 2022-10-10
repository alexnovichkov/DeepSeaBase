#ifndef QCPAXISTAG_H
#define QCPAXISTAG_H

#include <QObject>
#include "qcustomplot.h"

class Plot;
class QCPTrackingCursor;
class QCPItemRichText;

class QCPAxisTag : public QObject
{
  Q_OBJECT
public:
  explicit QCPAxisTag(Plot *parent, QCPTrackingCursor *cursor, QCPAxis *parentAxis);
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
  void updateLabel(bool showValues);

  void detach();

protected:
  Plot *parent;
  QCPTrackingCursor *cursor;
  QCPAxis *mAxis;

  QPointer<QCPItemTracer> mDummyTracer;
  QPointer<QCPItemRichText> mLabel;
  bool showLabels = false;
};

#endif // QCPAXISTAG_H
