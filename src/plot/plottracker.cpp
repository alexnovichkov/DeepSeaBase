#include "plottracker.h"

#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include <qwt_plot_zoomer.h>
#include "logging.h"
#include <QKeyEvent>
#include "plot.h"
#include "trackingpanel.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"

PlotTracker::PlotTracker(Plot *plot) :
    QwtPlotPicker(plot->canvas()), plot(plot)
{DD;
    setStateMachine(new QwtPickerTrackerMachine);
    setTrackerMode(QwtPicker::AlwaysOn);
    connect(this, SIGNAL(moved(QPointF)), this, SLOT(maybeHover(QPointF)));
}

PlotTracker::~PlotTracker()
{DD;

}

// блокируем срабатывание клавиш, чтобы не сдвигался курсор мыши
void PlotTracker::widgetKeyPressEvent(QKeyEvent *e)
{
    const int key = e->key();

    if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down) {

    }
    else QwtPlotPicker::widgetKeyPressEvent(e);
}

QString smartDouble(double v)
{
    double v1=qAbs(v);
    if (v1>=0.1 && v1 <= 10000) return QString::number(v,'f',2);
    if (v1>=0.01 && v1 <= 0.1) return QString::number(v,'f',3);
    if (v1>=0.001 && v1 <= 0.01) return QString::number(v,'f',4);
    if (v1>=0.0001 && v1 <= 0.001) return QString::number(v,'f',5);

    return QString::number(v,'g');
}

QwtText PlotTracker::trackerTextF(const QPointF &pos) const
{
    QColor bg(Qt::white);
    bg.setAlpha(200);

    QwtText text;

    if (plot->spectrogram) {
        bool success = false;
        double y = plot->curves.first()->channel->data()->YforXandZ(pos.x(), pos.y(), success);
        if (success)
            text = QwtText(smartDouble(pos.x())+", "+smartDouble(pos.y()) + ", "+smartDouble(y));
        else
            text = QwtText(smartDouble(pos.x())+", "+smartDouble(pos.y()));
    }
    else
        text = QwtText(smartDouble(pos.x())+", "+smartDouble(pos.y()));
    text.setBackgroundBrush(QBrush(bg));
    return text;
}

void PlotTracker::maybeHover(const QPointF &pos)
{
    bool found = false;
    const QwtPlotItemList& itmList = plot->itemList(QwtPlotItem::Rtti_PlotMarker);
    for (QwtPlotItemIterator it = itmList.constBegin(); it != itmList.constEnd(); ++it) {
        if (TrackingCursor *c = dynamic_cast<TrackingCursor *>(*it )) {
            if (!c->isVisible()) {
                continue;
            }
            int newX = (int)(plot->transform(QwtAxis::xBottom, c->xValue()));
            int posX = (int)(plot->transform(QwtAxis::xBottom, pos.x()));
            if (qAbs(newX-posX)<=4) {
                cursor = plot->canvas()->cursor();
                plot->canvas()->setCursor(QCursor(Qt::SizeHorCursor));
                found = true;
                break;
            }
        }
    }
    if (!found) plot->canvas()->setCursor(QCursor(Qt::CrossCursor));
}
