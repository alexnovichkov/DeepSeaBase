#ifndef AXISBOUNDSDIALOG_H
#define AXISBOUNDSDIALOG_H

#include <QDialog>
#include "enums.h"

#include "plot/qcustomplot.h"

class AxisBoundsDialog : public QDialog
{
public:
    AxisBoundsDialog(QCPAxis *axis, AxisParameters parameters, QWidget *parent = 0);
    inline QCPRange range() const {return {_leftBorder, _rightBorder};}
    AxisParameters parameters() const;
private:
    double _leftBorder = 0; //левая граница
    double _rightBorder = 0; //правая граница
    QCPAxis* _axis = nullptr;
    AxisParameters _parameters;
};

#endif // AXISBOUNDSDIALOG_H
