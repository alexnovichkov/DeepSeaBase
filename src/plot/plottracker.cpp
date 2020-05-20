#include "plottracker.h"

#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include "logging.h"
#include <QKeyEvent>
#include "plot.h"
#include "trackingpanel.h"

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

QwtText PlotTracker::trackerTextF(const QPointF &pos) const
{//DD;
    QString text;
    text.asprintf( "%.2f, %.2f", pos.x(), pos.y());

    return QwtText( text );
}

void PlotTracker::maybeHover(const QPointF &pos)
{
   // qDebug()<<"pos"<<pos;

    bool found = false;
    const QwtPlotItemList& itmList = plot->itemList(QwtPlotItem::Rtti_PlotMarker);
    for (QwtPlotItemIterator it = itmList.constBegin(); it != itmList.constEnd(); ++it) {
        if (TrackingCursor *c = dynamic_cast<TrackingCursor *>(*it )) {
//            qDebug()<<"found tracking cursor";
            if (!c->isVisible()) {
//                qDebug()<<"cursor not visible";
                continue;
            }
//            qDebug()<<"pos"<<pos;
            int newX = (int)(plot->transform(QwtAxis::xBottom, c->xValue()));
            int posX = (int)(plot->transform(QwtAxis::xBottom, pos.x()));
            if (qAbs(newX-posX)<=4) {
//                qDebug()<<"hovering";
                plot->canvas()->setCursor(QCursor(Qt::SizeHorCursor));
                found = true;
                break;
            }
        }
    }
    if (!found) plot->canvas()->unsetCursor();
}
