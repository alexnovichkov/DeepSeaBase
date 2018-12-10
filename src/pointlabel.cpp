#include "pointlabel.h"

#include "qwt_plot.h"
#include "logging.h"

PointLabel::PointLabel(QwtPlot *parent)
    : QwtPlotItem(),
      d_point(-1),
      d_origin(QPointF(0.0, 0.0)),
      d_displacement(QPoint(0, -13)),
     // d_label(title),
      plot(parent),
      d_selected(false)
{DD;
    setZ(40.0);
    d_label.setBorderPen(d_selected?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    d_mode=0;
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
    switch (d_mode) {
        case 0: d_label = QwtText(QString::number(d_origin.x(),'f',2)); break;
        case 1: d_label = QwtText(QString("%1; %2")
                                  .arg(QString::number(d_origin.x(),'f',2))
                                  .arg(QString::number(d_origin.y(),'f',1))); break;
        case 2: d_label = QwtText(QString::number(d_origin.y(),'f',1)); break;
    }
    d_label.setBorderPen(d_selected?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    setTitle(d_label);
    itemChanged();
}

void PointLabel::setMode(int mode)
{
    if (d_mode==mode) return;

    d_mode = mode;
    switch (d_mode) {
        case 0: d_label = QwtText(QString::number(d_origin.x(),'f',2)); break;
        case 1: d_label = QwtText(QString("%1; %2")
                                  .arg(QString::number(d_origin.x(),'f',2))
                                  .arg(QString::number(d_origin.y(),'f',1))); break;
        case 2: d_label = QwtText(QString::number(d_origin.y(),'f',1)); break;
    }
    d_label.setBorderPen(d_selected?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
    setTitle(d_label);
    itemChanged();
}

void PointLabel::cycleMode()
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

QPoint PointLabel::displacement() const
{DD;
    return d_displacement;
}

void PointLabel::setDisplacement(const QPoint &displacement)
{DD;
    if (d_displacement == displacement) return;
    d_displacement = displacement;
    itemChanged();
}

void PointLabel::setDisplacement(int dx, int dy)
{DD;
    setDisplacement(QPoint(dx, dy));
}

QwtText PointLabel::label() const
{DD;
    return d_label;
}

bool PointLabel::selected() const
{DD;
    return d_selected;
}

void PointLabel::setSelected(bool selected)
{DD;
    if (d_selected != selected) {
        d_selected = selected;
        d_label.setBorderPen(selected?QPen(Qt::darkGray, 1, Qt::DashLine):QPen(Qt::NoPen));
        itemChanged();
    }
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

void PointLabel::moveBy(const QPoint &pos)
{DD;
    if (pos.x() == 0 && pos.y() == 0) return;
    d_displacement.rx() += pos.x();
    d_displacement.ry() += pos.y();
    itemChanged();
}

bool PointLabel::contains(const QPoint &pos, int yAxis)
{DD;
    QPointF point(plot->transform(QwtPlot::xBottom, d_origin.x()),
                plot->transform(/*QwtPlot::yLeft*/yAxis, d_origin.y()));

    const QSizeF textSize = d_label.textSize();

    point.rx() += d_displacement.x();
    point.ry() += d_displacement.y();

    point.rx() -= textSize.width() / 2;
    point.ry() -= textSize.height() / 2;

    return QRectF(point.x(),
                  point.y(),
                  textSize.width(), textSize.height()).contains(pos);
}
