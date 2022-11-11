#include "qcpcursorharmonic.h"

#include "qcptrackingcursor.h"
#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include <QPen>
#include <QApplication>
#include <QClipboard>
#include "logging.h"
#include "qcpaxistag.h"
#include "qcpplot.h"


QCPCursorHarmonic::QCPCursorHarmonic(Plot *plot) : Cursor(Cursor::Type::Harmonic, Cursor::Style::Vertical, plot),
    plot(plot)
{DD;
    cursor = new QCPTrackingCursor(m_color, m_style, this);

    label = new QCPAxisTag(plot, cursor, plot->impl()->xAxis);

    plot->addSelectable(cursor);
    setShowValues(false);

    for (int i=0; i< m_harmonics; ++i) {
        auto c = new QCPTrackingCursor(m_color, Cursor::Style::Vertical, this);
        c->setPen(QPen(Qt::black, 0, Qt::DashDotLine));
        cursors << c;

        auto l = new QCPAxisTag(plot, c, plot->impl()->xAxis);
        labels << l;
    }
}

QCPCursorHarmonic::~QCPCursorHarmonic()
{DD;
    detach();

    delete cursor;
    delete label;

    qDeleteAll(cursors);
    qDeleteAll(labels);
}

void QCPCursorHarmonic::setColor(const QColor &color)
{DD;
    Cursor::setColor(color);
    cursor->setColor(color);
    for (auto &c: cursors) c->setPen(QPen(color, 0, Qt::DashDotLine));
}

void QCPCursorHarmonic::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{DD;
    Q_UNUSED(pos2);
    moveTo(pos1, silent);
}

void QCPCursorHarmonic::moveTo(const QPointF &pos1, bool silent)
{DD;
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    if (label) label->updatePosition(pos.x());

    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->moveTo(pos.x()*(i+2));
        labels[i]->updatePosition(cursors[i]->value().x());
    }
    if (!silent) emit cursorPositionChanged();
    update();
}

void QCPCursorHarmonic::moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent)
{DD;
    if (source == cursor) moveTo(pos1, silent);
}

void QCPCursorHarmonic::moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent)
{DD;
    if (count == 0 || source != cursor) return;
    QPointF pos = cursor->value();

    auto dist = m_plot->plotRange(Enums::AxisType::atBottom).dist();

    switch (key) {
        case Qt::Key_Left: {
            if (m_snapToValues) {
                pos = correctedPos(pos, -count, 0);
            }
            else
                pos.rx() -= count*dist/100;
            break;
        }
        case Qt::Key_Right: {
            if (m_snapToValues) {
                pos = correctedPos(pos, count, 0);
            }
            else
                pos.rx() += count*dist/100;
            break;
        }
//        case Qt::Key_Up: pos.ry() += count*rangeY/100; break;
//        case Qt::Key_Down: pos.ry() -= count*rangeY/100; break;
        default: break;
    }

    moveTo(pos, silent);
}

void QCPCursorHarmonic::updatePos()
{DD;
    moveTo(correctedPos(cursor->value()));
    update();
}

void QCPCursorHarmonic::attach()
{DD;

}

void QCPCursorHarmonic::detach()
{DD;
    m_plot->removeSelectable(cursor);
    cursor->detach();
    label->detach();
    for (int i=0; i<m_harmonics; ++i) {
        cursors[i]->detach();
        labels[i]->detach();
    }
}

bool QCPCursorHarmonic::contains(Selectable *selected) const
{DD;
    if (auto c = dynamic_cast<QCPTrackingCursor*>(selected))
        return c == cursor;
    else if (auto l = dynamic_cast<QCPAxisTag*>(selected))
        return l == label;

    return false;
}

void QCPCursorHarmonic::update()
{DD;
    label->updateLabel(m_showValues);
    if (cursors.size() != m_harmonics) {
        while (cursors.size() > m_harmonics) {
            auto c = cursors.takeLast();
            c->detach();
            delete c;
            auto l = labels.takeLast();
            l->detach();
            delete l;
        }

        while (cursors.size() < m_harmonics) {
            auto c = new QCPTrackingCursor(m_color, Cursor::Style::Vertical, this);
            c->setPen(QPen(Qt::black, 0, Qt::DashDotLine));
            cursors << c;

            auto l = new QCPAxisTag(plot, c, plot->impl()->xAxis);
            labels << l;
        }
    }
    for (auto &l: labels) l->updateLabel(m_showValues);
    plot->impl()->layer("overlay")->replot();
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

QStringList QCPCursorHarmonic::dataHeader(bool allData) const
{DD;
    Q_UNUSED(allData);
    return {/*"", "Время, с", QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor->xValue())};
}

QList<double> QCPCursorHarmonic::data(int curve, bool allData) const
{DD;
    Q_UNUSED(allData);
    auto curves = m_plot->model()->curves();
    bool success = false;
//    for (int i=0; i<curves.at(curve)->channel->data()->blocksCount(); ++i) {
//        result << curves.at(curve)->channel->data()->zValue(i) <<
//             curves.at(curve)->channel->data()->YforXandZ(cursor->xValue(),
//                                                          curves.at(curve)->channel->data()->zValue(i),
//                                                          success);
//    }
    double zval = curves.at(curve)->channel->data()->blocksCount()>1?cursor->yValue():0;
    return {curves.at(curve)->channel->data()->YforXandZ(cursor->xValue(), zval, success)};
}

QPointF QCPCursorHarmonic::currentPosition() const
{DD;
    return cursor->value();
}
