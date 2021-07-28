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

class ZoneSpan : public QwtPlotZoneItem
{
public:
    explicit ZoneSpan(const QColor &color);
};

class ClearableSpinBox: public QAbstractSpinBox
{
    Q_OBJECT
public:
    explicit ClearableSpinBox(QWidget *parent);
    void moveLeft();
    void moveRight();
    void setStep(double step);
    void setXValues(const QVector<double> &values);
    void moveTo(double xValue);
    void moveTo(const QPair<double, double> &position);
    double getXValue() const {return xVal;}
    double getYValue() const {return yVal;}
    void setYValue(double yValue) {yVal = yValue;}
    double getStep() const {return step;}
    void setPrefix(const QString &prefix);
signals:
    void valueChanged(double value);
private:
    void updateText(double val);
    double yVal = 0.0;
    double xVal = 0.0;
    double step = 0.0;
    int current = 0;
    QVector<double> xValues;
    QString prefix;

    // QAbstractSpinBox interface
public:
    virtual void stepBy(int steps) override;

protected:
    virtual StepEnabled stepEnabled() const override;

    // QWidget interface
public:
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
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
