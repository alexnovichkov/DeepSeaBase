#include "trackingpanel.h"

#include <QtWidgets>
#include <mainwindow.h>

#include <qwt_text.h>
#include <qwt_plot_zoneitem.h>
#include "plot/plot.h"
#include "fileformats/dfdfiledescriptor.h"
#include "plot/curve.h"
#include "logging.h"
#include "plot/trackingcursor.h"

QString roundedBy(double value)
{
    QString s = QString::number(value, 'f', 1);

    if (s == "0.0" || s == "-0.0") return QString::number(value, 'g', 1);

    return s;
}

template<typename InputIterator, typename ValueType>
InputIterator closest(InputIterator first, InputIterator last, ValueType value)
{
    return std::min_element(first, last, [&](ValueType x, ValueType y)
    {
        return std::abs(x - value) < std::abs(y - value);
    });
}

TrackingPanel::TrackingPanel(Plot *parent) : QWidget(parent), plot(parent)
{DD;
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Курсор");

    for (int i=0; i<2; ++i)
        cursors.append(new TrackingCursor(QColor(40,40,150), TrackingCursor::Cross));
    for (int i=2; i<4; ++i)
        cursors.append(new TrackingCursor(QColor(150,40,40), TrackingCursor::Cross));

    for (int i=0; i<10; ++i) {
        QwtPlotMarker *d = new QwtPlotMarker();
        d->setLineStyle( QwtPlotMarker::VLine );
        d->setLinePen( Qt::black, 0, Qt::DashDotLine );
        _harmonics.append(d);
    }

    cursorSpan1 = new ZoneSpan(QColor(64,131,182,50));
    cursorSpan2 = new ZoneSpan(QColor(182,131,64,50));

    mStep = 0.0;

    tree = new QTreeWidget(this);
    tree->setColumnCount(9);
    tree->setAlternatingRowColors(true);
    tree->setHeaderLabels(QStringList()<<""<<"Название"<<"X1"<<"Y1"<<"X2"<<"Y2"<<"Энергия в полосе"<<"Режекция"<<"СКЗ");
    tree->setRootIsDecorated(false);
    tree->setColumnWidth(0,50);
    tree->setColumnWidth(1,150);

    for (int i=0; i<4; ++i) {
        QCheckBox *c = new QCheckBox(this);
        c->setProperty("trackingCursor", i+1);
        connect(c, SIGNAL(stateChanged(int)), SLOT(update()));
        cursorBoxes << c;
    }

    for (int i=0; i<4; ++i) {
        auto *c = new ClearableSpinBox(this);
        c->setMinimum(0.0);
        c->setMaximum(60000.0);
        c->setValue(0.0);
        c->setDecimals(5);
        c->setPrefix(QString("X%1  ").arg(i+1));
//        c->lineEdit()->setClearButtonEnabled(true);
        connect(c, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [=](double val){
            DD;
            updateTrackingCursor({val, c->yVal}, i);
            cursorBoxes[i]->setChecked(true);
        });
        spins << c;
    }


    button = new QPushButton("Копировать", this);
    connect(button,&QPushButton::clicked,[=](){
        QStringList list;
        QStringList values;
        if (box->isChecked()) values << "Название";
        if (box1->isChecked()) values << "X1";
        values << "Y1";
        if (box1->isChecked()) values << "X2";
        values << "Y2"<<"Энергия в полосе"<<"Режекция"<<"СКЗ";
        list << values.join('\t');

        for(int i=0; i<tree->topLevelItemCount(); ++i) {
            values.clear();
            for (int j=1; j<tree->columnCount(); ++j) {
                values.append(tree->topLevelItem(i)->text(j));
            }
            for (int j=1; j<values.size(); ++j) {
                values[j].replace(".",",");
            }
            if (!box1->isChecked()) {
                values.removeAt(3);
                values.removeAt(1);
            }
            if (!box->isChecked()) {
                values.removeFirst();
            }
            list << values.join('\t');

        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(list.join("\n"));
    });

    box = new QCheckBox("Включая названия каналов", this);

    box1 = new QCheckBox("Включая значения по оси x", this);

    yValuesCheckBox = new QCheckBox("Показывать уровни дискрет", this);
    connect(yValuesCheckBox, &QCheckBox::stateChanged, [=](int state){
        for (TrackingCursor *c: cursors) {
            c->showYValues = state==Qt::Checked;
            c->updateLabel();
        }
    });
    yValuesCheckBox->setChecked(MainWindow::getSetting("cursorShowYValues", false).toBool());

    harmonics = new QCheckBox("Включить показ гармоник", this);
    connect(harmonics, &QCheckBox::stateChanged, [=](/*int state*/){
        update();
    });

    QGridLayout *lay = new QGridLayout;

    lay->addWidget(new QLabel("Курсоры", this), 0, 0, 1, 3);

    lay->addWidget(cursorBoxes[0], 1,0);
    lay->addWidget(spins[0], 1,1);
    lay->addWidget(new QLabel("ЛКМ", this), 1, 2);

    lay->addWidget(cursorBoxes[1], 2,0);
    lay->addWidget(spins[1], 2,1);
    lay->addWidget(new QLabel("Ctrl+ЛКМ", this), 2,2);

    lay->addWidget(new QLabel("Режекция", this), 3, 0, 1, 3);


    lay->addWidget(cursorBoxes[2], 4,0);
    lay->addWidget(spins[2], 4,1);

    lay->addWidget(cursorBoxes[3], 5,0);
    lay->addWidget(spins[3], 5,1);

    lay->addWidget(yValuesCheckBox, 7,0,1,3);
    lay->addWidget(harmonics, 8,0,1,3);

    lay->addWidget(tree, 0,3,9,2);

    lay->addWidget(button, 9,3,2,1);
    lay->addWidget(box, 9,4);
    lay->addWidget(box1, 10,4);

//    lay->setColumnStretch(1,1);
    lay->setColumnStretch(3,1);

    setLayout(lay);
    resize(680,250);
}

TrackingPanel::~TrackingPanel()
{DD;
    MainWindow::setSetting("cursorShowYValues", yValuesCheckBox->checkState()==Qt::Checked);
    for (auto *cursor: cursors) {
        cursor->detach();
        delete cursor;
    }
    cursors.clear();

    for (auto *d: _harmonics) {
        d->detach();
        delete d;
    }
    _harmonics.clear();

    cursorSpan1->detach();
    cursorSpan2->detach();
    delete cursorSpan1;
    delete cursorSpan2;
}

void TrackingPanel::updateState(const QList<TrackingPanel::TrackInfo> &curves)
{DD;
    for(int i=0; i<curves.size(); ++i) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if (!item) {
            item = new QTreeWidgetItem();
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
            tree->addTopLevelItem(item);
        }
        item->setBackground(0,curves[i].color);
        item->setText(1,curves[i].name);

        item->setText(4, QString::number(curves[i].values.at(1).first));
        item->setText(5, roundedBy(curves[i].values.at(1).second));

        item->setText(2, roundedBy(curves[i].values.at(0).first));
        item->setText(3, roundedBy(curves[i].values.at(0).second));

        item->setText(6, roundedBy(curves[i].energy));
        item->setText(7, roundedBy(curves[i].reject));
        item->setText(8, roundedBy(curves[i].skz));
    }
    for (int i=tree->topLevelItemCount()-1; i>=curves.size(); --i)
        delete tree->takeTopLevelItem(i);
    for (int i=0; i<tree->columnCount(); ++i)
        if (i!=1) tree->resizeColumnToContents(i);
}

