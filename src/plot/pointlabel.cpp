#include "pointlabel.h"

#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "logging.h"
#include <QPen>
#include <QPainter>
#include <QVector2D>
#include <QMenu>
#include "curve.h"
#include "settings.h"
#include "pointmarker.h"
#include "algorithms.h"

PointLabel::PointLabel(QwtPlot *parent, Curve *curve)
    : QwtPlotItem(),
      m_displacement(QPoint(0, -13)),
      m_marker{QwtSymbol::Ellipse, QBrush(Qt::gray), QColor(Qt::black), QSize(16,16)},
      m_plot(parent),
      m_curve(curve)
{DDD;
    setZ(40.0);
    if (Settings::getSetting("pointLabelRemember", true).toBool()) {
        switch (Settings::getSetting("pointLabelMode", 0).toInt()) {
            case 0: m_mode = Mode::XValue; break;
            case 1: m_mode = Mode::XYValue; break;
            case 2: m_mode = Mode::YValue; break;
            case 3: m_mode = Mode::XYZValue; break;
        }
    }
    m_marker.setColor(m_curve->pen().color());

    m_label.setBackgroundBrush(Qt::white);
    setXAxis(curve->xAxis());
    setYAxis(curve->yAxis());
    updateLabel();
}

PointLabel::~PointLabel()
{DDD;

}

int PointLabel::rtti() const
{//DDD;
    return QwtPlotItem::Rtti_PlotUserItem+1;
}

//SamplePoint PointLabel::origin() const
//{DDD;
//    return m_origin;
//}

//void PointLabel::setOrigin(const SamplePoint &origin)
//{DDD;
//    if (m_origin == origin) return;
//    m_origin = origin;
//    updateLabel();
//}

SamplePoint PointLabel::getOrigin() const
{
    auto val = m_curve->samplePoint(m_point);
    if (qIsNaN(val.z)) return val;
    return {val.x, val.z, val.y};
}

void PointLabel::setMode(Mode mode)
{DDD;
    if (m_mode==mode) return;
    m_mode = mode;
    updateLabel();
}

void PointLabel::cycle()
{DDD;
    switch (m_mode) {
        case Mode::XValue: setMode(Mode::XYValue); break;
        case Mode::XYValue: setMode(Mode::YValue); break;
        case Mode::YValue: setMode(Mode::XYZValue); break;
        case Mode::XYZValue: setMode(Mode::XValue); break;
        default: break;
    }
}

SelectedPoint PointLabel::point() const
{DDD;
    return m_point;
}

void PointLabel::setPoint(SelectedPoint point)
{DDD;
    m_point = point;
    updateLabel();
}

QwtText PointLabel::label() const
{DDD;
    return m_label;
}

