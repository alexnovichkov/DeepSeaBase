#include "qcptrackingcursor.h"
#include "logging.h"
#include "app.h"
#include <QPen>
#include <QPainter>
#include <QInputDialog>
#include "cursordialog.h"
#include <QMenu>
#include "plot.h"
#include "qcustomplot/qcpplot.h"

QCPTrackingCursor::QCPTrackingCursor(const QColor &col, Cursor::Style type, Cursor *parent)
    : type(type), parent{parent}
{DDD;
    impl = dynamic_cast<QCPPlot*>(parent->plot()->impl());

    horizontal = new QCPItemStraightLine(impl);
    horizontal->setPen(col);
    horizontal->point1->setAxes(impl->xAxis, impl->yAxis);
    horizontal->point2->setAxes(impl->xAxis, impl->yAxis);
    horizontal->setLayer("overlay");
    horizontal->setAntialiased(false);

    vertical = new QCPItemStraightLine(impl);
    vertical->setPen(col);
    vertical->point1->setAxes(impl->xAxis, impl->yAxis);
    vertical->point2->setAxes(impl->xAxis, impl->yAxis);
    vertical->setLayer("overlay");
    vertical->setAntialiased(false);

    switch (type) {
        case Cursor::Style::Horizontal: vertical->setVisible(false); break;
        case Cursor::Style::Vertical: horizontal->setVisible(false); break;
        case Cursor::Style::Cross: break;
        default: break;
    }

    energyAct = new QAction("Энергия", parent);
    energyAct->setCheckable(true);
    energyAct->setChecked(parent?parent->info() & Cursor::Energy : false);
    QObject::connect(energyAct, &QAction::triggered, [=](){
        if (parent) {
            auto info = parent->info();
            info.setFlag(Cursor::Energy, !(info & Cursor::Energy));
            parent->setInfo(info);
        }
    });

    rmsAct = new QAction("СКЗ", parent);
    rmsAct->setCheckable(true);
    rmsAct->setChecked(parent?parent->info() & Cursor::RMS : false);
    QObject::connect(rmsAct, &QAction::triggered, [=](){
        if (parent) {
            auto info = parent->info();
            info.setFlag(Cursor::RMS, !(info & Cursor::RMS));
            parent->setInfo(info);
        }
    });
}

void QCPTrackingCursor::moveTo(const double xValue)
{DDD;
    vertical->point1->setCoords(xValue, 0.0);
    vertical->point2->setCoords(xValue, 10.0);
    impl->layer("overlay")->replot();
}

void QCPTrackingCursor::moveTo(const QPointF &value)
{DDD;
    vertical->point1->setCoords(value.x(), 0.0);
    vertical->point2->setCoords(value.x(), 10.0);

    horizontal->point1->setCoords(0.0, value.y());
    horizontal->point2->setCoords(10.0, value.y());
    impl->layer("overlay")->replot();
}

void QCPTrackingCursor::moveToPos(QPoint pos, QPoint startPos)
{DDD;
    Q_UNUSED(startPos);
    double xVal = 0.0;
    double yVal = 0.0;

        if (type != Cursor::Style::Horizontal) {
            //adjust xval
            xVal = impl->xAxis->pixelToCoord(pos.x());
        }
        if (type != Cursor::Style::Vertical) {
            //adjust yval
            yVal = impl->yAxis->pixelToCoord(pos.y());
        }

    if (parent) parent->moveTo({xVal,yVal}, this);
}

void QCPTrackingCursor::moveLeft(int count)
{DDD;
    if (parent) parent->moveTo(Qt::Key_Left, count, this);
}

void QCPTrackingCursor::moveRight(int count)
{DDD;
    if (parent) parent->moveTo(Qt::Key_Right, count, this);
}

void QCPTrackingCursor::moveUp(int count)
{DDD;
    if (parent) parent->moveTo(Qt::Key_Up, count, this);
}

void QCPTrackingCursor::moveDown(int count)
{DDD;
    if (parent) parent->moveTo(Qt::Key_Down, count, this);
}

QList<QAction *> QCPTrackingCursor::actions()
{DDD;
    QList<QAction *> l;
    QAction *
    a = new QAction("Переместить в...", parent);
    QObject::connect(a, &QAction::triggered, [=](){
        double val = QInputDialog::getDouble(0, "Переместить курсор в...", "", xValue());
        if (parent) parent->moveTo({val, yValue()}, this);
    });
    l << a;

    a = new QAction("Копировать уровни дискрет", parent);
    QObject::connect(a, &QAction::triggered, [=](){
        if (parent) parent->copyValues();
    });
    l << a;

    if (parent->plot()->type() == Enums::PlotType::Spectrogram) {
        a = new QAction("Сохранить спектр...", parent);
        QObject::connect(a, &QAction::triggered, [=](){
            parent->plot()->saveSpectrum(yValue());
        });
        l << a;

        a = new QAction("Сохранить проходную...", parent);
        QObject::connect(a, &QAction::triggered, [=](){
            parent->plot()->saveThroughput(xValue());
        });
        l << a;
    }

    a = new QAction("Свойства...", parent);
    QObject::connect(a, &QAction::triggered, [=](){
        CursorDialog d(parent, nullptr);
        d.exec();
    });
    l << a;

    if (parent->type()==Cursor::Type::DoubleReject ||
        parent->type()==Cursor::Type::Double) {

        a = new QAction("Показывать", parent);
        QMenu *m = new QMenu();

        m->addAction(rmsAct);
        m->addAction(energyAct);
        energyAct->setChecked(parent->info() & Cursor::Energy);
        rmsAct->setChecked(parent->info() & Cursor::RMS);

        a->setMenu(m);
        l << a;
    }

    return l;
}

bool QCPTrackingCursor::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DDD;
    Q_UNUSED(point);
    int newX = impl->xAxis->coordToPixel(xValue());
    int newY = impl->yAxis->coordToPixel(yValue());

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

void QCPTrackingCursor::updateSelection(SelectedPoint point)
{DDD;
    Q_UNUSED(point);
    auto p = horizontal->pen();
    p.setWidth(selected()?2:1);
    horizontal->setPen(p);
    vertical->setPen(p);
    impl->layer("overlay")->replot();
}

double QCPTrackingCursor::xValue() const
{
    return vertical->point1->key();
}

double QCPTrackingCursor::yValue() const
{
    return horizontal->point1->value();
}

void QCPTrackingCursor::detach()
{
    impl->removeItem(horizontal);
    impl->removeItem(vertical);
    impl->layer("overlay")->replot();
}

void QCPTrackingCursor::setColor(const QColor &color)
{
    horizontal->setPen(color);
    vertical->setPen(color);
    impl->layer("overlay")->replot();
}

void QCPTrackingCursor::setPen(const QPen &pen)
{
    horizontal->setPen(pen);
    vertical->setPen(pen);
    impl->layer("overlay")->replot();
}
