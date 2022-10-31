#include "qcpaxisoverlay.h"

QCPAxisOverlay::QCPAxisOverlay(QCustomPlot *parent) : QCPItemRect(parent)
{
    setVisible(false);
    setLayer("axes");

    topLeft->setType( QCPItemPosition::ptAxisRectRatio);
    bottomRight->setType( QCPItemPosition::ptAxisRectRatio);
}

void QCPAxisOverlay::setVisibility(bool visible)
{
    if (visible) {
        setColor();
    }
    setVisible(visible);
    parentPlot()->layer("axes")->replot();
}

void QCPAxisOverlay::setColor()
{
    QColor Color = parentPlot()->palette().color(QPalette::Active, QPalette::Highlight);
    if (QApplication::keyboardModifiers() & Qt::CTRL) {
        auto blue = Color.blueF();
        Color.setBlueF(Color.greenF());
        Color.setGreenF(blue);
    }

    QPen Pen;
    Pen.setColor(Color.darker(120));
    Pen.setStyle(Qt::SolidLine);
    Pen.setWidth(1);
    Pen.setCosmetic(true);
    setPen(Pen);

    Color = Color.lighter(130);
    Color.setAlpha(64);
    setBrush(Color);
}

QCPLeftAxisOverlay::QCPLeftAxisOverlay(QCustomPlot *parentPlot) : QCPAxisOverlay(parentPlot)
{
    topLeft->setCoords(0, 0);
    bottomRight->setCoords(0.05, 1);
}


QCPRightAxisOverlay::QCPRightAxisOverlay(QCustomPlot *parentPlot) : QCPAxisOverlay(parentPlot)
{
    topLeft->setCoords(0.95, 0);
    bottomRight->setCoords(1, 1);
}
