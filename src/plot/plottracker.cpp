#include "plottracker.h"

#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include "logging.h"
#include <QKeyEvent>

PlotTracker::PlotTracker(QWidget *canvas) :
    QwtPlotPicker(canvas)
{DD;
    setStateMachine(new QwtPickerTrackerMachine);
    setTrackerMode(QwtPicker::AlwaysOn);
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
    text.sprintf( "%.2f, %.2f", pos.x(), pos.y());

    return QwtText( text );
}
