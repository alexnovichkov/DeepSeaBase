#include "cursors.h"

#include "plot.h"
#include "plotmodel.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "qcpcursorsingle.h"
#include "qcpcursordouble.h"
#include "qwtcursorharmonic.h"
#include "qwt_scale_map.h"
#include "logging.h"

Cursors::Cursors(Plot *parent) : QObject(parent), plot{parent}
{DDD;
}

void Cursors::update()
{DDD;
    for (auto c: m_cursors) c->update();
}

void Cursors::addDoubleCursor(const QPoint &pos, Cursor::Style style, bool reject)
{DDD;
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
}

void Cursors::addSingleCursor(const QPoint &pos, Cursor::Style style)
{DDD;
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
}

void Cursors::addDoubleCursor(const QPoint &pos, Cursor::Style style)
{DDD;
    addDoubleCursor(pos, style, false);
}

void Cursors::addRejectCursor(const QPoint &pos, Cursor::Style style)
{DDD;
    addDoubleCursor(pos, style, true);
}

void Cursors::addHarmonicCursor(const QPoint &pos)
{DDD;
//    auto c = new QwtCursorHarmonic(plot);
//    connect(c,SIGNAL(cursorPositionChanged()), this, SIGNAL(cursorPositionChanged()));
//    connect(c, SIGNAL(dataChanged()), this, SIGNAL(cursorsChanged()));
//    double xVal = 0.0;
//    double yVal = 0.0;
//    if (plot) {
//        xVal = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, pos.x());
//    }
//    c->attach();
//    c->moveTo({xVal, yVal});
//    m_cursors << c;
//    emit cursorsChanged();
}

int Cursors::dataCount() const
{DDD;
    int sum = 0;
    for (auto c: m_cursors) sum += c->dataCount(true);
    return sum;
}

QStringList Cursors::dataHeader() const
{DDD;
    QStringList l;
    l << "Канал";
    for (auto c: m_cursors) l << c->dataHeader(true);
    return l;
}

QStringList Cursors::data(int curveIndex) const
{DDD;
    QStringList l;
    l << plot->model()->curve(curveIndex)->title();
    for (auto c: m_cursors) {
        auto d = c->data(curveIndex, true);
        for (auto v: d) l << QString::number(v, c->format()==Cursor::Format::Fixed?'f':'e', c->digits());
    }
    return l;
}

void Cursors::removeCursor(Selectable *selected)
{DDD;
    for (int i=m_cursors.size()-1; i>=0; --i) {
        auto c = m_cursors.at(i);
        if (c->contains(selected)) {
            if (c->type()==Cursor::Type::DoubleReject) {
                //remove registered reject cursor
                for (auto cu: m_cursors) {
                    if (cu->type()==Cursor::Type::Double) cu->removeRejectCursor(c);
                }
            }
            delete m_cursors.takeAt(i);
            emit cursorsChanged();
            return;
        }
    }
}

