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
    enum Axis {
        XAxis,
        YAxis
    };
    explicit ClearableSpinBox(QWidget *parent);
    void setAxis(Axis axis);
    void moveOneStep(Enums::Direction direction);
    void setXStep(double step);
    void setYStep(double step);
    void setStep(double step);
    void setXValues(const QVector<double> &values);
    void setYValues(const QVector<double> &values);
    void moveTo(const QPointF &position);
    void moveBy(int xSteps, int ySteps);
    double getXValue() const {return xVal;}
    double getYValue() const {return yVal;}
    double getXStep() const {return xStep;}
    double getYStep() const {return yStep;}
    void setPrefix(const QString &prefix);
    void setXRange(double min, double max);
    void setYRange(double min, double max);
signals:
    void valueChanged(QPointF value);
private:
    void updateText(double val);
    double yVal = 0.0;
    double xVal = 0.0;
    double xStep = 0.0;
    double yStep = 0.0;
    int xCurrent = 0;
    int yCurrent = 0;
    QVector<double> xValues;
    QVector<double> yValues;
    QString prefix;
    double xMin = 0.0, xMax = 0.0;
    double yMin = 0.0, yMax = 0.0;
    Axis axis = XAxis;

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
