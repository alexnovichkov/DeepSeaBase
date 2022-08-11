#include "plottracker.h"

#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include <qwt_plot_zoomer.h>
#include "logging.h"
#include <QKeyEvent>
#include "plot.h"
//#include "trackingpanel.h"
#include "trackingcursor.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "plot/plotmodel.h"
#include "plot/cursor.h"

PlotTracker::PlotTracker(Plot *plot) :
    QwtPlotPicker(plot->canvas()), plot(plot)
{DDD;
    setStateMachine(new QwtPickerTrackerMachine);
    setTrackerMode(QwtPicker::AlwaysOn);
    connect(this, SIGNAL(moved(QPointF)), this, SLOT(maybeHover(QPointF)));
}

PlotTracker::~PlotTracker()
{DDD;

}

// блокируем срабатывание клавиш, чтобы не сдвигался курсор мыши
void PlotTracker::widgetKeyPressEvent(QKeyEvent *e)
{DDD;
    const int key = e->key();

    if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {

    }
    else QwtPlotPicker::widgetKeyPressEvent(e);
}

QwtText PlotTracker::trackerTextF(const QPointF &pos) const
{DDD;
    QColor bg(Qt::white);
    bg.setAlpha(200);

    QwtText text;
    if (plot) text = plot->pointCoordinates(pos);
    text.setBackgroundBrush(QBrush(bg));
    return text;
}

void PlotTracker::maybeHover(const QPointF &pos)
{DDD;
    if (!plot) return;
    bool found = false;

    const auto list = plot->itemList(QwtPlotItem::Rtti_PlotMarker);
    for (auto it: list) {
        if (auto *c = dynamic_cast<TrackingCursor *>(it)) {
            if (!c->isVisible()) {
                continue;
            }
            int newX = (int)(plot->transform(c->xAxis(), c->xValue()));
            int posX = (int)(plot->transform(c->xAxis(), pos.x()));
            int newY = (int)(plot->transform(c->yAxis(), c->yValue()));
            int posY = (int)(plot->transform(c->yAxis(), pos.y()));
            if (c->type == Cursor::Style::Horizontal || c->type == Cursor::Style::Cross) {
                if (qAbs(newY-posY)<=4) {
                    cursor = plot->canvas()->cursor();
                    plot->canvas()->setCursor(QCursor(Qt::SizeVerCursor));
                    found = true;
                    break;
                }
            }
            if (c->type == Cursor::Style::Vertical || c->type == Cursor::Style::Cross) {
                if (qAbs(newX-posX)<=4) {
                    cursor = plot->canvas()->cursor();
                    plot->canvas()->setCursor(QCursor(Qt::SizeHorCursor));
                    found = true;
                    break;
                }
            }

        }
    }
    if (!found) plot->canvas()->setCursor(QCursor(Qt::CrossCursor));
}
