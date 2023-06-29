#include "axisboundsdialog.h"

#include <QtWidgets>
#include "logging.h"
#include "plot/qcustomplot.h"

AxisBoundsDialog::AxisBoundsDialog(QCPAxis *axis, AxisTickerParameters parameters, QWidget *parent) : QDialog(parent),
    _axis(axis), _parameters(parameters)
{DD;
    setWindowTitle("Установка шкалы");

    _leftBorder = axis->range().lower;
    _rightBorder = axis->range().upper;

    if (_parameters.tickStepAutomatic) _parameters.tickStep = axis->ticker()->tickStep();
    if (_parameters.subTickStepAutomatic) _parameters.subTickStep = axis->ticker()->subTickStep();

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

//    QString text = "Подогнать масштаб по вертикальным осям";
//    if (axisType == Enums::AxisType::atLeft || axisType == Enums::AxisType::atRight)
//        text = "Подогнать масштаб по горизонтальной оси";
//    QCheckBox *scaleAxis = new QCheckBox(text, this);
//    scaleAxis->setChecked(false);
//    connect(scaleAxis, &QCheckBox::stateChanged, [&](int state){
//        _autoscale = state==Qt::Checked;
//    });

    QGroupBox *group = new QGroupBox("Деления", this);
    QLineEdit *tick = new QLineEdit(QString::number(_parameters.tickStep), this);
    tick->setEnabled(!_parameters.tickStepAutomatic);
    connect(tick, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        double val = text.toDouble(&ok);
        if (ok) _parameters.tickStep = val;
    });
    QLineEdit *subTick = new QLineEdit(QString::number(_parameters.subTickStep), this);
    subTick->setEnabled(!_parameters.subTickStepAutomatic);
    connect(subTick, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        double val = text.toDouble(&ok);
        if (ok) _parameters.subTickStep = val;
    });
    QCheckBox *tickAutomatic = new QCheckBox("Автоматически", this);
    tickAutomatic->setChecked(_parameters.tickStepAutomatic);
    connect(tickAutomatic, &QCheckBox::stateChanged, [=](int state){
        _parameters.tickStepAutomatic = state == Qt::Checked;
        tick->setEnabled(state != Qt::Checked);
    });
    QCheckBox *subTickAutomatic = new QCheckBox("Автоматически", this);
    subTickAutomatic->setChecked(_parameters.subTickStepAutomatic);
    connect(subTickAutomatic, &QCheckBox::stateChanged, [=](int state){
        _parameters.subTickStepAutomatic = state == Qt::Checked;
        subTick->setEnabled(state != Qt::Checked);
    });
    QGridLayout *ll = new QGridLayout;
    ll->addWidget(new QLabel("Основные", this), 0,0);
    ll->addWidget(tick, 0, 1);
    ll->addWidget(tickAutomatic, 0, 2);
    ll->addWidget(new QLabel("Промежуточные", this), 1, 0);
    ll->addWidget(subTick, 1, 1);
    ll->addWidget(subTickAutomatic, 1, 2);
    group->setLayout(ll);

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Границы от", this), 0,0);
    l->addWidget(leftEdit, 0,1);
    l->addWidget(new QLabel("до", this), 0,2);
    l->addWidget(rightEdit, 0, 3);
    l->addWidget(group, 1, 0, 1, 4);
//    l->addWidget(scaleAxis, 2,0,1,4,Qt::AlignLeft);
    l->addWidget(buttonBox, 2,0,1,4, Qt::AlignRight);
    setLayout(l);
}

AxisTickerParameters AxisBoundsDialog::parameters() const
{
    return _parameters;
}
