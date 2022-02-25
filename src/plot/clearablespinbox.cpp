#include "clearablespinbox.h"
#include "logging.h"
#include <QLineEdit>
#include "algorithms.h"


ClearableSpinBox::ClearableSpinBox(QWidget *parent) : QAbstractSpinBox(parent)
{DD;
    setKeyboardTracking(false);
    setWrapping(false);
    lineEdit()->setAlignment(Qt::AlignLeft);
    updateText(xVal);

    connect(this, &QAbstractSpinBox::editingFinished, [=](){
        bool ok;
        double val = lineEdit()->text().toDouble(&ok);
        if (ok) {
            if (axis == XAxis)
                moveTo({val, yVal});
            else
                moveTo({xVal, val});
        }
    });
}

void ClearableSpinBox::setAxis(ClearableSpinBox::Axis axis)
{
    this->axis = axis;
    updateText(axis == XAxis?xVal:yVal);
}

void ClearableSpinBox::moveOneStep(Enums::Direction direction)
{
    switch (direction) {
        case Enums::Up: moveBy(0,1); break;
        case Enums::Down: moveBy(0,-1); break;
        case Enums::Left: moveBy(-1,0); break;
        case Enums::Right: moveBy(1,0); break;
    }
}

void ClearableSpinBox::setXStep(double step)
{DD;
    this->xStep = step;
    if (!qFuzzyIsNull(step)) xValues.clear();
}

void ClearableSpinBox::setYStep(double step)
{DD;
    this->yStep = step;
    if (!qFuzzyIsNull(step)) yValues.clear();
}

void ClearableSpinBox::setStep(double step)
{
    if (axis==XAxis) setXStep(step);
    else setYStep(step);
}

void ClearableSpinBox::setXValues(const QVector<double> &values)
{DD;
    xValues = values;
    xStep = 0.0;
    if (!xValues.isEmpty())
        setXRange(xValues.first(), xValues.last());
}

void ClearableSpinBox::setYValues(const QVector<double> &values)
{
    yValues = values;
    yStep = 0.0;
    if (!xValues.isEmpty())
        setYRange(yValues.first(), yValues.last());
}

void ClearableSpinBox::moveTo(const QPointF &position)
{DD;
    //если spectrogram, то вторая координата соответствует zVal
    int xDistance = 0;
    int yDistance = 0;

    if (!qIsNaN(position.x())) {
        if (qFuzzyIsNull(xStep) && !xValues.isEmpty()) {
            //moving to the nearest xValue from xValues
            //не будет работать, если на одном графике есть октавы и третьоктавы
            auto currentIndex = closest(xValues.cbegin(), xValues.cend(), xVal);
            auto newIndex = closest(xValues.cbegin(), xValues.cend(), position.x());
            xDistance = std::distance(currentIndex, newIndex);
        }
        else {
            if (!qFuzzyCompare(xVal, position.x())) {
                xVal = position.x();
            }
        }
    }
    if (!qIsNaN(position.y())) {
        if (qFuzzyIsNull(yStep) && !yValues.isEmpty()) {
            //moving to the nearest yValue from yValues
            auto currentIndex = closest(yValues.cbegin(), yValues.cend(), yVal);
            auto newIndex = closest(yValues.cbegin(), yValues.cend(), position.y());
            yDistance = std::distance(currentIndex, newIndex);
        }
        else {
            if (!qFuzzyCompare(yVal, position.y())) {
                yVal = position.y();
            }
        }
    }

    if (xDistance!=0) moveByX(xDistance);
    if (yDistance!=0) moveByY(yDistance);

    updateText(axis==XAxis?xVal:yVal);
    emit valueChanged({xVal, yVal});
}

void ClearableSpinBox::moveBy(int xSteps, int ySteps)
{DD;
    if (xSteps==0 && ySteps==0) return;

    if (xSteps != 0) moveByX(xSteps);
    if (ySteps != 0) moveByY(ySteps);

    updateText(axis==XAxis?xVal:yVal);
    emit valueChanged({xVal, yVal});
}

