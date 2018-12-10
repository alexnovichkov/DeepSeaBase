#ifndef AXISBOUNDSDIALOG_H
#define AXISBOUNDSDIALOG_H

#include <QDialog>



class AxisBoundsDialog : public QDialog
{
public:
    AxisBoundsDialog(double leftBorder, double rightBorder, int axis, QWidget *parent = 0);
    double leftBorder() const;
    double rightBorder() const;
    bool autoscale() const {return _autoscale;}
private:
    double _leftBorder;
    double _rightBorder;
    int _axis;
    bool _autoscale;
};

#endif // AXISBOUNDSDIALOG_H
