#include "trackingcursor.h"
#include "logging.h"
#include "mainwindow.h"
#include <QPen>
#include <qwt_text.h>
#include <qwt_scale_map.h>
#include <qwt_symbol.h>
#include <QPainter>

TrackingCursor::TrackingCursor(const QColor &col, Type type): type(type)
{DD;
    auto lineStyle = QwtPlotMarker::NoLine;
    switch (type) {
        case Horizontal: lineStyle = QwtPlotMarker::HLine; break;
        case Vertical: lineStyle = QwtPlotMarker::VLine; break;
        case Cross: lineStyle = QwtPlotMarker::Cross; break;
        default: break;
    }

    setLineStyle(lineStyle);
    setLinePen(col, 1, /*Qt::DashDotLine*/Qt::SolidLine);
    setLabelAlignment(Qt::AlignBottom | Qt::AlignRight);
    showYValues = MainWindow::getSetting("cursorShowYValues", false).toBool();

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

void TrackingCursor::setCurrent(bool current)
{DD;
    this->current = current;
    if (current)
        setLinePen( this->linePen().color(), 2, /*Qt::DashDotLine*/Qt::SolidLine );
    else
        setLinePen( this->linePen().color(), 1, /*Qt::DashDotLine*/Qt::SolidLine );
}

void TrackingCursor::updateLabel()
{DD;
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
