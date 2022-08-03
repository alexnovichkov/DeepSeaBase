#include "cursordouble.h"

#include "trackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "cursorlabel.h"
#include <qwt_scale_map.h>
#include "curve.h"
#include "algorithms.h"

//возвращает СКЗ в диапазоне [x1,x2]
double rms(DataHolder *data, const CursorDouble *cursor, const QList<Cursor*> &rejectCursors)
{
    double cumul = 0.0;
    double x1 = cursor->interval().minValue();
    double x2 = cursor->interval().maxValue();

    int left = data->floor(x1);
    int right = data->ceil(x2);
    if (left > right) std::swap(left,right);
    if (left < 0) left = 0;
    if (right < 0) return cumul;

    QVector<double> values = data->linears(0);
    if (values.isEmpty()) return cumul;
    if (right >= values.size()) right = values.size()-1;

    int count = right-left+1;

    for (int i = left; i<=right; ++i) {
        auto xval = data->xValue(i);
        if (std::any_of(rejectCursors.begin(), rejectCursors.end(), [xval](Cursor *c)
        {
            if (auto dc = dynamic_cast<CursorDouble*>(c))
                return dc->interval().contains(xval);
            return false;
        })) {
            count--;
            continue;
        }

        double v2 = values[i];
        if (data->yValuesUnits() != DataHolder::YValuesUnits::UnitsQuadratic)
            v2 *= values[i];
        cumul += v2;
    }

    return DataHolder::toLog(sqrt(cumul)/double(count), data->threshold(), DataHolder::UnitsLinear);
}

//возвращает энергию в диапазоне [x1,x2]
double energy(DataHolder *data, const CursorDouble *cursor, const QList<Cursor*> &rejectCursors)
{
    double x1 = cursor->interval().minValue();
    double x2 = cursor->interval().maxValue();
    double cumul = 0.0;



    int left = data->floor(x1);
    int right = data->ceil(x2);
    if (left > right) std::swap(left,right);
    if (left < 0) left = 0;
    if (right < 0) return cumul;

    QVector<double> values = data->linears(0);
    if (values.isEmpty()) return cumul;
    if (right >= values.size()) right = values.size()-1;

    for (int i = left; i<=right; ++i) {
        auto xval = data->xValue(i);
        if (std::any_of(rejectCursors.begin(), rejectCursors.end(), [xval](Cursor *c)
        {
            if (auto dc = dynamic_cast<CursorDouble*>(c))
                return dc->interval().contains(xval);
            return false;
        })) continue;

        double v2 = values[i];
        if (data->yValuesUnits() != DataHolder::YValuesUnits::UnitsQuadratic)
            v2 *= values[i];
        cumul += v2;
    }

    return DataHolder::toLog(cumul, data->threshold(), DataHolder::YValuesUnits::UnitsQuadratic);
}

CursorDouble::CursorDouble(Cursor::Style style, bool reject, Plot *plot)
    : Cursor(reject?Cursor::Type::DoubleReject:Cursor::Type::Double, style, plot)
{
    cursor1 = new TrackingCursor(m_color, style, this);
    xlabel1 = new CursorLabel(plot, cursor1);
    xlabel1->setAxis(CursorLabel::Axis::XAxis);
    xlabel1->updateAlignment();

    cursor2 = new TrackingCursor(m_color, style, this);
    xlabel2 = new CursorLabel(plot, cursor2);
    xlabel2->setAxis(CursorLabel::Axis::XAxis);
    xlabel2->updateAlignment();

    zone = new Zone(reject?QColor(182,131,64,50):QColor(64,131,182,50));

    setColor(reject?QColor(150,40,40):QColor(40,40,150));
}

CursorDouble::~CursorDouble()
{
    detach();
    delete cursor1;
    delete cursor2;
    delete xlabel1;
    delete xlabel2;
    delete zone;
}

