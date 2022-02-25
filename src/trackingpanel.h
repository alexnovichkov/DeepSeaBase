#ifndef TRACKINGPANEL_H
#define TRACKINGPANEL_H


#include <QtWidgets>
#include <qwt_plot_zoneitem.h>
#include <qwt_plot_marker.h>
#include "enums.h"

class QTreeWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class Plot;
class TrackingCursor;
class ClearableSpinBox;

class ZoneSpan : public QwtPlotZoneItem
{
public:
    explicit ZoneSpan(const QColor &color);
};



class TrackingPanel: public QWidget
{
    Q_OBJECT
public:
    struct TrackInfo {
        QString name;
        QColor color;
        QList<QPair<double, double>> values;
        double skz;
        double energy;
        double reject;
    };

    explicit TrackingPanel(Plot *parent=0);
    ~TrackingPanel();
    void updateState(const QList<TrackInfo> &curves);


    void setStep(double step);
    void switchVisibility();
public slots:
    // рассчитывает точное значение Х и Y и меняет показания на счетчиках
    void setValue(QPointF value, bool second);
    void setValue(QPointF value);
    void update();
    void updateState();
    void changeSelectedCursor(TrackingCursor *cursor);
    void moveCursor(Enums::Direction direction);
signals:
    void switchHarmonics(bool on);
    void closeRequested();
private:
    void setXY(QPointF value, int index);
    void updateTrackingCursor(QPointF val, int index);


    QTreeWidget *tree;
    QPushButton *button;
    QCheckBox *box;
    QCheckBox *box1;
    QCheckBox *harmonics;
    QCheckBox *yValuesCheckBox;

    double mStep;

    QList<TrackingCursor *> cursors;
    QList<ClearableSpinBox *> spins;
    QList<QCheckBox *> cursorBoxes;

    QList<QwtPlotMarker *> _harmonics;

    ZoneSpan *cursorSpan1;
    ZoneSpan *cursorSpan2;
    Plot *plot;

    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
};

#endif // TRACKINGPANEL_H
