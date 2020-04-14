#ifndef PLOTPICKER_H
#define PLOTPICKER_H

#include <qwt_plot_picker.h>

class QKeyEvent;

/**
 * @brief The PlotPicker class
 * Простой класс, отображающий текущие координаты на курсоре
 */
class PlotTracker : public QwtPlotPicker
{
public:
    explicit PlotTracker(QWidget *canvas);
    ~PlotTracker();
protected:
    virtual void widgetKeyPressEvent(QKeyEvent *e);
    virtual QwtText trackerTextF(const QPointF &pos) const;
};

#endif // PLOTPICKER_H
