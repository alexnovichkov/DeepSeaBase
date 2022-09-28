#include "pointlabel.h"

#include "qwt_scale_map.h"
#include "qwt_plot_item.h"
#include "qwt_text.h"
#include "qwt_symbol.h"
#include "logging.h"
#include <QPen>
#include <QPainter>
#include <QVector2D>
#include <QMenu>
#include "curve.h"
#include "settings.h"
#include "pointmarker.h"
#include "algorithms.h"
#include "qwtplotimpl.h"
#include <qwt_plot.h>
#include "qwtplotimpl.h"
#include "plot.h"

class LabelImpl {
public:
    virtual ~LabelImpl() {}
    virtual void attachTo(Plot *plot) = 0;
    virtual void detachFrom(Plot *plot) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setBrush(const QBrush &brush) = 0;
    virtual void setXAxis(Enums::AxisType axis) = 0;
    virtual void setYAxis(Enums::AxisType axis) = 0;
    virtual void setLabel(const QString &label) = 0;
    virtual void setBorder(const QPen &pen) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual QSizeF textSize() const = 0;
    virtual void update() = 0;
};

class QwtLabelImpl : public QwtPlotItem, public LabelImpl
{
public:
    QwtLabelImpl(PointLabel *parent) : QwtPlotItem(),
        m_marker{QwtSymbol::Ellipse, QBrush(Qt::gray), QColor(Qt::black), QSize(16,16)},
        parent(parent)
    {
        setZ(40.0);

    }

private:
    QwtText m_label;
    QwtSymbol m_marker;
    PointLabel *parent;

    // LabelImpl interface
public:
    virtual void attachTo(Plot *plot) override;
    virtual void detachFrom(Plot *plot) override;
    virtual void setColor(const QColor &color) override;
    virtual void setBrush(const QBrush &brush) override;
    virtual void setXAxis(Enums::AxisType axis) override;
    virtual void setYAxis(Enums::AxisType axis) override;
    virtual void setLabel(const QString &label) override;
    virtual void setBorder(const QPen &pen) override;
    virtual QSizeF textSize() const override;
    virtual void update() override;
    virtual void setVisible(bool visible) override;

    // QwtPlotItem interface
public:
    virtual int rtti() const override
    {
        return QwtPlotItem::Rtti_PlotUserItem+1;
    }
    virtual void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const override;
};

void QwtLabelImpl::attachTo(Plot *plot)
{
    if (auto qwt = dynamic_cast<QwtPlot*>(plot->impl()))
        QwtPlotItem::attach(qwt);
}

void QwtLabelImpl::detachFrom(Plot *plot)
{
    Q_UNUSED(plot);
    QwtPlotItem::detach();
}

void QwtLabelImpl::setColor(const QColor &color)
{
    m_marker.setColor(color);
}

void QwtLabelImpl::setBrush(const QBrush &brush)
{
    m_label.setBackgroundBrush(brush);
}

void QwtLabelImpl::setXAxis(Enums::AxisType axis)
{
    QwtPlotItem::setXAxis(toQwtAxisType(axis));
}

void QwtLabelImpl::setYAxis(Enums::AxisType axis)
{
    QwtPlotItem::setYAxis(toQwtAxisType(axis));
}

void QwtLabelImpl::setLabel(const QString &label)
{
    setTitle(label);
    itemChanged();
}

void QwtLabelImpl::setBorder(const QPen &pen)
{
    m_label.setBorderPen(pen);
}

QSizeF QwtLabelImpl::textSize() const
{
    return m_label.textSize();
}

void QwtLabelImpl::update()
{
    itemChanged();
}

void QwtLabelImpl::setVisible(bool visible)
{
    QwtPlotItem::setVisible(visible);
}

void QwtLabelImpl::draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const
{
    Q_UNUSED(canvasRect);
    auto origin = parent->getOrigin();
    QPointF pos(xMap.transform(origin.x), yMap.transform(origin.y));

    const QSizeF textSize = m_label.textSize(painter->font());

    const QRectF markerRect(-5, -5, 10, 10);
    painter->translate(pos.x(), pos.y());
    m_marker.drawSymbol(painter, markerRect);

//    painter->translate(pos.x(), pos.y());
    painter->translate(parent->getDisplacement().x()-textSize.width() / 2,
                       parent->getDisplacement().y()-textSize.height() / 2);
    const QRectF textRect(0, 0, textSize.width(), textSize.height());
    m_label.draw(painter, textRect);
}