// пересчитываем значение xVal и обновляем показания счетчиков
void TrackingPanel::setXY(QPointF value, int index)
{DD;
    // здесь value - произвольное число, соответствующее какому-то положению на оси X
    double xVal = value.x();
    double yVal = value.y();
    if (plot->hasCurves()) {
        //ищем минимальный шаг по оси X
        double xstep = (*std::min_element(plot->curves.constBegin(), plot->curves.constEnd(),
                                  [](Curve *c1, Curve *c2){
            return c1->channel->data()->xStep() <= c2->channel->data()->xStep();
        }))->channel->data()->xStep();

        if (xstep==0.0) xstep = plot->curves.constFirst()->channel->data()->xStep();

        if (xstep!=0.0) {
            //среди графиков есть график с минимальным ненулевым шагом
            setStep(xstep);

            //2. compute the actual xVal based on the xVal and xstep
            int steps = int(xVal/xstep);
            if (steps <= 0) xVal = 0.0;
            else {
                if (qAbs(xstep*(steps+1)-xVal) < qAbs(xstep*steps-xVal)) steps++;
                xVal = xstep*steps;
            }
        }
        else if (plot->curves.constFirst()->channel->data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
            //xstep == 0 -> третьоктава или еще что-нибудь, проверяем тип файла
            QVector<double> x = plot->curves.constFirst()->channel->data()->xValues();
            if (!x.isEmpty()) {
                auto iter = closest<QVector<double>::const_iterator, double>(x.constBegin(), x.constEnd(), xVal);
                xVal = *iter;
            }
        }
    }
    if (!qFuzzyCompare(spins.at(index)->yVal+1.0, yVal+1.0)) {
        spins[index]->yVal = yVal;
        updateTrackingCursor({xVal, yVal}, index);
        cursorBoxes[index]->setChecked(true);
    }

    spins[index]->yVal = yVal;
    spins[index]->setValue(xVal);
}

