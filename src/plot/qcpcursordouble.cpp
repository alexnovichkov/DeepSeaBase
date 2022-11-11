#include "qcpcursordouble.h"

#include "qcptrackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "curve.h"
#include "logging.h"
#include "qcpaxistag.h"
#include "qcpplot.h"
#include "algorithms.h"

class QCPZone : public QCPItemRect
{
public:
    explicit QCPZone(const QColor &color, QCustomPlot *parent) : QCPItemRect(parent)
    {
        topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
        topLeft->setTypeY(QCPItemPosition::ptViewportRatio);
        topLeft->setAxes(parent->xAxis, nullptr);
        bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
        bottomRight->setTypeY(QCPItemPosition::ptViewportRatio);
        bottomRight->setAxes(parent->xAxis, nullptr);

        moveToLayer(parent->layer("overlay"), false);
        setPen(Qt::NoPen);
        setBrush(color);
    }
    void setRange(const QPointF &p1, const QPointF &p2) {
        auto min = qMin(p1.x(), p2.x());
        auto max = qMax(p1.x(), p2.x());
        topLeft->setCoords(min, -3);
        bottomRight->setCoords(max, 1);
    }
};

//возвращает СКЗ в диапазоне [x1,x2]
double rms(DataHolder *data, const QCPCursorDouble *cursor, const QList<Cursor*> &rejectCursors)
{DD;
    double cumul = 0.0;
    double x1 = cursor->interval().first;
    double x2 = cursor->interval().second;

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
            if (auto dc = dynamic_cast<QCPCursorDouble*>(c))
                return (dc->interval().first <= xval && dc->interval().second >= xval);
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
double energy(DataHolder *data, const QCPCursorDouble *cursor, const QList<Cursor*> &rejectCursors)
{DD;
    double x1 = cursor->interval().first;
    double x2 = cursor->interval().second;
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
            if (auto dc = dynamic_cast<QCPCursorDouble*>(c))
                return (dc->interval().first <= xval && dc->interval().second >= xval);
            return false;
        })) continue;

        double v2 = values[i];
        if (data->yValuesUnits() != DataHolder::YValuesUnits::UnitsQuadratic)
            v2 *= values[i];
        cumul += v2;
    }

    return DataHolder::toLog(cumul, data->threshold(), DataHolder::YValuesUnits::UnitsQuadratic);
}

QCPCursorDouble::QCPCursorDouble(Cursor::Style style, bool reject, Plot *plot)
    : Cursor(reject?Cursor::Type::DoubleReject:Cursor::Type::Double, style, plot), m_plot(plot)
{
    m_cursor1 = new QCPTrackingCursor(m_color, style, this);
    m_cursor2 = new QCPTrackingCursor(m_color, style, this);

    m_axisTag1 = new QCPAxisTag(plot, m_cursor1, plot->impl()->xAxis);
    m_axisTag2 = new QCPAxisTag(plot, m_cursor2, plot->impl()->xAxis);

    plot->addSelectable(m_cursor1);
    plot->addSelectable(m_cursor2);

    m_zone = new QCPZone(reject?QColor(182,131,64,50):QColor(64,131,182,50), plot->impl());

    setColor(reject?QColor(150,40,40):QColor(40,40,150));
}

QCPCursorDouble::~QCPCursorDouble()
{
    detach();
    delete m_cursor1;
    delete m_cursor2;
}

void QCPCursorDouble::setColor(const QColor &color)
{
    Cursor::setColor(color);
    m_cursor1->setColor(color);
    m_cursor2->setColor(color);
}

void QCPCursorDouble::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;
    m_cursor1->moveTo(pos);
    if (m_axisTag1) m_axisTag1->updatePosition(pos.x());

    pos = m_snapToValues ? correctedPos(pos2) : pos2;
    m_cursor2->moveTo(pos);
    if (m_axisTag2) m_axisTag2->updatePosition(pos.x());

    if (!silent) emit cursorPositionChanged();

    m_zone->setRange(m_cursor1->value(), m_cursor2->value());
    update();
}

