#ifndef QCPAXISOVERLAY_H
#define QCPAXISOVERLAY_H

#include "qcustomplot.h"

class QCPAxisOverlay : public QCPItemRect
{
public:
    QCPAxisOverlay(QCustomPlot *parentPlot);
    void setVisibility(bool visible);
    void setColor();
};

class QCPLeftAxisOverlay : public QCPAxisOverlay
{
public:
    explicit QCPLeftAxisOverlay(QCustomPlot *parentPlot);
};

class QCPRightAxisOverlay : public QCPAxisOverlay
{
public:
    explicit QCPRightAxisOverlay(QCustomPlot *parentPlot);
};

#endif // QCPAXISOVERLAY_H
