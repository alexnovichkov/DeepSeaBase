#ifndef QCPITEMRICHTEXT_H
#define QCPITEMRICHTEXT_H

#include "qcustomplot.h"

class QCPItemRichText : public QCPItemText
{
public:
    QCPItemRichText(QCustomPlot *parentPlot);
//    void setLabels(const )

    // QCPLayerable interface
protected:
    virtual void draw(QCPPainter *painter) override;

    // QCPAbstractItem interface
protected:
    virtual QPointF anchorPixelPosition(int anchorId) const override;
};

#endif // QCPITEMRICHTEXT_H
