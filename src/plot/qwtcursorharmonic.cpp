#include "qwtcursorharmonic.h"

#include "trackingcursor.h"
#include "cursorlabel.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include <QPen>
#include <QApplication>
#include <QClipboard>
#include <qwt_scale_map.h>
#include <qwt_plot.h>
#include "plotinterface.h"
#include "logging.h"

//QwtCursorHarmonic::QwtCursorHarmonic(Plot *plot) : Cursor(Cursor::Type::Harmonic, Cursor::Style::Vertical, plot)
//{DDD;
//    cursor = new TrackingCursor(m_color, m_style, this);

//    label = new CursorLabel(plot, cursor);
//    label->setAxis(CursorLabel::Axis::XAxis);
//    label->updateAlignment();

//    for (int i=0; i< m_harmonics; ++i) {
//        auto c = new TrackingCursor(m_color, Cursor::Style::Vertical, this);
//        c->setLinePen(Qt::black, 0, Qt::DashDotLine);
//        cursors << c;

//        auto l = new CursorLabel(plot, c);
//        l->setAxis(CursorLabel::Axis::XAxis);
//        setShowValues(false);
//        l->updateAlignment();
//        labels << l;
//    }
//}

//QwtCursorHarmonic::~QwtCursorHarmonic()
//{DDD;
//    cursor->detach();
//    label->detach();
//    delete cursor;
//    delete label;
//    for (int i=0; i<m_harmonics; ++i) {
//        cursors[i]->detach();
//        labels[i]->detach();
//    }
//    qDeleteAll(cursors);
//    qDeleteAll(labels);
//}

//void QwtCursorHarmonic::setColor(const QColor &color)
//{DDD;
//    Cursor::setColor(color);
//    auto pen = cursor->linePen();
//    pen.setColor(color);
//    cursor->setLinePen(pen);
//    for (auto &c: cursors) c->setLinePen(color, 0, Qt::DashDotLine);
//}

//void QwtCursorHarmonic::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
//{DDD;
//    Q_UNUSED(pos2);
//    moveTo(pos1, silent);
//}

//void QwtCursorHarmonic::moveTo(const QPointF &pos1, bool silent)
//{DDD;
//    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

//    cursor->moveTo(pos);
//    if (label) label->updateLabel(m_showValues);

//    for (int i=0; i<m_harmonics; ++i) {
//        cursors[i]->moveTo(pos.x()*(i+2));
//        labels[i]->updateLabel(m_showValues);
//    }
//    if (!silent) emit cursorPositionChanged();
//}

//void QwtCursorHarmonic::moveTo(const QPointF &pos1, TrackingCursor *source, bool silent)
//{DDD;
//    if (source == cursor) moveTo(pos1, silent);
//}

//void QwtCursorHarmonic::moveTo(Qt::Key key, int count, TrackingCursor *source, bool silent)
//{DDD;
//    if (count == 0 || source != cursor) return;
//    QPointF pos = cursor->value();

//    auto dist = m_plot->plotRange(Enums::AxisType::atBottom).dist();

//    switch (key) {
//        case Qt::Key_Left: {
//            if (m_snapToValues) {
//                pos = correctedPos(pos, -count, 0);
//            }
//            else
//                pos.rx() -= count*dist/100;
//            break;
//        }
//        case Qt::Key_Right: {
//            if (m_snapToValues) {
//                pos = correctedPos(pos, count, 0);
//            }
//            else
//                pos.rx() += count*dist/100;
//            break;
//        }
////        case Qt::Key_Up: pos.ry() += count*rangeY/100; break;
////        case Qt::Key_Down: pos.ry() -= count*rangeY/100; break;
//        default: break;
//    }

//    moveTo(pos, silent);
//}

//void QwtCursorHarmonic::updatePos()
//{DDD;
//    moveTo(correctedPos(cursor->value()));
//}

//void QwtCursorHarmonic::attach()
//{DDD;
//    if (auto qwt = dynamic_cast<QwtPlot*>(m_plot->impl())) {
//        cursor->attach(qwt);
//        label->attach(qwt);
//        for (int i=0; i<m_harmonics; ++i) {
//            cursors[i]->attach(qwt);
//            labels[i]->attach(qwt);
//        }
//    }
//}

//void QwtCursorHarmonic::detach()
//{DDD;
//    cursor->detach();
//    label->detach();
//    for (int i=0; i<m_harmonics; ++i) {
//        cursors[i]->detach();
//        labels[i]->detach();
//    }
//}

//bool QwtCursorHarmonic::contains(Selectable *selected) const
//{DDD;
//    if (auto c = dynamic_cast<TrackingCursor*>(selected))
//        return c == cursor;
//    else if (auto l = dynamic_cast<CursorLabel*>(selected))
//        return l == label;

//    return false;
//}

//void QwtCursorHarmonic::update()
//{DDD;
//    label->updateLabel(m_showValues);
//    if (cursors.size() != m_harmonics) {
//        while (cursors.size() > m_harmonics) {
//            delete cursors.takeLast();
//            delete labels.takeLast();
//        }
//        while (cursors.size() < m_harmonics) {
//            auto c = new TrackingCursor(m_color, Cursor::Style::Vertical, this);
//            c->setLinePen(Qt::black, 0, Qt::DashDotLine);
//            if (auto qwt = dynamic_cast<QwtPlot*>(m_plot->impl())) c->attach(qwt);
//            cursors << c;

//            auto l = new CursorLabel(m_plot, c);
//            l->setAxis(CursorLabel::Axis::XAxis);
//            setShowValues(false);
//            l->updateAlignment();
//            if (auto qwt = dynamic_cast<QwtPlot*>(m_plot->impl())) l->attach(qwt);
//            labels << l;
//        }
//    }
//    for (auto &l: labels) l->updateLabel(m_showValues);
//}

////QStringList CursorHarmonic::getValues() const
////{
////    QStringList list;

////    list << QString("\t%1").arg(cursor->xValue(), 0, 'f', 1);

////    auto curves = m_plot->model()->curves();
////    for (auto curve: curves) {
////        bool success = false;
////        auto val = curve->channel->data()->YforXandZ(cursor->xValue(), 0, success);
////        list << QString("%1\t%2").arg(curve->channel->name()).arg(success?val:qQNaN(), 0, 'f', 1);
////    }

////    return list;
////}

//QStringList QwtCursorHarmonic::dataHeader(bool allData) const
//{DDD;
//    Q_UNUSED(allData);
//    return {/*"", "Время, с", QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor->xValue())};
//}

//QList<double> QwtCursorHarmonic::data(int curve, bool allData) const
//{DDD;
//    Q_UNUSED(allData);
//    auto curves = m_plot->model()->curves();
//    bool success = false;
////    for (int i=0; i<curves.at(curve)->channel->data()->blocksCount(); ++i) {
////        result << curves.at(curve)->channel->data()->zValue(i) <<
////             curves.at(curve)->channel->data()->YforXandZ(cursor->xValue(),
////                                                          curves.at(curve)->channel->data()->zValue(i),
////                                                          success);
////    }
//    double zval = curves.at(curve)->channel->data()->blocksCount()>1?cursor->yValue():0;
//    return {curves.at(curve)->channel->data()->YforXandZ(cursor->xValue(), zval, success)};
//}

//QPointF QwtCursorHarmonic::currentPosition() const
//{DDD;
//    return cursor->value();
//}