void CursorDouble::setColor(const QColor &color)
{
    Cursor::setColor(color);
    auto pen = cursor1->linePen();
    pen.setColor(color);
    cursor1->setLinePen(pen);
    cursor2->setLinePen(pen);
}

void CursorDouble::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{
    cursor1->moveTo(m_snapToValues ? correctedPos(pos1) : pos1);
    if (xlabel1) xlabel1->updateLabel(m_showValues);

    cursor2->moveTo(m_snapToValues ? correctedPos(pos2) : pos2);
    if (xlabel2) xlabel2->updateLabel(m_showValues);
    if (!silent) emit cursorPositionChanged();

    zone->setRange(cursor1->value(), cursor2->value());
}

void CursorDouble::moveTo(const QPointF &pos1, bool silent)
{
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;
    auto delta = pos.x()-cursor1->xValue();

    cursor1->moveTo(pos);
    if (xlabel1) xlabel1->updateLabel(m_showValues);

    auto pos2 = cursor2->value();
    pos2.rx() += delta;
    cursor2->moveTo(pos2);
    if (xlabel2) xlabel2->updateLabel(m_showValues);
    if (!silent) emit cursorPositionChanged();

    zone->setRange(cursor1->value(), cursor2->value());
}

void CursorDouble::moveTo(const QPointF &pos1, TrackingCursor *source, bool silent)
{
    if (source == cursor2) moveTo(cursor1->value(), pos1, silent);
    if (source == cursor1) moveTo(pos1, cursor2->value(), silent);
}

void CursorDouble::moveTo(Qt::Key key, int count, TrackingCursor *source, bool silent)
{
    if (count == 0) return;
    QPointF pos = source->value();
    double rangeX = m_plot->canvasMap(source->xAxis()).sDist();

    switch (key) {
        case Qt::Key_Left: {
            if (m_snapToValues) {
                pos = correctedPos(pos, -count, 0);
            }
            else
                pos.rx() -= count*rangeX/100;
            break;
        }
        case Qt::Key_Right: {
            if (m_snapToValues) {
                pos = correctedPos(pos, count, 0);
            }
            else
                pos.rx() += count*rangeX/100;
            break;
        }
        default: break;
    }

    source->moveTo(pos);
    if (!silent) emit cursorPositionChanged();

    if (source == cursor1) xlabel1->updateLabel(m_showValues);
    if (source == cursor2) xlabel2->updateLabel(m_showValues);

    zone->setRange(cursor1->value(), cursor2->value());
}

void CursorDouble::updatePos()
{
    auto pos = cursor1->value();
    pos = correctedPos(pos);
    cursor1->moveTo(pos);
    if (xlabel1) xlabel1->updateLabel(m_showValues);

    pos = cursor2->value();
    pos = correctedPos(pos);
    cursor2->moveTo(pos);
    if (xlabel2) xlabel2->updateLabel(m_showValues);

    zone->setRange(cursor1->value(), cursor2->value());
}

void CursorDouble::attach()
{
    cursor1->attach(m_plot);
    cursor2->attach(m_plot);
    if (xlabel1) xlabel1->attach(m_plot);
    if (xlabel2) xlabel2->attach(m_plot);
    zone->attach(m_plot);
}

void CursorDouble::detach()
{
    cursor1->detach();
    cursor2->detach();
    xlabel1->detach();
    xlabel2->detach();
    zone->detach();
}

bool CursorDouble::contains(Selectable *selected) const
{
    if (auto c = dynamic_cast<TrackingCursor*>(selected))
        return c == cursor1 || c == cursor2;
    else if (auto l = dynamic_cast<CursorLabel*>(selected))
        return l == xlabel1 || l == xlabel2;

    return false;
}

void CursorDouble::update()
{
    if (xlabel1) xlabel1->updateLabel(m_showValues);
    if (xlabel2) xlabel2->updateLabel(m_showValues);
}

