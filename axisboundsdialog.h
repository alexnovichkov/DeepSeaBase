#ifndef AXISBOUNDSDIALOG_H
#define AXISBOUNDSDIALOG_H

#include <QDialog>



class AxisBoundsDialog : public QDialog
{
public:
    AxisBoundsDialog(double leftBorder, double rightBorder, QWidget *parent = 0);
    double leftBorder() const;
    double rightBorder() const;
private:
    double _leftBorder;
    double _rightBorder;
};

#endif // AXISBOUNDSDIALOG_H
