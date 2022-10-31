#ifndef CURVEPROPERTIESDIALOG_H
#define CURVEPROPERTIESDIALOG_H

#include <QDialog>

class Curve;
class QLineEdit;
class QSpinBox;
class QComboBox;
class Plot;

#include <QPen>
#include <QBrush>
#include <QLabel>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent=0)
        : QLabel(parent) {}
Q_SIGNALS:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    QPixmap cursor;
};

class CurvePropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CurvePropertiesDialog(Curve *curve, Plot *parent = 0);
    
signals:
    void curveChanged(Curve *curve);
public slots:
private:
    QIcon iconForMarker(int shape) const;
    Curve *curve;
    Plot *plot;
    QLineEdit *titleEdit;
    QLineEdit *legendEdit;
    QSpinBox *widthSpinBox;
    QComboBox *styleComboBox;
    QComboBox *markerComboBox;
    QSpinBox *markerSizeSpinBox;

    QPen oldPen;
    QString oldTitle;
    int oldMarkerShape;
    int oldMarkerSize;

    // QDialog interface
public slots:
    virtual void reject();
};

#endif
