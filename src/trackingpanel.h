#ifndef TRACKINGPANEL_H
#define TRACKINGPANEL_H

#include <QWidget>
#include <qwt_plot_marker.h>

class QTreeWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class Plot;

#include <QtWidgets>
#include <qwt_plot_zoneitem.h>

class TrackingCursor : public QwtPlotMarker
{
public:
    explicit TrackingCursor(const QColor &col);
    void moveTo(const double xValue);
    void setYValues(const QVector<double> &yValues);
    void setCurrent(bool current);

    void updateLabel();
    bool showYValues;
    QVector<double> yValues;
    bool reject = false;
};

class ZoneSpan : public QwtPlotZoneItem
{
public:
    explicit ZoneSpan(const QColor &color);
};

class ClearableSpinBox: public QDoubleSpinBox
{
public:
    explicit ClearableSpinBox(QWidget *parent);
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
    // рассчитывает точное значение Х и меняет показания на счетчиках
    void setXValue(double value, bool second);
    void setXValue(QwtPlotMarker *cursor, double value);
    void update();
    void updateSelectedCursor(QwtPlotMarker *cursor);
signals:
    void switchHarmonics(bool on);
    void closeRequested();
private:
    void setX(double value, int index);
    void updateTrackingCursor(double xVal, int index);


    QTreeWidget *tree;
    QPushButton *button;
    QCheckBox *box;
    QCheckBox *box1;
    QCheckBox *harmonics;
    QCheckBox *yValuesCheckBox;

    QLabel *x1Label;
    QLabel *x2Label;


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
