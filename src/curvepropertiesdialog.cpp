#include "curvepropertiesdialog.h"

#include <QtWidgets>

#include "plot/plot.h"
#include "plot/curve.h"
#include "fileformats/dfdfiledescriptor.h"
#include "logging.h"

void ClickableLabel::mouseReleaseEvent(QMouseEvent *ev)
{DDD;
    if (ev->button()==Qt::LeftButton)
        Q_EMIT clicked();
    QLabel::mouseReleaseEvent(ev);
}

CurvePropertiesDialog::CurvePropertiesDialog(Curve *curve, Plot *parent) :
    QDialog(parent->widget()), curve(curve), plot(parent)
{DDD;
    setWindowTitle("Настройки кривой");

    oldPen = curve->pen();
    oldTitle = curve->channel->name();

    titleEdit = new QLineEdit(oldTitle, this);
    connect(titleEdit, &QLineEdit::textChanged, [=](const QString &newValue) {
        if (curve->channel->name() != newValue) {
            curve->channel->setName(newValue);
            curve->setTitle(curve->channel->legendName());
    //        curve->legend = curve->channel->legendName();

            curve->channel->descriptor()->setChanged(true);
    //        curve->channel->descriptor()->write();
            emit curveChanged(curve);
        }
    }
    );



    widthSpinBox = new QSpinBox(this);
    widthSpinBox->setRange(0,20);
    widthSpinBox->setValue(oldPen.width());
    connect(widthSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [=](int newValue) {
        QPen pen = curve->pen();
        pen.setWidth(newValue);
        this->curve->setPen(pen);
        emit curveChanged(curve);
    }
    );

    styleComboBox = new QComboBox(this);
    styleComboBox->setEditable(false);
    styleComboBox->addItems(QStringList()<<"Без линии"
                            <<"Сплошная"
                            <<"Штриховая"
                            <<"Пунктирная"
                            <<"Штрихпунктирная"
                            <<"Штрих - двойной пунктир");
    styleComboBox->setCurrentIndex(oldPen.style());
    connect(styleComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int newValue) {
        QPen pen = curve->pen();
        pen.setStyle((Qt::PenStyle)newValue);
        this->curve->setPen(pen);
        emit curveChanged(curve);
    });

    ClickableLabel *colorLabel = new ClickableLabel(this);
    colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorLabel->setPalette(QPalette(oldPen.color()));
    colorLabel->setAutoFillBackground(true);
    connect(colorLabel, &ClickableLabel::clicked,
            [=]() {
                    QPen pen = curve->pen();
                    const QColor color = QColorDialog::getColor(pen.color(), this, "", QColorDialog::ShowAlphaChannel);

                    if (color.isValid()) {
                        colorLabel->setPalette(QPalette(color));
                        pen.setColor(color);
                        this->curve->setPen(pen);
                        emit curveChanged(curve);
                    }
                }
    );

    QComboBox *axisComboBox = new QComboBox(this);
    axisComboBox->setEditable(false);
    axisComboBox->addItems(QStringList()<<"Левая"
                           <<"Правая");
    axisComboBox->setCurrentIndex(static_cast<int>(curve->yAxis()));
    connect(axisComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int axis) {
        plot->moveCurve(curve, Enums::AxisType(axis+1));
    });

    QFormLayout *l = new QFormLayout;

    l->addRow(new QLabel("Название", this), titleEdit);
    l->addRow(new QLabel("Цвет", this), colorLabel);
    l->addRow(new QLabel("Толщина", this), widthSpinBox);
    l->addRow(new QLabel("Тип", this), styleComboBox);

    QFrame *line = new QFrame(this);
    line->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    l->addWidget(line);

    l->addRow(new QLabel("Ось Y", this), axisComboBox);
//    l->addRow(new QLabel("Маркер", this), symbolCombo);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    l->addWidget(buttonBox);
    setLayout(l);
}

void CurvePropertiesDialog::reject()
{DDD;
    curve->setPen(oldPen);
    curve->channel->setName(oldTitle);
    curve->setTitle(curve->channel->legendName());
    QDialog::reject();
}
