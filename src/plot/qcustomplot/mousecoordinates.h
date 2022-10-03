#ifndef MOUSECOORDINATES_H
#define MOUSECOORDINATES_H

#include "qcustomplot.h"

class QCPPlot;

class MouseCoordinates : public QCPItemText
{
public:
    MouseCoordinates(QCPPlot *parent);
    void update(QMouseEvent *event);
private:
    QCPPlot *parent;
};

#endif // MOUSECOORDINATES_H