void QCPCursorDouble::moveTo(const QPointF &pos1, bool silent)
{
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;
    auto delta = pos.x() - m_cursor1->xValue();

    auto pos2 = m_cursor2->value();
    pos2.rx() += delta;

    moveTo(pos, pos2, silent);
}

void QCPCursorDouble::moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent)
{
    if (source == m_cursor2) moveTo(m_cursor1->value(), pos1, silent);
    if (source == m_cursor1) moveTo(pos1, m_cursor2->value(), silent);
}

void QCPCursorDouble::moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent)
{
    if (count == 0) return;
    QPointF pos = source->value();
    double rangeX = m_plot->plotRange(Enums::AxisType::atBottom).dist();

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

    if (source == m_cursor1) m_axisTag1->updatePosition(pos.x());
    if (source == m_cursor2) m_axisTag2->updatePosition(pos.x());

    m_zone->setRange(m_cursor1->value(), m_cursor2->value());
    update();
}

void QCPCursorDouble::updatePos()
{
    auto pos = m_cursor1->value();
    pos = correctedPos(pos);
    m_cursor1->moveTo(pos);

    pos = m_cursor2->value();
    pos = correctedPos(pos);
    m_cursor2->moveTo(pos);

    update();
}

void QCPCursorDouble::attach()
{

}

void QCPCursorDouble::detach()
{
    m_plot->removeSelectable(m_cursor1);
    m_plot->removeSelectable(m_cursor2);
    m_cursor1->detach();
    m_cursor2->detach();
    m_plot->impl()->removeItem(m_zone);
    if (m_axisTag1) m_axisTag1->detach();
    if (m_axisTag2) m_axisTag2->detach();
}

bool QCPCursorDouble::contains(Selectable *selected) const
{
    if (auto c = dynamic_cast<QCPTrackingCursor*>(selected))
        return c == m_cursor1 || c == m_cursor2;
    else if (auto l = dynamic_cast<QCPAxisTag*>(selected))
        return l == m_axisTag1 || l == m_axisTag2;

    return false;
}

void QCPCursorDouble::update()
{
    if (m_axisTag1) m_axisTag1->updateLabel(m_showValues);
    if (m_axisTag2) m_axisTag2->updateLabel(m_showValues);
    m_plot->impl()->layer("overlay")->replot();
}

int QCPCursorDouble::dataCount(bool allData) const
{
    int m=2;
    if (allData) {
        if (m_info & Cursor::RMS) m++;
        if (m_info & Cursor::Energy) m++;
        if (m_info & Cursor::Reject) m++;
    }
    return m;
}

QStringList QCPCursorDouble::dataHeader(bool allData) const
{
    QStringList list;
    //list << "" << "Время, с";
    list << /*QString("Частота ")+*/QLocale(QLocale::Russian).toString(m_cursor1->xValue());
    list << /*QString("Частота ")+*/QLocale(QLocale::Russian).toString(m_cursor2->xValue());
    if (allData) {
        if (m_info & Cursor::RMS) list << "СКЗ";
        if (m_info & Cursor::Energy) list << "Энергия";
        if (m_info & Cursor::Reject) list << "Режекция";
    }
    return list;
}

QList<double> QCPCursorDouble::data(int curve, bool allData) const
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
    double zval = curves.at(curve)->channel->data()->blocksCount()>1 ? m_cursor1->yValue():0;
    auto val1 = curves.at(curve)->channel->data()->YforXandZ(m_cursor1->xValue(), zval, success);
    auto val2 = curves.at(curve)->channel->data()->YforXandZ(m_cursor2->xValue(), zval, success);
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

QPointF QCPCursorDouble::currentPosition() const
{
    return m_cursor1->value();
}

QPair<double, double> QCPCursorDouble::interval() const
{
    auto range = QCPRange(m_cursor1->xValue(), m_cursor2->xValue());
    range.normalize();
    return {range.lower, range.upper};
}
