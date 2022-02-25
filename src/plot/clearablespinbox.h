#ifndef CLEARABLESPINBOX_H
#define CLEARABLESPINBOX_H

#include <QAbstractSpinBox>
#include "enums.h"

class ClearableSpinBox: public QAbstractSpinBox
{
    Q_OBJECT
public:
    enum Axis {
        XAxis,
        YAxis
    };
    explicit ClearableSpinBox(QWidget *parent);
    void setAxis(Axis axis);
    void moveOneStep(Enums::Direction direction);
    void setXStep(double step);
    void setYStep(double step);
    void setStep(double step);
    void setXValues(const QVector<double> &values);
    void setYValues(const QVector<double> &values);
    void moveTo(const QPointF &position);
    void moveBy(int xSteps, int ySteps);
    double getXValue() const {return xVal;}
    double getYValue() const {return yVal;}
    double getXStep() const {return xStep;}
    double getYStep() const {return yStep;}
    void setPrefix(const QString &prefix);
    void setXRange(double min, double max);
    void setYRange(double min, double max);
signals:
    void valueChanged(QPointF value);
private:
    void updateText(double val);
    void moveByX(int steps);
    void moveByY(int steps);
    double yVal = 0.0;
    double xVal = 0.0;
    double xStep = 0.0;
    double yStep = 0.0;
    int xCurrent = 0;
    int yCurrent = 0;
    QVector<double> xValues;
    QVector<double> yValues;
    QString prefix;
    double xMin = 0.0, xMax = 0.0;
    double yMin = 0.0, yMax = 0.0;
    Axis axis = XAxis;

    // QAbstractSpinBox interface
public:
    virtual void stepBy(int steps) override;

protected:
    virtual StepEnabled stepEnabled() const override;

    // QWidget interface
public:
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
};

#endif // CLEARABLESPINBOX_H
