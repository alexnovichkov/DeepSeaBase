#include "pointmarker.h"
#include <qwt_symbol.h>
#include <qwt_text.h>

PointMarker::PointMarker(const QColor &color, QwtAxisId axis)
{
    setLineStyle(QwtPlotMarker::NoLine);
    setSymbol(new QwtSymbol(QwtSymbol::Ellipse,
                                    Qt::gray,
                                    color,
                                    QSize(8, 8))
                      );
    setLabelAlignment(Qt::AlignTop);
    setYAxis(axis);
}

void PointMarker::moveTo(const QPointF &val)
{
    setValue(val);
    setLabel(QwtText(QString::number(val.x(),'f',2)));
}