void TrackingPanel::setStep(double step)
{DD;
    mStep = step;
    for (int i=0; i<4; ++i) spins[i]->setSingleStep(mStep);
}

void TrackingPanel::switchVisibility()
{DD;
    if (isVisible()) {
        setVisible(false);
        for (auto cursor: cursors) cursor->detach();
        for (auto h: _harmonics) h->detach();
        cursorSpan1->detach();
        cursorSpan2->detach();
    }
    else {
        setVisible(true);
        update();
    }
}

void TrackingPanel::updateTrackingCursor(QPointF val, int index)
{DD;
    if (!isVisible()) return;

    cursors[index]->moveTo(val);
    for (int i=0; i<cursors.size(); ++i)
        cursors[i]->setCurrent(i == index);

    for (int i=0; i<_harmonics.size(); ++i)
        _harmonics[i]->setValue(val.x()*(i+2), 0.0);

    update();
}

void TrackingPanel::update()
{DD;
    for (int i=0; i<4; ++i) {
        if (cursorBoxes[i]->checkState()==Qt::Checked && isVisible()) {
            cursors[i]->attach(plot);
        }
        else {
            cursors[i]->detach();
        }
    }
    for (auto h: _harmonics) {
        if (harmonics->checkState() == Qt::Checked && isVisible()) {
            h->attach(plot);
        }
        else {
            h->detach();
        }
    }

    double leftBorder = cursors[0]->xValue();
    double rightBorder = cursors[1]->xValue();
    double leftExclude = cursors[2]->xValue();
    double rightExclude = cursors[3]->xValue();

    int minBorder = 0;
    int maxBorder = 1;
    int minExclude = 2;
    int maxExclude = 3;

    if (leftBorder  > rightBorder)  {
        std::swap(leftBorder , rightBorder);
        minBorder = 1;
        maxBorder = 0;
    }
    if (leftExclude > rightExclude) {
        std::swap(leftExclude , rightExclude);
        minExclude = 3;
        maxExclude = 2;
    }
    cursorSpan1->setInterval(leftBorder, rightBorder);
    cursorSpan2->setInterval(leftExclude, rightExclude);

    const bool computeEnergy = cursorBoxes[0]->isChecked() && cursorBoxes[1]->isChecked();
    if (computeEnergy && isVisible()) cursorSpan1->attach(plot);
    else cursorSpan1->detach();

    const bool filter = cursorBoxes[2]->isChecked() && cursorBoxes[3]->isChecked();
    if (filter && isVisible()) cursorSpan2->attach(plot);
    else cursorSpan2->detach();

    //get the y values from all curves
    QList<TrackingPanel::TrackInfo> list;
    QVector<QVector<double> > yValues(4);
    QVector<QVector<QColor> > colors(4);

    for (Curve *c: plot->curves) {
        QVector<int> steps(4);

        auto xVals = c->channel->xValues();
        auto iter = closest<QVector<double>::const_iterator, double>(xVals.constBegin(), xVals.constEnd(), leftBorder);
        steps[minBorder] = iter - xVals.constBegin();

        iter = closest<QVector<double>::const_iterator, double>(xVals.constBegin(), xVals.constEnd(), rightBorder);
        steps[maxBorder] = iter - xVals.constBegin();

        iter = closest<QVector<double>::const_iterator, double>(xVals.constBegin(), xVals.constEnd(), leftExclude);
        steps[minExclude] = iter - xVals.constBegin();

        iter = closest<QVector<double>::const_iterator, double>(xVals.constBegin(), xVals.constEnd(), rightExclude);
        steps[maxExclude] = iter - xVals.constBegin();

        for (int i=0; i<4; ++i) {
            if (steps[i] < 0) steps[i] = xVals.size()-1;
        }

        double cumul = 0.0;
        double energy = 0.0;
        double reject = 0.0;


        if (computeEnergy) {
            QVector<double> values = c->channel->data()->linears();
            for (int i=steps[minBorder]; i<=steps[maxBorder]; ++i) {
                double v2 = values[i];
                if (c->channel->data()->yValuesUnits() != DataHolder::YValuesUnits::UnitsQuadratic)
                    v2 *= values[i];
                if (i>=steps[minExclude] && i<=steps[maxExclude] && filter)
                    reject += v2;
                cumul += v2;
                energy += v2;
            }
            if (filter) reject = energy - reject;
            cumul  = DataHolder::toLog(sqrt(cumul)/double(steps[maxBorder]-steps[minBorder]+1),
                                       c->channel->data()->threshold(),
                                       DataHolder::UnitsLinear);
            energy = DataHolder::toLog(energy, c->channel->data()->threshold(), DataHolder::YValuesUnits::UnitsQuadratic);
            reject = DataHolder::toLog(reject, c->channel->data()->threshold(), DataHolder::YValuesUnits::UnitsQuadratic);
            if (!filter) reject = 0.0;
        }

        QList<QPair<double, double>> values;
        for (int i=0; i<4; ++i) values << qMakePair<double, double>(xVals.at(steps[i]), c->channel->data()->yValue(steps[i]));

        TrackingPanel::TrackInfo ti{c->channel->name(), c->pen().color(),
                    values,
                    cumul, energy, reject};
        list << ti;
        for (int i=0; i<4; ++i) {
            yValues[i] << c->channel->data()->yValue(steps[i]);
            colors[i] << c->pen().color();
        }
    }
    for (int i=0; i<4; ++i)
        cursors[i]->setYValues(yValues.at(i), colors.at(i));

    updateState(list);
}

