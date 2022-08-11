#include "pointmarker.h"
#include <qwt_symbol.h>
#include <qwt_text.h>
#include "curve.h"
#include "logging.h"

PointMarker::PointMarker(const QColor &color, QwtAxisId axis)
{DDD;
    setLineStyle(QwtPlotMarker::NoLine);
    setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                    Qt::gray,
                                    color,
                                    QSize(8, 8))
                      );
    setLabelAlignment(Qt::AlignTop);
    setYAxis(axis);
}

PointMarker::PointMarker(Curve *parent) : curve(parent)
{DDD;
    setLineStyle(QwtPlotMarker::NoLine);
    setLabelAlignment(Qt::AlignTop);
}

void PointMarker::moveTo(const QPointF &val)
{DDD;
    setValue(val);
    setLabel(QwtText(QString::number(val.x(),'f',2)));
    if (curve) {
        setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                        Qt::gray,
                                        curve->pen().color(),
                                        QSize(8, 8))
                          );
        setYAxis(curve->yAxis());
    }
}
