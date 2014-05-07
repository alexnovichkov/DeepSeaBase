#ifndef GRAPHPROPERTIESDIALOG_H
#define GRAPHPROPERTIESDIALOG_H

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

class GraphPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GraphPropertiesDialog(Curve *curve, Plot *parent = 0);
    
signals:
    void curveChanged(Curve *curve);
public slots:
private:
    Curve *curve;
    Plot *plot;
    QLineEdit *titleEdit;
    QSpinBox *widthSpinBox;
    QComboBox *styleComboBox;

    QPen oldPen;
    QString oldTitle;

    // QDialog interface
public slots:
    virtual void reject();
};

#endif // GRAPHPROPERTIESDIALOG_H
