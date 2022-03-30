#include "cursorharmonic.h"

#include "trackingcursor.h"
#include "cursorlabel.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include <QPen>
#include <QApplication>
#include <QClipboard>
#include <qwt_scale_map.h>

CursorHarmonic::CursorHarmonic(Plot *plot) : Cursor(Cursor::Type::Harmonic, Cursor::Style::Vertical, plot)
{
    cursor = new TrackingCursor(m_color, m_style, this);

    label = new CursorLabel(plot, cursor);
    label->setAxis(CursorLabel::Axis::XAxis);
    label->updateAlignment();

    for (int i=0; i< m_harmonics; ++i) {
        auto c = new TrackingCursor(m_color, Cursor::Style::Vertical, this);
        c->setLinePen(Qt::black, 0, Qt::DashDotLine);
        cursors << c;

        auto l = new CursorLabel(plot, c);
        l->setAxis(CursorLabel::Axis::XAxis);
        setShowValues(false);
        l->updateAlignment();
        labels << l;
    }
}

CursorHarmonic::~CursorHarmonic()
{
    cursor->detach();
    label->detach();
    delete cursor;
    delete label;
    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->detach();
        labels[i]->detach();
    }
    qDeleteAll(cursors);
    qDeleteAll(labels);
}

void CursorHarmonic::setColor(const QColor &color)
{
    Cursor::setColor(color);
    auto pen = cursor->linePen();
    pen.setColor(color);
    cursor->setLinePen(pen);
    for (auto &c: cursors) c->setLinePen(color, 0, Qt::DashDotLine);
}

void CursorHarmonic::moveTo(const QPointF &pos1, const QPointF &pos2)
{
    Q_UNUSED(pos2);
    moveTo(pos1);
}

void CursorHarmonic::moveTo(const QPointF &pos1)
{
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    if (label) label->updateLabel(m_showValues);

    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->moveTo(pos.x()*(i+2));
        labels[i]->updateLabel(m_showValues);
    }
    emit cursorPositionChanged();
}

void CursorHarmonic::moveTo(const QPointF &pos1, TrackingCursor *source)
{
    if (source == cursor) moveTo(pos1);
}

void CursorHarmonic::moveTo(Qt::Key key, int count, TrackingCursor *source)
{
    if (count == 0 || source != cursor) return;
    QPointF pos = cursor->value();

    double rangeX = m_plot->canvasMap(cursor->xAxis()).sDist();
    double rangeY = m_plot->canvasMap(cursor->yAxis()).sDist();

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
//        case Qt::Key_Up: pos.ry() += count*rangeY/100; break;
//        case Qt::Key_Down: pos.ry() -= count*rangeY/100; break;
        default: break;
    }

    moveTo(pos);
}

void CursorHarmonic::updatePos()
{
    moveTo(correctedPos(cursor->value()));
}

void CursorHarmonic::attach()
{
    cursor->attach(m_plot);
    label->attach(m_plot);
    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->attach(m_plot);
        labels[i]->attach(m_plot);
    }
}

void CursorHarmonic::detach()
{
    cursor->detach();
    label->detach();
    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->detach();
        labels[i]->detach();
    }
}

bool CursorHarmonic::contains(Selectable *selected) const
{
    if (auto c = dynamic_cast<TrackingCursor*>(selected))
        return c == cursor;
    else if (auto l = dynamic_cast<CursorLabel*>(selected))
        return l == label;

    return false;
}

void CursorHarmonic::update()
{
    label->updateLabel(m_showValues);
    if (cursors.size() != m_harmonics) {
        while (cursors.size() > m_harmonics) {
            delete cursors.takeLast();
            delete labels.takeLast();
        }
        while (cursors.size() < m_harmonics) {
            auto c = new TrackingCursor(m_color, Cursor::Style::Vertical, this);
            c->setLinePen(Qt::black, 0, Qt::DashDotLine);
            c->attach(m_plot);
            cursors << c;

            auto l = new CursorLabel(m_plot, c);
            l->setAxis(CursorLabel::Axis::XAxis);
            setShowValues(false);
            l->updateAlignment();
            l->attach(m_plot);
            labels << l;
        }
    }
    for (auto &l: labels) l->updateLabel(m_showValues);
}

//QStringList CursorHarmonic::getValues() const
//{
//    QStringList list;

//    list << QString("\t%1").arg(cursor->xValue(), 0, 'f', 1);

//    auto curves = m_plot->model()->curves();
//    for (auto curve: curves) {
//        bool success = false;
//        auto val = curve->channel->data()->YforXandZ(cursor->xValue(), 0, success);
//        list << QString("%1\t%2").arg(curve->channel->name()).arg(success?val:qQNaN(), 0, 'f', 1);
//    }

//    return list;
//}

QStringList CursorHarmonic::dataHeader(bool allData) const
{
    Q_UNUSED(allData);
    return {QString::number(cursor->xValue(), 'f', 2)};
}

QList<double> CursorHarmonic::data(int curve, bool allData) const
{
    Q_UNUSED(allData);
    auto curves = m_plot->model()->curves();
    bool success = false;
    return {curves.at(curve)->channel->data()->YforXandZ(cursor->xValue(), 0, success)};
}