int CursorDouble::dataCount(bool allData) const
{
    int m=2;
    if (allData) {
        if (m_info & Cursor::RMS) m++;
        if (m_info & Cursor::Energy) m++;
        if (m_info & Cursor::Reject) m++;
    }
    return m;
}

QStringList CursorDouble::dataHeader(bool allData) const
{
    QStringList list;
    //list << "" << "Время, с";
    list << /*QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor1->xValue());
    list << /*QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor2->xValue());
    if (allData) {
        if (m_info & Cursor::RMS) list << "СКЗ";
        if (m_info & Cursor::Energy) list << "Энергия";
        if (m_info & Cursor::Reject) list << "Режекция";
    }
    return list;
}

QList<double> CursorDouble::data(int curve, bool allData) const
{
    auto curves = m_plot->model()->curves();
    bool success = false;

//    QList<QList<double> > list;
//    for (int i=0; i<curves.at(curve)->channel->data()->blocksCount(); ++i) {
//        auto val1 = curves.at(curve)->channel->data()->YforXandZ(cursor1->xValue(),
//                                                                 curves.at(curve)->channel->data()->zValue(i), success);
//        auto val2 = curves.at(curve)->channel->data()->YforXandZ(cursor2->xValue(),
//                                                                 curves.at(curve)->channel->data()->zValue(i), success);
//        QList<double> l;
//        l << curves.at(curve)->channel->data()->zValue(i) << val1 << val2;
//        if (allData) {
//            if (m_info & Cursor::RMS) l << rms(curves.at(curve)->channel->data(), this, rejectCursors);
//            if (m_info & Cursor::Energy) {
//                double e = energy(curves.at(curve)->channel->data(), this, rejectCursors);
//                l << e;
//            }
//        }
//        list << l;
//    }

    QList<double> list;
    double zval = curves.at(curve)->channel->data()->blocksCount()>1 ? cursor1->yValue():0;
    auto val1 = curves.at(curve)->channel->data()->YforXandZ(cursor1->xValue(), zval, success);
    auto val2 = curves.at(curve)->channel->data()->YforXandZ(cursor2->xValue(), zval, success);
    list << val1 << val2;
    if (allData) {
        if (m_info & Cursor::RMS) list << rms(curves.at(curve)->channel->data(), this, rejectCursors);
        if (m_info & Cursor::Energy) {
            double e = energy(curves.at(curve)->channel->data(), this, rejectCursors);
            list << e;
        }
    }
    return list;
}

QPointF CursorDouble::currentPosition() const
{
    return cursor1->position();
}

QwtInterval CursorDouble::interval() const
{
    return QwtInterval(cursor1->xValue(), cursor2->xValue()).normalized();
}

//QStringList CursorDouble::getValues() const
//{
//    auto c1 = cursor1;
//    auto c2 = cursor2;
//    if (c1->xValue() > c2->xValue()) qSwap(c1,c2);

//    char f = m_format==Format::Fixed?'f':'e';

//    QStringList list;
//    list << QString("\t%1\t%2").arg(c1->xValue(), 0, f, m_digits).arg(c2->xValue(), 0, f, m_digits);

//    auto curves = m_plot->model()->curves();
//    for (auto curve: curves) {
//        bool success1 = false;
//        bool success2 = false;
//        auto val1 = curve->channel->data()->YforXandZ(c1->xValue(), 0, success1);
//        auto val2 = curve->channel->data()->YforXandZ(c2->xValue(), 0, success2);
//        list << QString("%1\t%2\t%3").arg(curve->channel->name()).arg(success1?val1:qQNaN(), 0, f, m_digits)
//                .arg(success2?val2:qQNaN(), 0, f, m_digits);
//    }

//    return list;
//}

Zone::Zone(QColor color) : QwtPlotZoneItem()
{
    setOrientation(Qt::Vertical);
    setBrush(color);
}

void Zone::setRange(const QPointF &p1, const QPointF &p2)
{
    setInterval(qMin(p1.x(), p2.x()), qMax(p1.x(), p2.x()));
}
