#include "trackingcursor.h"
#include "logging.h"
#include "app.h"
#include <QPen>
#include <qwt_text.h>
#include <qwt_scale_map.h>
#include <qwt_symbol.h>
#include <qwt_plot.h>
#include <QPainter>
#include <QInputDialog>
#include "cursordialog.h"
#include <QMenu>

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

    energyAct = new QAction("Энергия", parent);
    energyAct->setCheckable(true);
    energyAct->setChecked(parent?parent->info() & Cursor::Energy : false);
    QObject::connect(energyAct, &QAction::triggered, [=](){
        if (!parent) return;
        auto info = parent->info();
        info.setFlag(Cursor::Energy, !(info & Cursor::Energy));
        parent->setInfo(info);
    });

    rmsAct = new QAction("СКЗ", parent);
    rmsAct->setCheckable(true);
    rmsAct->setChecked(parent?parent->info() & Cursor::RMS : false);
    QObject::connect(rmsAct, &QAction::triggered, [=](){
        if (!parent) return;
        auto info = parent->info();
        info.setFlag(Cursor::RMS, !(info & Cursor::RMS));
        parent->setInfo(info);
    });
}

void TrackingCursor::moveTo(const double xValue)
{DD;
    setValue(xValue, 0.0);
}

void TrackingCursor::moveTo(const QPointF &value)
{
    setValue(value);
}

void TrackingCursor::moveToPos(QPoint pos, QPoint startPos)
{
    Q_UNUSED(startPos);
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
    if (parent) parent->moveTo({xVal,yVal}, this);
}

void TrackingCursor::moveLeft(int count)
{
    if (parent) parent->moveTo(Qt::Key_Left, count, this);
}

void TrackingCursor::moveRight(int count)
{
    if (parent) parent->moveTo(Qt::Key_Right, count, this);
}

void TrackingCursor::moveUp(int count)
{
    if (parent) parent->moveTo(Qt::Key_Up, count, this);
}

void TrackingCursor::moveDown(int count)
{
    if (parent) parent->moveTo(Qt::Key_Down, count, this);
}

QList<QAction *> TrackingCursor::actions()
{
    QList<QAction *> l;
    QAction *
    a = new QAction("Переместить в...", parent);
    QObject::connect(a, &QAction::triggered, [=](){
        double val = QInputDialog::getDouble(0, "Переместить курсор в...", "", xValue());
        if (parent) parent->moveTo({val,yValue()}, this);
    });
    l << a;

    a = new QAction("Копировать уровни дискрет", parent);
    QObject::connect(a, &QAction::triggered, [=](){
        if (parent) parent->copyValues();
    });
    l << a;

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
    auto p = linePen();
    p.setWidth(selected()?2:1);
    setLinePen(p);
}
