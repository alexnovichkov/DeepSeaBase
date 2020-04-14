#ifndef PICKER_H
#define PICKER_H

#include <QObject>

#include "plot.h"

class PointLabel;
class PointMarker;
class Curve;

class Picker: public QObject
{
    Q_OBJECT
public:
    enum Mode {
        ModeNone,
        ModeDrag
    };
    Picker(Plot *plot);
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    inline void setMode(Plot::InteractionMode mode) {interactionMode = mode;}
    inline bool isEnabled() const {return enabled;}
    inline void setEnabled(bool enabled) {this->enabled = enabled;}
signals:
    void cursorMovedTo(double newValue);
    void moveCursor(bool moveToRight);
    void cursorSelected(QwtPlotMarker *cursor);
    void setZoomEnabled(bool);
    void xAxisClicked(double xValue, bool secondCursor);
    void selectCursor(int index);
private:
    void procMouseEvent(QEvent *e);
    void procKeyboardEvent(QEvent *e);

    void startPick(QMouseEvent *e);
    void proceedPick(QMouseEvent *e);
    void endPick(QMouseEvent *e);

    QwtPlotMarker *findCursor(const QPoint &pos);
    Curve *findClosestPoint(const QPoint &pos, int &index) const;
    PointLabel *findLabel(const QPoint &pos);

    void highlightPoint(bool enable);

    Plot *plot;
    Plot::InteractionMode interactionMode;
    Mode mode;
    bool enabled;

    int d_selectedPoint;
    Curve *d_selectedCurve;
    PointLabel *d_selectedLabel;
    QwtPlotMarker *d_selectedCursor;

    QPoint d_currentPos;
    PointMarker *marker;

    QPoint pos;
};

#endif // PICKER_H
