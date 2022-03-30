#include "pointlabel.h"

#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "logging.h"
#include <QPen>
#include <QPainter>
#include <QVector2D>
#include <QMenu>
#include "curve.h"
#include <app.h>

PointLabel::PointLabel(QwtPlot *parent, Curve *curve)
    : QwtPlotItem(),
      d_point(-1),
      d_origin(QPointF(0.0, 0.0)),
      d_displacement(QPoint(0, -13)),
      plot(parent),
      curve(curve)
{DD;
    setZ(40.0);
    if (App->getSetting("pointLabelRemember", true).toBool())
        d_mode = App->getSetting("pointLabelMode", 0).toInt();
    setXAxis(curve->xAxis());
    setYAxis(curve->yAxis());
    updateLabel();
}

PointLabel::~PointLabel()
{DD;

}

int PointLabel::rtti() const
{//DD;
    return QwtPlotItem::Rtti_PlotUserItem+1;
}

QPointF PointLabel::origin() const
{DD;
    return d_origin;
}

void PointLabel::setOrigin(const QPointF &origin)
{DD;
    if (d_origin == origin) return;
    d_origin = origin;
    updateLabel();
}

void PointLabel::setMode(int mode)
{
    if (d_mode==mode) return;
    d_mode = mode;
    updateLabel();
}

void PointLabel::cycle()
{
    switch (d_mode) {
        case 0: setMode(1); break;
        case 1: setMode(2); break;
        case 2: setMode(0); break;
        default: break;
    }
}

int PointLabel::point() const
{DD;
    return d_point;
}

void PointLabel::setPoint(int point)
{DD;
    d_point = point;
}

QwtText PointLabel::label() const
{DD;
    return d_label;
}

void PointLabel::updateSelection()
{
    d_label.setBorderPen(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    itemChanged();
}

bool PointLabel::underMouse(const QPoint &pos, double *distanceX, double *distanceY) const
{
    QPointF point(plot->transform(curve->xAxis(), d_origin.x()),
                plot->transform(curve->yAxis(), d_origin.y()));

    const QSizeF textSize = d_label.textSize();

    point.rx() += d_displacement.x();
    point.ry() += d_displacement.y();

    point.rx() -= textSize.width() / 2;
    point.ry() -= textSize.height() / 2;

    if (distanceX) *distanceX = qAbs(point.x()-pos.x());
    if (distanceY) *distanceY = qAbs(point.y()-pos.y());

    return QRectF(point.x(),
                  point.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

void PointLabel::setLabel(const QwtText &label)
{DD;
    if (d_label == label) return;
    d_label = label;
    itemChanged();
}

void PointLabel::draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap,
                      const QRectF &canvasRect) const
{DD;
    Q_UNUSED(canvasRect)
    QPointF pos(xMap.transform(d_origin.x()), yMap.transform(d_origin.y()));

    const QSizeF textSize = d_label.textSize(painter->font());

    pos.rx() += d_displacement.x();
    pos.ry() += d_displacement.y();

    pos.rx() -= textSize.width() / 2;
    pos.ry() -= textSize.height() / 2;


    painter->translate(pos.x(), pos.y());
    const QRectF textRect(0, 0, textSize.width(), textSize.height());
    d_label.draw(painter, textRect);
}

void PointLabel::remove()
{
    if (curve) curve->removeLabel(this);
}

void PointLabel::moveToPos(QPoint pos, QPoint startPos)
{
    auto delta = pos-startPos;
    qDebug()<<pos<<startPos<<delta;
//    if (pos.x() == 0 && pos.y() == 0) return;
    d_displacement.rx() += delta.x();
    d_displacement.ry() += delta.y();
    itemChanged();
}

QList<QAction *> PointLabel::actions()
{
    QList<QAction *> l;

    auto a = new QAction("Показывать", plot);
    QMenu *m = new QMenu();
    QActionGroup *ag = new QActionGroup(plot);

    auto a1 = m->addAction("значение по оси X", [=](){
        if (App->getSetting("pointLabelRemember", true).toBool())
            App->setSetting("pointLabelMode", 0);
        setMode(0);
    });
    a1->setCheckable(true);
    a1->setChecked(d_mode==0);
    ag->addAction(a1);

    a1 = m->addAction("значения по осям X и Y", [=](){
        if (App->getSetting("pointLabelRemember", true).toBool())
            App->setSetting("pointLabelMode", 1);
        setMode(1);
    });
    a1->setCheckable(true);
    a1->setChecked(d_mode==1);
    ag->addAction(a1);

    a1 = m->addAction("значение по оси Y", [=](){
        if (App->getSetting("pointLabelRemember", true).toBool())
            App->setSetting("pointLabelMode", 2);
        setMode(2);
    });
    a1->setCheckable(true);
    a1->setChecked(d_mode==2);
    ag->addAction(a1);

    a->setMenu(m);
    l << a;

    a = new QAction("Запомнить выбор", plot);
    a->setCheckable(true);
    a->setChecked(App->getSetting("pointLabelRemember", true).toBool());
    QObject::connect(a, &QAction::triggered, [=](){
        bool r = App->getSetting("pointLabelRemember", true).toBool();
        App->setSetting("pointLabelRemember", !r);
    });
    l<<a;

    return l;
}

bool PointLabel::contains(const QPoint &pos)
{DD;
    QPointF point(plot->transform(curve->xAxis(), d_origin.x()),
                plot->transform(curve->yAxis(), d_origin.y()));

    const QSizeF textSize = d_label.textSize();

    point.rx() += d_displacement.x();
    point.ry() += d_displacement.y();

    point.rx() -= textSize.width() / 2;
    point.ry() -= textSize.height() / 2;

    return QRectF(point.x(),
                  point.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

void PointLabel::updateLabel()
{
    switch (d_mode) {
        case 0: d_label = QwtText(QString::number(d_origin.x(),'f',2)); break;
        case 1: d_label = QwtText(QString("%1; %2")
                                  .arg(QString::number(d_origin.x(),'f',2))
                                  .arg(QString::number(d_origin.y(),'f',1))); break;
        case 2: d_label = QwtText(QString::number(d_origin.y(),'f',1)); break;
    }
    d_label.setBorderPen(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    setTitle(d_label);
    itemChanged();
}
