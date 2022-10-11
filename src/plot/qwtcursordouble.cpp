#include "qwtcursordouble.h"

#include "trackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "cursorlabel.h"
#include <qwt_scale_map.h>
#include "curve.h"
#include "algorithms.h"
#include "logging.h"
#include <qwt_plot.h>
#include "plotinterface.h"
#include <qwt_interval.h>


//QwtCursorDouble::QwtCursorDouble(Cursor::Style style, bool reject, Plot *plot)
//    : Cursor(reject?Cursor::Type::DoubleReject:Cursor::Type::Double, style, plot)
//{DDD;
//    cursor1 = new TrackingCursor(m_color, style, this);
//    xlabel1 = new CursorLabel(plot, cursor1);
//    xlabel1->setAxis(CursorLabel::Axis::XAxis);
//    xlabel1->updateAlignment();

//    cursor2 = new TrackingCursor(m_color, style, this);
//    xlabel2 = new CursorLabel(plot, cursor2);
//    xlabel2->setAxis(CursorLabel::Axis::XAxis);
//    xlabel2->updateAlignment();

//    zone = new Zone(reject?QColor(182,131,64,50):QColor(64,131,182,50));

//    setColor(reject?QColor(150,40,40):QColor(40,40,150));
//}

//QwtCursorDouble::~QwtCursorDouble()
//{DDD;
//    detach();
//    delete cursor1;
//    delete cursor2;
//    delete xlabel1;
//    delete xlabel2;
//    delete zone;
//}

//void QwtCursorDouble::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
//{DDD;
//    cursor1->moveTo(m_snapToValues ? correctedPos(pos1) : pos1);
//    if (xlabel1) xlabel1->updateLabel(m_showValues);

//    cursor2->moveTo(m_snapToValues ? correctedPos(pos2) : pos2);
//    if (xlabel2) xlabel2->updateLabel(m_showValues);
//    if (!silent) emit cursorPositionChanged();

//    zone->setRange(cursor1->value(), cursor2->value());
//}

//void QwtCursorDouble::moveTo(const QPointF &pos1, bool silent)
//{DDD;
//    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;
//    auto delta = pos.x()-cursor1->xValue();

//    cursor1->moveTo(pos);
//    if (xlabel1) xlabel1->updateLabel(m_showValues);

//    auto pos2 = cursor2->value();
//    pos2.rx() += delta;
//    cursor2->moveTo(pos2);
//    if (xlabel2) xlabel2->updateLabel(m_showValues);
//    if (!silent) emit cursorPositionChanged();

//    zone->setRange(cursor1->value(), cursor2->value());
//}

//void QwtCursorDouble::moveTo(const QPointF &pos1, TrackingCursor *source, bool silent)
//{DDD;
//    if (source == cursor2) moveTo(cursor1->value(), pos1, silent);
//    if (source == cursor1) moveTo(pos1, cursor2->value(), silent);
//}

//void QwtCursorDouble::moveTo(Qt::Key key, int count, TrackingCursor *source, bool silent)
//{DDD;
//    if (count == 0) return;
//    QPointF pos = source->value();
//    double rangeX = m_plot->plotRange(Enums::AxisType::atBottom).dist();

//    switch (key) {
//        case Qt::Key_Left: {
//            if (m_snapToValues) {
//                pos = correctedPos(pos, -count, 0);
//            }
//            else
//                pos.rx() -= count*rangeX/100;
//            break;
//        }
//        case Qt::Key_Right: {
//            if (m_snapToValues) {
//                pos = correctedPos(pos, count, 0);
//            }
//            else
//                pos.rx() += count*rangeX/100;
//            break;
//        }
//        default: break;
//    }

//    source->moveTo(pos);
//    if (!silent) emit cursorPositionChanged();

//    if (source == cursor1) xlabel1->updateLabel(m_showValues);
//    if (source == cursor2) xlabel2->updateLabel(m_showValues);

//    zone->setRange(cursor1->value(), cursor2->value());
//}

//void QwtCursorDouble::updatePos()
//{DDD;
//    auto pos = cursor1->value();
//    pos = correctedPos(pos);
//    cursor1->moveTo(pos);
//    if (xlabel1) xlabel1->updateLabel(m_showValues);

//    pos = cursor2->value();
//    pos = correctedPos(pos);
//    cursor2->moveTo(pos);
//    if (xlabel2) xlabel2->updateLabel(m_showValues);

//    zone->setRange(cursor1->value(), cursor2->value());
//}



//QwtInterval QwtCursorDouble::interval() const
//{DDD;
//    return QwtInterval(cursor1->xValue(), cursor2->xValue()).normalized();
//}

////QStringList CursorDouble::getValues() const
////{
////    auto c1 = cursor1;
////    auto c2 = cursor2;
////    if (c1->xValue() > c2->xValue()) qSwap(c1,c2);

////    char f = m_format==Format::Fixed?'f':'e';

////    QStringList list;
////    list << QString("\t%1\t%2").arg(c1->xValue(), 0, f, m_digits).arg(c2->xValue(), 0, f, m_digits);

////    auto curves = m_plot->model()->curves();
////    for (auto curve: curves) {
////        bool success1 = false;
////        bool success2 = false;
////        auto val1 = curve->channel->data()->YforXandZ(c1->xValue(), 0, success1);
////        auto val2 = curve->channel->data()->YforXandZ(c2->xValue(), 0, success2);
////        list << QString("%1\t%2\t%3").arg(curve->channel->name()).arg(success1?val1:qQNaN(), 0, f, m_digits)
////                .arg(success2?val2:qQNaN(), 0, f, m_digits);
////    }

////    return list;
////}

//Zone::Zone(QColor color) : QwtPlotZoneItem()
//{DDD;
//    setOrientation(Qt::Vertical);
//    setBrush(color);
//}

//void Zone::setRange(const QPointF &p1, const QPointF &p2)
//{DDD;
//    setInterval(qMin(p1.x(), p2.x()), qMax(p1.x(), p2.x()));
//}
