#ifndef AXISBOUNDSDIALOG_H
#define AXISBOUNDSDIALOG_H

#include <QDialog>
#include <qwt_axis_id.h>


class AxisBoundsDialog : public QDialog
{
public:
    AxisBoundsDialog(double leftBorder, double rightBorder, QwtAxisId axis, QWidget *parent = 0);
    inline double leftBorder() const {return _leftBorder;}
    inline double rightBorder() const {return _rightBorder;}
    inline bool autoscale() const {return _autoscale;}
private:
    double _leftBorder;
    double _rightBorder;
    QwtAxisId _axis;
    bool _autoscale;
};

#endif // AXISBOUNDSDIALOG_H
