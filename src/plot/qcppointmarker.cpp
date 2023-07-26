#include "qcppointmarker.h"

#include "qcpplot.h"
#include "graph2d.h"
#include "data2d.h"
#include "plot/curve.h"
#include "plot/plot.h"
#include "settings.h"
#include "qcptracer.h"
#include "logging.h"

PointLabel::PointLabel(QCPPlot *plot, Curve *curve)
    : QCPItemText(dynamic_cast<QCustomPlot*>(plot)), m_plot(plot), m_curve(curve)
{
    if (se->getSetting("pointLabelRemember", true).toBool()) {
        switch (se->getSetting("pointLabelMode", 0).toInt()) {
            case 0: m_mode = Mode::XValue; break;
            case 1: m_mode = Mode::XYValue; break;
            case 2: m_mode = Mode::YValue; break;
            case 3: m_mode = Mode::XYZValue; break;
        }
    }
    setBrush(QColor(255,255,255,220));
    setAntialiased(false);

    m_tracer = new QCPTracer(plot);

    m_tracer->setStyle(QCPTracer::tsSquare);
    m_tracer->setSize(8);
    m_tracer->setAntialiased(false);
    if (auto g = dynamic_cast<Graph2D*>(m_curve)) {
        m_tracer->setGraph(g);
        m_tracer->setGraphIndex(0);
    }
    else {
        m_tracer->position->setAxes(plot->xAxis, plot->yAxis);
        m_tracer->position->setType(QCPItemPosition::ptPlotCoords);
    }
//    m_tracer->setLayer("overlay");
//    setLayer("overlay");

    setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    position->setParentAnchor(m_tracer->position);
    position->setType(QCPItemPosition::ptAbsolute);
    position->setCoords({0, -10});

    m_plot->addSelectable(this);
}

void PointLabel::detachFrom(QCPPlot *plot)
{
    plot->removeSelectable(this);
    plot->removeItem(this);
    plot->removeItem(m_tracer);
}

void PointLabel::setVisible(bool visible)
{DD;
    QCPLayerable::setVisible(visible);
    m_tracer->setVisible(visible);
}

void PointLabel::setMode(PointLabel::Mode mode)
{
    if (m_mode==mode) return;
    m_mode = mode;
    updateLabel();
    m_plot->replot();
}

SamplePoint PointLabel::getOrigin() const
{
    auto val = m_curve->samplePoint(m_point);
    if (qIsNaN(val.z)) return val;
    return {val.x, val.z, val.y};
}

void PointLabel::setPoint(SelectedPoint point)
{
    m_point = point;
    if (dynamic_cast<Graph2D*>(m_curve)) {
        m_tracer->setGraphIndex(point.x);
        m_tracer->updatePosition();
    }
    else {
        auto val = m_curve->samplePoint(m_point);;
        m_tracer->position->setCoords({val.x, val.z});
    }
    updateLabel();
}

bool PointLabel::draggable() const
{
    return true;
}

void PointLabel::moveToPos(QPoint pos, QPoint startPos)
{
    auto delta = pos-startPos;
    auto coords = position->coords();
    coords.rx() += delta.x();
    coords.ry() += delta.y();
    position->setCoords(coords);
}

void PointLabel::cycle()
{
    switch (m_mode) {
        case Mode::XValue: setMode(Mode::XYValue); break;
        case Mode::XYValue: setMode(Mode::YValue); break;
        case Mode::YValue: setMode(Mode::XYZValue); break;
        case Mode::XYZValue: setMode(Mode::XValue); break;
        default: break;
    }
}

bool PointLabel::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{
    if (!visible()) return false;
    Q_UNUSED(point);

    auto origin = getOrigin();
    QPointF p(m_plot->plotToScreenCoordinates(m_curve->xAxis(), origin.x),
                  m_plot->plotToScreenCoordinates(m_curve->yAxis(), origin.y));

    const QSizeF textSize = QFontMetricsF(font()).size(Qt::TextSingleLine, text());

    p.rx() += position->coords().x();
    p.ry() += position->coords().y();

    p.rx() -= textSize.width() / 2;
    p.ry() -= textSize.height();

    if (distanceX) *distanceX = qAbs(p.x()-pos.x());
    if (distanceY) *distanceY = qAbs(p.y()-pos.y());

    return QRectF(p.x(),
                  p.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

QList<QAction *> PointLabel::actions()
{
    QList<QAction *> l;

    auto a = new QAction("Показывать", m_plot);
    QMenu *m = new QMenu();
    QActionGroup *ag = new QActionGroup(m_plot);

    auto a1 = m->addAction("значение по оси X", [=](){
        if (se->getSetting("pointLabelRemember", true).toBool())
            se->setSetting("pointLabelMode", 0);
        setMode(Mode::XValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XValue);
    ag->addAction(a1);

    a1 = m->addAction("значения по осям X и Y", [=](){
        if (se->getSetting("pointLabelRemember", true).toBool())
            se->setSetting("pointLabelMode", 1);
        setMode(Mode::XYValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XYValue);
    ag->addAction(a1);

    a1 = m->addAction("значение по оси Y", [=](){
        if (se->getSetting("pointLabelRemember", true).toBool())
            se->setSetting("pointLabelMode", 2);
        setMode(Mode::YValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::YValue);
    ag->addAction(a1);

    a1 = m->addAction("значение по осям X, Y и Z", [=](){
        if (se->getSetting("pointLabelRemember", true).toBool())
            se->setSetting("pointLabelMode", 3);
        setMode(Mode::XYZValue);
    });
    a1->setCheckable(true);
    a1->setChecked(m_mode==Mode::XYZValue);
    ag->addAction(a1);

    a->setMenu(m);
    l << a;

    a = new QAction("Запомнить выбор", m_plot);
    a->setCheckable(true);
    a->setChecked(se->getSetting("pointLabelRemember", true).toBool());
    QObject::connect(a, &QAction::triggered, [=](){
        bool r = se->getSetting("pointLabelRemember", true).toBool();
        se->setSetting("pointLabelRemember", !r);
    });
    l<<a;

    return l;
}

void PointLabel::remove()
{
    detachFrom(m_plot);
    m_plot->replot();
}

void PointLabel::updateSelection(SelectedPoint point)
{
    Q_UNUSED(point);
    setPen(Selectable::selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
}

void PointLabel::updateLabel()
{
    auto origin = getOrigin();
    double xDist = m_plot->tickDistance(Enums::AxisType::atBottom);
    double yDist = m_plot->tickDistance(Enums::AxisType::atLeft);
    double zDist = m_plot->tickDistance(Enums::AxisType::atRight);
    switch (m_mode) {
        case Mode::XValue: setText(smartDouble(origin.x, xDist)); break;
        case Mode::XYValue: setText(QString("%1; %2")
                                  .arg(smartDouble(origin.x, xDist))
                                  .arg(smartDouble(origin.y, yDist))); break;
        case Mode::YValue: setText(smartDouble(origin.y, yDist)); break;
        case Mode::XYZValue: setText(QString("%1; %2; %3")
                                  .arg(smartDouble(origin.x, xDist))
                                  .arg(smartDouble(origin.y, yDist))
                                  .arg(smartDouble(origin.z, zDist))); break;
    }
    setPen(Selectable::selected()?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));

    //setTitle(m_label);
//    update();
}