void TrackingPanel::changeSelectedCursor(TrackingCursor *cursor)
{DD;
    for (int i=0; i<cursors.size(); ++i) {
        if (cursors[i] == cursor) {
            cursors[i]->setCurrent(true);
        }
        else {
            cursors[i]->setCurrent(false);
        }
    }
}

void TrackingPanel::moveCursor(Enums::Direction direction)
{DD;
    for (int i=0; i<cursors.size(); ++i) {
        if (cursors[i]->current) {
            switch (direction) {
                case Enums::Right:
                    spins[i]->setValue(spins[i]->value()+mStep);
                    break;
                case Enums::Left:
                    spins[i]->setValue(spins[i]->value()-mStep);
                    break;
                default:
                    break;
            }
        }
    }
}

// установка первых двух курсоров
//вызывается: 1. щелчком мыши по канве графика - сигнал PlotZoom->updateTrackingCursor
//            2. щелчком мыши по шкале Х - сигнал AxisZoom->updateTrackingCursor
void TrackingPanel::setValue(QPointF value, bool second)
{
    changeSelectedCursor(cursors[second?1:0]);
    setXY(value, second?1:0);
}

void TrackingPanel::setXValue(double value, bool second)
{
    changeSelectedCursor(cursors[second?1:0]);
    setXY({value,0}, second?1:0);
}

void TrackingPanel::setYValue(double value, bool second)
{
    changeSelectedCursor(cursors[second?1:0]);
    setXY({0, value}, second?1:0);
}

//установка любого курсора передвижением мышью
//вызывается: PlotPicker->cursorMovedTo
void TrackingPanel::setValue(QPointF value)
{
    for (int i=0; i<cursors.size(); ++i) {
        if (cursors[i]->current) {
            setXY(value, i);
        }
    }
}

void TrackingPanel::closeEvent(QCloseEvent *event)
{
    emit closeRequested();
    QWidget::closeEvent(event);
}

void TrackingPanel::hideEvent(QHideEvent *event)
{
    for (auto *c: cursors)
        c->detach();
    for (auto *d: _harmonics)
        d->detach();

    cursorSpan1->detach();
    cursorSpan2->detach();
    QWidget::hideEvent(event);
}

ZoneSpan::ZoneSpan(const QColor &color)
{
    setOrientation(Qt::Vertical);
    setBrush(color);
}

ClearableSpinBox::ClearableSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{

}
