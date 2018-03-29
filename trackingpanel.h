#ifndef TRACKINGPANEL_H
#define TRACKINGPANEL_H

#include <QWidget>
#include <qwt_plot_marker.h>

class QTreeWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class QDoubleSpinBox;
class Plot;

class TrackingCursor : public QwtPlotMarker
{
public:
    void moveTo(const double xValue);
    void setYValues(const QVector<double> &yValues);

    void updateLabel();
    bool showYValues;
    QVector<double> yValues;
};

class TrackingPanel:public QWidget
{
    Q_OBJECT
public:
    struct TrackInfo {
        QString name;
        QColor color;
        double xval, yval;
        //double xval2, yval2;
        double skz;
        double energy;
    };

    explicit TrackingPanel(Plot *parent=0);
    ~TrackingPanel();
    void updateState(const QList<TrackInfo> &curves, bool second);
    void setX(double x, bool second);
    void setStep(double step);
    void switchVisibility();
public slots:
    void updateTrackingCursor(double xVal, bool second);
    void updateTrackingCursor(QwtPlotMarker*cursor, double newVal);
signals:
    void switchHarmonics(bool on);
    void closeRequested();
private:
    QTreeWidget *tree;
    QPushButton *button;
    QCheckBox *box;
    QCheckBox *box1;
    QCheckBox *harmonics;
    QCheckBox *yValuesCheckBox;

    QCheckBox *showFirst;
    QCheckBox *showSecond;

    QLabel *x1Label;
    QLabel *x2Label;
    QDoubleSpinBox *x1Spin;
    QDoubleSpinBox *x2Spin;

    double mStep;

    TrackingCursor *_trackingCursor;
    TrackingCursor *_trackingCursor1;
    Plot *plot;

    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event);
    virtual void hideEvent(QHideEvent *event);
};

#endif // TRACKINGPANEL_H