void PointLabel::updateSelection(SelectedPoint point)
{DDD;
    Q_UNUSED(point);
    m_label.setBorderPen(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    itemChanged();
}

bool PointLabel::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DDD;
    Q_UNUSED(point);
    auto origin = getOrigin();
    QPointF p(m_plot->transform(m_curve->xAxis(), origin.x),
                m_plot->transform(m_curve->yAxis(), origin.y));

    const QSizeF textSize = m_label.textSize();

    p.rx() += m_displacement.x();
    p.ry() += m_displacement.y();

    p.rx() -= textSize.width() / 2;
    p.ry() -= textSize.height() / 2;

    if (distanceX) *distanceX = qAbs(p.x()-pos.x());
    if (distanceY) *distanceY = qAbs(p.y()-pos.y());

    return QRectF(p.x(),
                  p.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

void PointLabel::setLabel(const QwtText &label)
{DDD;
    if (m_label == label) return;
    m_label = label;
    itemChanged();
}

void PointLabel::draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap,
                      const QRectF &canvasRect) const
{DDD;
    Q_UNUSED(canvasRect);
    auto origin = getOrigin();
    QPointF pos(xMap.transform(origin.x), yMap.transform(origin.y));

    const QSizeF textSize = m_label.textSize(painter->font());

    const QRectF markerRect(-5, -5, 10, 10);
    painter->translate(pos.x(), pos.y());
    m_marker.drawSymbol(painter, markerRect);

//    painter->translate(pos.x(), pos.y());
    painter->translate(m_displacement.x()-textSize.width() / 2, m_displacement.y()-textSize.height() / 2);
    const QRectF textRect(0, 0, textSize.width(), textSize.height());
    m_label.draw(painter, textRect);
}

void PointLabel::remove()
{DDD;
    if (m_curve) m_curve->removeLabel(this);
}

void PointLabel::moveToPos(QPoint pos, QPoint startPos)
{DDD;
    auto delta = pos-startPos;

    m_displacement.rx() += delta.x();
    m_displacement.ry() += delta.y();
    itemChanged();
}

QList<QAction *> PointLabel::actions()
{DDD;
    QList<QAction *> l;

    auto a = new QAction("Показывать", m_plot);
    QMenu *m = new QMenu();
    QActionGroup *ag = new QActionGroup(m_plot);

    auto a1 = m->addAction("значение по оси X", [=](){
        if (Settings::getSetting("pointLabelRemember", true).toBool())
            Settings::setSetting("pointLabelMode", 0);
        setMode(Mode::XValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XValue);
    ag->addAction(a1);

    a1 = m->addAction("значения по осям X и Y", [=](){
        if (Settings::getSetting("pointLabelRemember", true).toBool())
            Settings::setSetting("pointLabelMode", 1);
        setMode(Mode::XYValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XYValue);
    ag->addAction(a1);

    a1 = m->addAction("значение по оси Y", [=](){
        if (Settings::getSetting("pointLabelRemember", true).toBool())
            Settings::setSetting("pointLabelMode", 2);
        setMode(Mode::YValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::YValue);
    ag->addAction(a1);

    a1 = m->addAction("значение по осям X, Y и Z", [=](){
        if (Settings::getSetting("pointLabelRemember", true).toBool())
            Settings::setSetting("pointLabelMode", 3);
        setMode(Mode::XYZValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XYZValue);
    ag->addAction(a1);

    a->setMenu(m);
    l << a;

    a = new QAction("Запомнить выбор", m_plot);
    a->setCheckable(true);
    a->setChecked(Settings::getSetting("pointLabelRemember", true).toBool());
    QObject::connect(a, &QAction::triggered, [=](){
        bool r = Settings::getSetting("pointLabelRemember", true).toBool();
        Settings::setSetting("pointLabelRemember", !r);
    });
    l<<a;

    return l;
}

bool PointLabel::contains(const QPoint &pos)
{DDD;
    auto origin = getOrigin();

    QPointF point(m_plot->transform(m_curve->xAxis(), origin.x),
                m_plot->transform(m_curve->yAxis(), origin.y));

    const QSizeF textSize = m_label.textSize();

    point.rx() += m_displacement.x();
    point.ry() += m_displacement.y();

    point.rx() -= textSize.width() / 2;
    point.ry() -= textSize.height() / 2;

    return QRectF(point.x(),
                  point.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

SamplePoint PointLabel::check(SamplePoint point)
{
    if (qIsNaN(point.z)) return point;
    return {point.x, point.z, point.y};
}

void PointLabel::updateLabel()
{DDD;
    auto origin = getOrigin();
    switch (m_mode) {
        case Mode::XValue: m_label.setText(smartDouble(origin.x)); break;
        case Mode::XYValue: m_label.setText(QString("%1; %2")
                                  .arg(smartDouble(origin.x))
                                  .arg(smartDouble(origin.y))); break;
        case Mode::YValue: m_label.setText(smartDouble(origin.y)); break;
        case Mode::XYZValue: m_label.setText(QString("%1; %2; %3")
                                  .arg(smartDouble(origin.x))
                                  .arg(smartDouble(origin.y))
                                  .arg(smartDouble(origin.z))); break;
    }
    m_label.setBorderPen(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    setTitle(m_label);
    itemChanged();
}