PointLabel::PointLabel(Plot *parent, Curve *curve)
    : m_displacement(QPoint(0, -13)),
      m_plot(parent),
      m_curve(curve),
      m_impl(new QwtLabelImpl(this))
{DDD;
    if (Settings::getSetting("pointLabelRemember", true).toBool()) {
        switch (Settings::getSetting("pointLabelMode", 0).toInt()) {
            case 0: m_mode = Mode::XValue; break;
            case 1: m_mode = Mode::XYValue; break;
            case 2: m_mode = Mode::YValue; break;
            case 3: m_mode = Mode::XYZValue; break;
        }
    }
    if (m_impl) m_impl->setColor(m_curve->pen().color());
    if (m_impl) m_impl->setBrush(Qt::white);
    if (m_impl) m_impl->setXAxis(curve->xAxis());
    if (m_impl) m_impl->setYAxis(curve->yAxis());
    updateLabel();
}

PointLabel::~PointLabel()
{DDD;

}

SamplePoint PointLabel::getOrigin() const
{
    auto val = m_curve->samplePoint(m_point);
    if (qIsNaN(val.z)) return val;
    return {val.x, val.z, val.y};
}

QPoint PointLabel::getDisplacement() const
{
    return m_displacement;
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

//QwtText PointLabel::label() const
//{DDD;
//    return m_label;
//}

void PointLabel::attachTo(Plot *plot)
{
    if (m_impl) m_impl->attachTo(plot);
}

void PointLabel::detachFrom(Plot *plot)
{
    if (m_impl) m_impl->detachFrom(plot);
}

void PointLabel::updateSelection(SelectedPoint point)
{DDD;
    Q_UNUSED(point);
    if (m_impl) m_impl->setBorder(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    if (m_impl) m_impl->update();
}

bool PointLabel::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DDD;
    Q_UNUSED(point);
    auto origin = getOrigin();
    QPointF p(m_plot->plotToScreenCoordinates(m_curve->xAxis(), origin.x),
                m_plot->plotToScreenCoordinates(m_curve->yAxis(), origin.y));

    const QSizeF textSize = m_impl?m_impl->textSize():QSizeF();

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

void PointLabel::setLabel(const QString &label)
{DDD;
    if (m_impl) m_impl->setLabel(label);
}

void PointLabel::setXAxis(Enums::AxisType axis)
{
    if (m_impl) m_impl->setXAxis(axis);
}

void PointLabel::setYAxis(Enums::AxisType axis)
{
    if (m_impl) m_impl->setYAxis(axis);
}

void PointLabel::setVisible(bool visible)
{
    if (m_impl) m_impl->setVisible(visible);
}

//QString PointLabel::label() const
//{
//    return m_impl->label();
//}

void PointLabel::remove()
{DDD;
    if (m_curve) m_curve->removeLabel(this);
}

void PointLabel::moveToPos(QPoint pos, QPoint startPos)
{DDD;
    auto delta = pos-startPos;

    m_displacement.rx() += delta.x();
    m_displacement.ry() += delta.y();
    if (m_impl) m_impl->update();
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

    QPointF point(m_plot->plotToScreenCoordinates(m_curve->xAxis(), origin.x),
                m_plot->plotToScreenCoordinates(m_curve->yAxis(), origin.y));

    const QSizeF textSize = m_impl?m_impl->textSize():QSizeF();

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
    if (!m_impl) return;
    auto origin = getOrigin();
    switch (m_mode) {
        case Mode::XValue: m_impl->setLabel(smartDouble(origin.x)); break;
        case Mode::XYValue: m_impl->setLabel(QString("%1; %2")
                                  .arg(smartDouble(origin.x))
                                  .arg(smartDouble(origin.y))); break;
        case Mode::YValue: m_impl->setLabel(smartDouble(origin.y)); break;
        case Mode::XYZValue: m_impl->setLabel(QString("%1; %2; %3")
                                  .arg(smartDouble(origin.x))
                                  .arg(smartDouble(origin.y))
                                  .arg(smartDouble(origin.z))); break;
    }
    m_impl->setBorder(selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));

    //setTitle(m_label);
    m_impl->update();
}
