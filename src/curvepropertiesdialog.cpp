#include "curvepropertiesdialog.h"

#include <QtWidgets>

#include "plot/plot.h"
#include "plot/curve.h"
#include "fileformats/dfdfiledescriptor.h"
#include "logging.h"
#include "plot/qcustomplot/qcustomplot.h"

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
    oldMarkerShape = static_cast<int>(curve->markerShape());
    oldMarkerSize = curve->markerSize();

    titleEdit = new QLineEdit(oldTitle, this);
    connect(titleEdit, &QLineEdit::textChanged, [=](const QString &newValue) {
        if (curve->channel->name() != newValue) {
            curve->channel->setName(newValue);
            curve->setTitle(curve->channel->legendName());
    //        curve->legend = curve->channel->legendName();

            curve->channel->descriptor()->setChanged(true);
    //        curve->channel->descriptor()->write();
            emit curveChanged(curve);
            plot->replot();
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
        curve->updateScatter();
        emit curveChanged(curve);
        plot->replot();
    }
    );

    markerSizeSpinBox = new QSpinBox(this);
    markerSizeSpinBox->setRange(0,50);
    markerSizeSpinBox->setValue(oldMarkerSize);

    connect(markerSizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [=](int newValue) {
        curve->setMarkerSize(newValue);
        emit curveChanged(curve);
        plot->replot();
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
        plot->replot();
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
                        plot->replot();
                    }
                }
    );

    markerComboBox = new QComboBox(this);
    markerComboBox->setEditable(false);
    markerComboBox->addItems({"Без маркера",
                              "Точка",
                              "Крест",
                              "Плюс",
                              "Окружность",
                              "Диск",
                              "Квадрат",
                              "Ромб",
                              "Звезда",
                              "Треугольник",
                              "Перевернутый треугольник",
                              "Квадрат с крестом",
                              "Квадрат с плюсом",
                              "Окружность с крестом",
                              "Окружность с плюсом",
                              "Пацифик"});

    for (int i=0; i<16; ++i) {
        markerComboBox->setItemIcon(i, iconForMarker(i));
    }
    markerComboBox->setCurrentIndex(static_cast<int>(curve->markerShape()));
    connect(markerComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int shape) {
        curve->setMarkerShape(static_cast<Curve::MarkerShape>(shape));
        plot->replot();
    });

    QComboBox *axisComboBox = new QComboBox(this);
    axisComboBox->setEditable(false);
    axisComboBox->addItems(QStringList()<<"Левая"
                           <<"Правая");
    axisComboBox->setCurrentIndex(static_cast<int>(curve->yAxis())-1);
    connect(axisComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int axis) {
        plot->moveCurve(curve, Enums::AxisType(axis+1));
        plot->replot();
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
    l->addRow(new QLabel("Маркер", this), markerComboBox);
    l->addRow(new QLabel("Размер маркера", this), markerSizeSpinBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    l->addWidget(buttonBox);
    setLayout(l);
}

QIcon CurvePropertiesDialog::iconForMarker(int shape) const
{
    switch (shape) {
        case 1: return QIcon(":/icons/ssDot.png");
        case 2: return QIcon(":/icons/ssCross.png");
        case 3: return QIcon(":/icons/ssPlus.png");
        case 4: return QIcon(":/icons/ssCircle.png");
        case 5: return QIcon(":/icons/ssDisc.png");
        case 6: return QIcon(":/icons/ssSquare.png");
        case 7: return QIcon(":/icons/ssDiamond.png");
        case 8: return QIcon(":/icons/ssStar.png");
        case 9: return QIcon(":/icons/ssTriangle.png");
        case 10: return QIcon(":/icons/ssTriangleInverted.png");
        case 11: return QIcon(":/icons/ssCrossSquare.png");
        case 12: return QIcon(":/icons/ssPlusSquare.png");
        case 13: return QIcon(":/icons/ssCrossCircle.png");
        case 14: return QIcon(":/icons/ssPlusCircle.png");
        case 15: return QIcon(":/icons/ssPeace.png");
    }

    return QIcon();
}

void CurvePropertiesDialog::reject()
{DDD;
    curve->setPen(oldPen);
    curve->channel->setName(oldTitle);
    curve->setTitle(curve->channel->legendName());
    curve->setMarkerSize(oldMarkerSize);
    curve->setMarkerShape(static_cast<Curve::MarkerShape>(oldMarkerShape));
    plot->replot();
    QDialog::reject();
}
