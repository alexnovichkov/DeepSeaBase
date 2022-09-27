#include "plottracker.h"

#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include <qwt_plot.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_canvas.h>
#include "logging.h"
#include <QKeyEvent>
#include "plot.h"
//#include "trackingpanel.h"
#include "trackingcursor.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "plot/plotmodel.h"
#include "plot/cursor.h"
#include "plotinterface.h"

PlotTracker::PlotTracker(Plot *plot) :
    QwtPlotPicker(dynamic_cast<QwtPlot*>(plot->impl())->canvas()), plot(plot)
{DDD;
    impl = dynamic_cast<QwtPlot*>(plot->impl());
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

    const auto list = impl->itemList(QwtPlotItem::Rtti_PlotMarker);
    for (auto it: list) {
        if (auto *c = dynamic_cast<TrackingCursor *>(it)) {
            if (!c->isVisible()) {
                continue;
            }
            int newX = (int)(impl->transform(c->xAxis(), c->xValue()));
            int posX = (int)(impl->transform(c->xAxis(), pos.x()));
            int newY = (int)(impl->transform(c->yAxis(), c->yValue()));
            int posY = (int)(impl->transform(c->yAxis(), pos.y()));
            if (c->type == Cursor::Style::Horizontal || c->type == Cursor::Style::Cross) {
                if (qAbs(newY-posY)<=4) {
                    cursor = impl->canvas()->cursor();
                    impl->canvas()->setCursor(QCursor(Qt::SizeVerCursor));
                    found = true;
                    break;
                }
            }
            if (c->type == Cursor::Style::Vertical || c->type == Cursor::Style::Cross) {
                if (qAbs(newX-posX)<=4) {
                    cursor = impl->canvas()->cursor();
                    impl->canvas()->setCursor(QCursor(Qt::SizeHorCursor));
                    found = true;
                    break;
                }
            }

        }
    }
    if (!found) impl->canvas()->setCursor(QCursor(Qt::CrossCursor));
}
