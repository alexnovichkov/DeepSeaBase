#include "cursors.h"

#include "plot.h"
#include "cursor.h"
#include "cursorsingle.h"
#include "qwt_scale_map.h"
#include <QtDebug>

Cursors::Cursors(Plot *parent) : QObject(parent), plot{parent}
{
}

void Cursors::addSingleCursor(const QPoint &pos)
{
    auto c = new CursorSingle(Cursor::Style::Cross, plot);
    double xVal = 0.0;
    double yVal = 0.0;
    if (plot) {
        auto map = plot->canvasMap(QwtAxis::XBottom);
        xVal = map.invTransform(pos.x());
    }

    c->attach();
    c->moveTo({xVal, yVal});

    cursors << c;
}
