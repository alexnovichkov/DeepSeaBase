#ifndef AXISBOUNDSDIALOG_H
#define AXISBOUNDSDIALOG_H

#include <QDialog>
#include "enums.h"


class AxisBoundsDialog : public QDialog
{
public:
    AxisBoundsDialog(double leftBorder, double rightBorder, Enums::AxisType axis, QWidget *parent = 0);
    inline double leftBorder() const {return _leftBorder;}
    inline double rightBorder() const {return _rightBorder;}
    inline bool autoscale() const {return _autoscale;}
private:
    double _leftBorder;
    double _rightBorder;
    Enums::AxisType _axis;
    bool _autoscale;
};

#endif // AXISBOUNDSDIALOG_H
