#ifndef PLOTPICKER_H
#define PLOTPICKER_H

#include <qwt_plot_picker.h>

class QKeyEvent;
class Plot;

/**
 * @brief The PlotPicker class
 * Простой класс, отображающий текущие координаты на курсоре
 */
class PlotTracker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit PlotTracker(Plot *plot);
    ~PlotTracker();
protected:
    virtual void widgetKeyPressEvent(QKeyEvent *e);
    virtual QwtText trackerTextF(const QPointF &pos) const;
private slots:
    void maybeHover(const QPointF &pos);
private:
    Plot *plot;
};

#endif // PLOTPICKER_H
