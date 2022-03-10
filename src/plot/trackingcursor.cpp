#include "trackingcursor.h"
#include "logging.h"
#include "app.h"
#include <QPen>
#include <qwt_text.h>
#include <qwt_scale_map.h>
#include <qwt_symbol.h>
#include <qwt_plot.h>
#include <QPainter>

TrackingCursor::TrackingCursor(const QColor &col, Cursor::Style type, Cursor *parent)
    : type(type), parent{parent}
{DD;
    auto lineStyle = QwtPlotMarker::NoLine;
    switch (type) {
        case Cursor::Style::Horizontal: lineStyle = QwtPlotMarker::HLine; break;
        case Cursor::Style::Vertical: lineStyle = QwtPlotMarker::VLine; break;
        case Cursor::Style::Cross: lineStyle = QwtPlotMarker::Cross; break;
        default: break;
    }

    setLineStyle(lineStyle);
    setLinePen(col, 1, Qt::SolidLine);
    setLabelAlignment(Qt::AlignBottom | Qt::AlignRight);
    showYValues = App->getSetting("cursorShowYValues", false).toBool();

    xLabel.setBackgroundBrush(Qt::white);
    xLabel.setBorderRadius(1.0);

    yLabel.setBackgroundBrush(Qt::white);
    yLabel.setBorderRadius(1.0);
}

void TrackingCursor::moveTo(const double xValue)
{DD;
    setValue(xValue, 0.0);
}

void TrackingCursor::moveTo(const QPointF &value)
{
    setValue(value);
}

void TrackingCursor::setYValues(const QVector<double> &yValues, const QVector<QColor>&colors)
{DD;
    this->yValues = yValues;
    this->colors = colors;
    updateLabel();
}

void TrackingCursor::updateLabel()
{DD;
    if (!showLabel) return;

    QStringList label;
    if (!yValues.isEmpty() && showYValues) {
        for (int i = 0; i < yValues.size(); ++i) {
            label << QString("<font color=%1>%2</font>").arg(colors[i].name()).arg(yValues[i], 0, 'f', 1);
        }
    }
    label << QString("<b>%1</b>").arg(this->xValue(), 0, 'f', 1);
    xLabel.setText(label.join("<br>"),QwtText::RichText);
    yLabel.setText(QString("<b>%1</b>").arg(this->yValue(), 0, 'f', 1), QwtText::RichText);

    setLabel(xLabel);
}

void TrackingCursor::drawLabel(QPainter *painter, const QRectF &canvasRect, const QPointF &pos) const
{
    if (!showLabel) return;

    if (!xLabel.isEmpty()) {
        painter->save();
        QPointF alignPos = pos;
        alignPos.setY(canvasRect.bottom() - 1);

        qreal pw2 = linePen().widthF() / 2.0;
        if (qFuzzyIsNull(pw2)) pw2 = 0.5;

        const QSizeF textSize = xLabel.textSize(painter->font());

        alignPos.rx() += pw2 + spacing();
        alignPos.ry() -= pw2 + spacing();
        alignPos.ry() -= textSize.height();

        painter->translate(alignPos.x(), alignPos.y());

        const QRectF textRect(0, 0, textSize.width(), textSize.height());
        xLabel.draw(painter, textRect);
        painter->restore();
    }

    if (!yLabel.isEmpty()) {
        painter->save();
        QPointF alignPos = pos;
        alignPos.setX(canvasRect.left());


        qreal pw2 = linePen().widthF() / 2.0;
        if (qFuzzyIsNull(pw2)) pw2 = 0.5;

        const QSizeF textSize = yLabel.textSize(painter->font());

        alignPos.rx() += spacing();
        alignPos.ry() -= pw2 + spacing();
        alignPos.ry() -= textSize.height();

        painter->translate(alignPos.x(), alignPos.y());

        const QRectF textRect(0, 0, textSize.width(), textSize.height());
        yLabel.draw(painter, textRect);
        painter->restore();
    }
}

void TrackingCursor::moveToPos(QPoint pos)
{
    double xVal = 0.0;
    double yVal = 0.0;
    if (plot()) {
        if (type != Cursor::Style::Horizontal) {
            //adjust xval
            auto map = plot()->canvasMap(xAxis());
            xVal = map.invTransform(pos.x());
        }
        if (type != Cursor::Style::Vertical) {
            //adjust yval
            auto map = plot()->canvasMap(yAxis());
            yVal = map.invTransform(pos.y());
        }
    }
    if (parent) parent->moveTo({xVal,yVal});
}

bool TrackingCursor::underMouse(const QPoint &pos, double *distanceX, double *distanceY) const
{
    int newX = (int)(plot()->transform(xAxis(), xValue()));
    int newY = (int)(plot()->transform(yAxis(), yValue()));

    if (type == Cursor::Style::Vertical) {
        if (qAbs(newX-pos.x())<=5) {
            if (distanceX) *distanceX = qAbs(newX-pos.x());
            if (distanceY) *distanceY = qInf();
            return true;
        }
    }
    else if (type == Cursor::Style::Horizontal) {
        if (qAbs(newY-pos.y())<=5) {
            if (distanceY) *distanceY = qAbs(newY-pos.y());
            if (distanceX) *distanceX = qInf();
            return true;
        }
    }
    else if (type == Cursor::Style::Cross) {
        auto x = qAbs(newX-pos.x());
        auto y = qAbs(newY-pos.y());
        auto m = std::min(x, y);
        if (m<=5) {
            if (x < y) {
                if (distanceX) *distanceX = qAbs(newX-pos.x());
                if (distanceY) *distanceY = qInf();
            }
            else {
                if (distanceY) *distanceY = qAbs(newY-pos.y());
                if (distanceX) *distanceX = qInf();
            }
            return true;
        }
    }
    return false;
}

void TrackingCursor::updateSelection()
{
    if (selected())
        setLinePen(linePen().color(), 2, Qt::SolidLine);
    else
        setLinePen(linePen().color(), 1, Qt::SolidLine);
}
