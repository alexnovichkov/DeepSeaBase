#include "graphpropertiesdialog.h"

#include <QtWidgets>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include "plot.h"
#include "curve.h"
#include "dfdfiledescriptor.h"

void ClickableLabel::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button()==Qt::LeftButton)
        Q_EMIT clicked();
    QLabel::mouseReleaseEvent(ev);
}

GraphPropertiesDialog::GraphPropertiesDialog(Curve *curve, Plot *parent) :
    QDialog(parent), curve(curve), plot(parent)
{
    setWindowTitle("Настройки кривой");

    oldPen = curve->pen();
    oldTitle = curve->title().text();

    titleEdit = new QLineEdit(oldTitle, this);
    connect(titleEdit, &QLineEdit::textChanged, [=](const QString &newValue) {
        this->curve->setTitle(newValue);
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
    });

    ClickableLabel *colorLabel = new ClickableLabel(this);
    colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorLabel->setPalette(QPalette(oldPen.color()));
    colorLabel->setAutoFillBackground(true);
    connect(colorLabel, &ClickableLabel::clicked,
            [=]() {
                    QPen pen = curve->pen();
                    const QColor color = QColorDialog::getColor(pen.color(), this);

                    if (color.isValid()) {
                        colorLabel->setPalette(QPalette(color));
                        pen.setColor(color);
                        this->curve->setPen(pen);
                        this->curve->dfd->channels[this->curve->channel]->color = color;
                        emit curveChanged(curve);
                    }
                }
    );

    QComboBox *axisComboBox = new QComboBox(this);
    axisComboBox->setEditable(false);
    axisComboBox->addItems(QStringList()<<"Левая"
                           <<"Правая");
    axisComboBox->setCurrentIndex(curve->yAxis());
    connect(axisComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int axis) {
        if (axis == QwtPlot::yLeft && plot->canBePlottedOnLeftAxis(curve->dfd->channels[curve->channel], true)) {
            plot->prepareAxis(axis, curve->dfd->channels[curve->channel]->YName);
            curve->setYAxis(axis);
            plot->moveGraph(curve);
        }
        else if (axis == QwtPlot::yRight && plot->canBePlottedOnRightAxis(curve->dfd->channels[curve->channel], true)) {
            plot->prepareAxis(axis, curve->dfd->channels[curve->channel]->YName);
            curve->setYAxis(axis);
            plot->moveGraph(curve);
        }
        else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");
    });

//    QComboBox *symbolCombo = new QComboBox(this);
//    symbolCombo->setEditable(false);
//    symbolCombo->addItems(QStringList()
//                          <<"Без маркера"
//                          <<"Окружность"
//                          <<"Прямоугольник"
//                          <<"Ромб"
//                          <<"Треугольник"
//                          <<"Треугольник1"
//                          <<"Треугольник2"
//                          <<"Треугольник3"
//                          <<"Треугольник4"
//                          <<"Плюс"
//                          <<"Крест"
//                          <<"Горизонтальная линия"
//                          <<"Вертикальная линия"
//                          <<"Звезда1"
//                          <<"Звезда2"
//                          <<"Шестиугольник");
//    if (curve->symbol()) {
//        if (curve->symbol()->style()>=-1 && curve->symbol()->style()<=15) {
//            symbolCombo->setCurrentIndex(curve->symbol()->style()+1);
//        }
//        else
//            symbolCombo->setCurrentIndex(0);
//    }
//    connect(symbolCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
//            [=](int newValue) {
//        if (newValue == 0) {
//            curve->setSymbol(0);
//        }
//        else {
//            QwtSymbol *s = new QwtSymbol(QwtSymbol::Style(newValue-1));
//            //s->setPen(curve->pen());
//            //s->setBrush(curve->brush());
//            this->curve->setSymbol(s);
//        }
//    });

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

void GraphPropertiesDialog::reject()
{
    curve->setPen(oldPen);
    curve->setTitle(oldTitle);
    QDialog::reject();
}
