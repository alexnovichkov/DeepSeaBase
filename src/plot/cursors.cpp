#include "cursors.h"

#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "qcpcursorsingle.h"
#include "qcpcursordouble.h"
#include "qcpcursorharmonic.h"
#include "logging.h"

Cursors::Cursors(Plot *parent) : QObject(parent), plot{parent}
{DD;
}

void Cursors::update()
{DD;
    for (auto c: m_cursors) c->update();
}

Cursor *Cursors::addDoubleCursor(const QPoint &pos, Cursor::Style style, bool reject)
{DD;
    auto c = new QCPCursorDouble(style, reject, plot);
    connect(c, SIGNAL(cursorPositionChanged()), this, SIGNAL(cursorPositionChanged()));
    connect(c, SIGNAL(dataChanged()), this, SIGNAL(cursorsChanged()));
    double xVal1 = 0.0;
    double xVal2 = 0.0;
    double yVal = 0.0;
    if (plot) {
        auto range = plot->plotRange(Enums::AxisType::atBottom);
        xVal1 = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, pos.x());
        xVal2 = qMin(xVal1 + range.dist()/10, range.dist()*9/10);
    }
    c->attach();
    c->moveTo({xVal1, yVal}, {xVal2, yVal});
    m_cursors << c;
    emit cursorsChanged();
    if (reject) {
        //register reject cursors for all standard double cursors
        for (auto cu: m_cursors) {
            if (cu->type()==Cursor::Type::Double) cu->addRejectCursor(c);
        }
    }
    return c;
}

Cursor *Cursors::addSingleCursor(const QPoint &pos, Cursor::Style style)
{DD;
    auto c = new QCPCursorSingle(style, plot);
    connect(c,SIGNAL(cursorPositionChanged()), this, SIGNAL(cursorPositionChanged()));
    connect(c, SIGNAL(dataChanged()), this, SIGNAL(cursorsChanged()));
    double xVal = 0.0;
    double yVal = 0.0;
    if (plot) {
        xVal = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, pos.x());
    }
    c->attach();
    c->moveTo({xVal, yVal});
    m_cursors << c;
    emit cursorsChanged();
    return c;
}

Cursor *Cursors::addDoubleCursor(const QPoint &pos, Cursor::Style style)
{DD;
    return addDoubleCursor(pos, style, false);
}

Cursor *Cursors::addRejectCursor(const QPoint &pos, Cursor::Style style)
{DD;
    return addDoubleCursor(pos, style, true);
}

Cursor *Cursors::addHarmonicCursor(const QPoint &pos)
{DD;
    auto c = new QCPCursorHarmonic(plot);
    connect(c,SIGNAL(cursorPositionChanged()), this, SIGNAL(cursorPositionChanged()));
    connect(c, SIGNAL(dataChanged()), this, SIGNAL(cursorsChanged()));
    double xVal = 0.0;
    double yVal = 0.0;
    if (plot) {
        xVal = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, pos.x());
    }
    c->attach();
    c->moveTo({xVal, yVal});
    m_cursors << c;
    emit cursorsChanged();
    return c;
}

int Cursors::dataCount() const
{DD;
    int sum = 0;
    for (auto c: m_cursors) sum += c->dataCount(true);
    return sum;
}

QStringList Cursors::dataHeader() const
{DD;
    QStringList l;
    l << "Канал";
    for (auto c: m_cursors) l << c->dataHeader(true);
    return l;
}

QStringList Cursors::data(int curveIndex) const
{DD;
    QStringList l;
    l << plot->model()->curve(curveIndex)->title();
    for (auto c: m_cursors) {
        auto d = c->data(curveIndex, true);
        for (auto v: d) l << QString::number(v, c->format()==Cursor::Format::Fixed?'f':'e', c->digits());
    }
    return l;
}

Cursor *Cursors::cursorFor(Selectable *selected) const
{
    for (int i = m_cursors.size()-1; i >= 0; --i) {
        auto c = m_cursors.at(i);
        if (c->contains(selected)) return c;
    }
    return nullptr;
}

void Cursors::removeCursor(Cursor *cursor)
{DD;
    if (cursor->type()==Cursor::Type::DoubleReject) {
        //remove registered reject cursor
        for (auto cu: m_cursors) {
            if (cu->type()==Cursor::Type::Double) cu->removeRejectCursor(cursor);
        }
    }
    m_cursors.removeOne(cursor);
    delete cursor;
    emit cursorsChanged();
}