void ClearableSpinBox::moveByX(int steps)
{DD;
    if (qFuzzyIsNull(xStep) && !xValues.isEmpty()) {
        int newcurrent = std::clamp(steps + xCurrent, 0, xValues.size()-1);
        if (xCurrent != newcurrent) {
            xCurrent = newcurrent;
            xVal = xValues.at(xCurrent);
        }
    }
    else {
        if (xVal + steps*xStep >= xMin && xVal + steps*xStep <= xMax) {
            xVal += steps*xStep;
        }
    }
}

void ClearableSpinBox::moveByY(int steps)
{DD;
    if (qFuzzyIsNull(yStep) && !yValues.isEmpty()) {
        int newcurrent = std::clamp(steps + yCurrent, 0, yValues.size()-1);
        if (yCurrent != newcurrent) {
            yCurrent = newcurrent;
            yVal = yValues.at(yCurrent);
        }
    }
    else {
        if (yVal + steps*yStep >= yMin && yVal + steps*yStep <= yMax) {
            yVal += steps*yStep;
        }
    }
}

void ClearableSpinBox::stepBy(int steps)
{DD;
    if (axis==XAxis) moveBy(steps, 0);
    else moveBy(0, steps);
}

void ClearableSpinBox::setPrefix(const QString &prefix)
{DD;
    this->prefix = prefix;
    updateText(axis==XAxis?xVal:yVal);
}

void ClearableSpinBox::setXRange(double min, double max)
{DD;
    this->xMin = min;
    this->xMax = max;
}

void ClearableSpinBox::setYRange(double min, double max)
{DD;
    this->yMin = min;
    this->yMax = max;
}

void ClearableSpinBox::updateText(double val)
{DD;
    double v = std::abs(val);
    QString s;
    if (qFuzzyIsNull(v)) s = "0";
    else if (v < 0.001) s = QString::number(val,'e',4);
    else if (v > 99999) s = QString::number(val,'e',4);
    else if (v < 10) s = QString::number(val,'f', 6);
    else if (v < 100) s = QString::number(val,'f', 5);
    else if (v < 1000) s = QString::number(val,'f', 4);
    else if (v < 10000) s = QString::number(val,'f', 3);
    else if (v < 100000) s = QString::number(val,'f', 2);
    else s = QString::number(val,'f',6);
    lineEdit()->setText(prefix+s);
}




QAbstractSpinBox::StepEnabled ClearableSpinBox::stepEnabled() const
{DD;
    QAbstractSpinBox::StepEnabled res = 0;
    if (axis==XAxis) {
        if (qFuzzyIsNull(xStep) && !xValues.isEmpty()) {
            if (closest(xValues.cbegin(), xValues.cend(), xVal)==xValues.cbegin()) return StepUpEnabled;
            if (closest(xValues.cbegin(), xValues.cend(), xVal)==xValues.cend()-1) return StepDownEnabled;
        }
        if (xVal > xMin) res |= StepDownEnabled;
        if (xVal < xMax) res |= StepUpEnabled;
    }
    else {
        if (qFuzzyIsNull(yStep) && !yValues.isEmpty()) {
            if (closest(yValues.cbegin(), yValues.cend(), yVal)==yValues.cbegin()) return StepUpEnabled;
            if (closest(yValues.cbegin(), yValues.cend(), yVal)==yValues.cend()-1) return StepDownEnabled;
        }
        if (yVal > yMin) res |= StepDownEnabled;
        if (yVal < yMax) res |= StepUpEnabled;
    }

    return res;
}


QSize ClearableSpinBox::sizeHint() const
{DD;
    auto s = QAbstractSpinBox::sizeHint();

    s.setWidth(fontMetrics().horizontalAdvance(prefix+"+9,9999e+999"));
    return s;
}

QSize ClearableSpinBox::minimumSizeHint() const
{DD;
    auto s = QAbstractSpinBox::minimumSizeHint();
    s.setWidth(50);
    return s;
}
