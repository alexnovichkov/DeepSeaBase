#include "axisboundsdialog.h"

#include <QtWidgets>
#include <qwt_plot.h>

AxisBoundsDialog::AxisBoundsDialog(double leftBorder, double rightBorder, int axis, QWidget *parent) : QDialog(parent),
    _leftBorder(leftBorder), _rightBorder(rightBorder), _axis(axis), _autoscale(false)
{
    setWindowTitle("Установка шкалы");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QLineEdit *leftEdit = new QLineEdit(QString::number(_leftBorder), this);
    connect(leftEdit, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        double val = text.toDouble(&ok);
        if (ok) _leftBorder = val;
    });

    QLineEdit *rightEdit = new QLineEdit(QString::number(_rightBorder), this);
    connect(rightEdit, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        double val = text.toDouble(&ok);
        if (ok) _rightBorder = val;
    });

    QString text = "Подогнать масштаб по вертикальным осям";
    if (axis == QwtPlot::yLeft || axis == QwtPlot::yRight)
        text = "Подогнать масштаб по горизонтальной оси";
    QCheckBox *scaleAxis = new QCheckBox(text, this);
    scaleAxis->setChecked(false);
    connect(scaleAxis, QCheckBox::stateChanged, [=](int state){
        _autoscale = state==Qt::Checked;
    });

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("s1", this), 0,0);
    l->addWidget(leftEdit, 0,1);
    l->addWidget(new QLabel("s2", this), 0,2);
    l->addWidget(rightEdit, 0, 3);
    l->addWidget(scaleAxis, 1,0,1,4,Qt::AlignLeft);
    l->addWidget(buttonBox, 2,0,1,4, Qt::AlignRight);
    setLayout(l);
}

double AxisBoundsDialog::leftBorder() const
{
    return _leftBorder;
}

double AxisBoundsDialog::rightBorder() const
{
    return _rightBorder;
}
