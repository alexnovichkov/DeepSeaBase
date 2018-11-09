#include "trackingpanel.h"

#include <QtWidgets>
#include <mainwindow.h>

#include <qwt_text.h>
#include <qwt_plot_zoneitem.h>
#include "plot.h"
#include "dfdfiledescriptor.h"
#include "curve.h"
#include "logging.h"

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
        cursors.append(new TrackingCursor(QColor(40,40,150)));
    for (int i=2; i<4; ++i)
        cursors.append(new TrackingCursor(QColor(150,40,40)));

    cursorSpan1 = new ZoneSpan(QColor(0,0,255,100));
    cursorSpan2 = new ZoneSpan(QColor(255,93,93,100));

    mStep = 0.0;

    tree = new QTreeWidget(this);
    tree->setColumnCount(8);
    tree->setAlternatingRowColors(true);
    tree->setHeaderLabels(QStringList()<<""<<"Название"<<"X1"<<"Y1"<<"X2"<<"Y2"<<"СКЗ"<<"Энергия в полосе");
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
        ClearableSpinBox *c = new ClearableSpinBox(this);
        c->setMinimum(0.0);
        c->setMaximum(60000.0);
        c->setValue(0.0);
        c->setDecimals(5);
        c->setPrefix(QString("X%1  ").arg(i+1));
//        c->lineEdit()->setClearButtonEnabled(true);
        connect(c, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [=](double val){
            updateTrackingCursor(val, i);
            cursorBoxes[i]->setChecked(true);
            update();
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
        values << "Y2"<<"СКЗ"<<"Энергия в полосе";
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
        foreach(TrackingCursor *c, cursors) {
            c->showYValues = state==Qt::Checked;
            c->updateLabel();
        }
    });
    yValuesCheckBox->setChecked(MainWindow::getSetting("cursorShowYValues", false).toBool());

    harmonics = new QCheckBox("Включить показ гармоник", this);
    connect(harmonics, &QCheckBox::stateChanged, [=](int state){
        emit switchHarmonics(state==Qt::Checked);
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
    foreach(TrackingCursor *cursor, cursors) {
        cursor->detach();
        delete cursor;
    }
    cursors.clear();

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
        item->setBackgroundColor(0,curves[i].color);
        item->setText(1,curves[i].name);
//        if (index == 1) {
            item->setText(4, QString::number(curves[i].values.at(1).first));
            item->setText(5, QString::number(curves[i].values.at(1).second, 'f', 1));
//        }
//        else if (index == 0) {
            item->setText(2, QString::number(curves[i].values.at(0).first));
            item->setText(3, QString::number(curves[i].values.at(0).second, 'f', 1));
//        }
        item->setText(6, QString::number(curves[i].skz, 'f', 1));
        item->setText(7, QString::number(curves[i].energy, 'f', 1));
    }
    for (int i=tree->topLevelItemCount()-1; i>=curves.size(); --i)
        delete tree->takeTopLevelItem(i);
    for (int i=0; i<tree->columnCount(); ++i)
        if (i!=1) tree->resizeColumnToContents(i);
}

// пересчитываем значение xVal и обновляем показания счетчиков
void TrackingPanel::setX(double xVal, int index)
{DD;
    // здесь xVal - произвольное число, соответствующее какому-то положению на оси X

    if (plot->hasGraphs()) {
        //ищем минимальный шаг по оси X
        double xstep = (*std::min_element(plot->graphs.begin(), plot->graphs.end(),
                                  [](Curve *c1, Curve *c2){
            return c1->channel->xStep() <= c2->channel->xStep();
        }))->channel->xStep();

        if (xstep==0.0) xstep = plot->graphs.first()->channel->xStep();

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
        else if (plot->graphs.first()->channel->data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
            //xstep == 0 -> третьоктава или еще что-нибудь, проверяем тип файла
            QVector<double> x = plot->graphs.first()->channel->data()->xValues();
            if (!x.isEmpty()) {
                auto iter = closest<QVector<double>::iterator, double>(x.begin(), x.end(), xVal);
                xVal = *iter;
            }
        }
    }

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
        for (int i=0; i<4; ++i) cursors[i]->detach();
        cursorSpan1->detach();
        cursorSpan2->detach();
    }
    else {
        setVisible(true);
        update();
    }
}

void TrackingPanel::updateTrackingCursor(double xVal, int index)
{DD;
    if (!isVisible()) return;

//    DebugPrint(xVal);

    cursors[index]->moveTo(xVal);
//    cursors[index]->attach(plot);

//    for (int i=0; i<4; ++i) {
//        if (cursorBoxes[i]->checkState()==Qt::Checked) {
//            cursors[i]->attach(plot);
//        }
//        else {
//            cursors[i]->detach();
//        }
//    }

//    double leftBorder = cursors[0]->xValue();
//    double rightBorder = cursors[1]->xValue();
//    double leftExclude = cursors[2]->xValue();
//    double rightExclude = cursors[3]->xValue();
//    if (leftBorder > rightBorder) std::swap(leftBorder , rightBorder);
//    if (leftExclude > rightExclude) std::swap(leftExclude , rightExclude);
//    cursorSpan1->setInterval(leftBorder, rightBorder);
//    cursorSpan2->setInterval(leftExclude, rightExclude);

//    if (cursorBoxes[0]->isChecked() && cursorBoxes[1]->isChecked()) cursorSpan1->attach(plot);
//    else cursorSpan1->detach();

//    if (cursorBoxes[2]->isChecked() && cursorBoxes[3]->isChecked()) cursorSpan2->attach(plot);
//    else cursorSpan2->detach();

//    bool filter = cursorBoxes[2]->isChecked() && cursorBoxes[3]->isChecked();

//    plot->replot();
//    QRectF rect(leftExclude, 0, rightExclude, 1);

//    //4. get the y values from all graphs
//    QList<TrackingPanel::TrackInfo> list;
//    QVector<double> yValues;
//    foreach(Curve *c, plot->graphs) {
//        int steps, steps1, steps2;

//        auto xVals = c->channel->xValues();
//        auto iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), xVal);
//        steps = iter - xVals.begin();
//        if (steps < 0) steps = xVals.size()-1;

//        iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), leftBorder);
//        steps1 = iter - xVals.begin();
//        if (steps1 < 0) steps1 = xVals.size()-1;

//        iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), rightBorder);
//        steps2 = iter - xVals.begin();
//        if (steps2 < 0) steps2 = xVals.size()-1;

//        if (steps2<steps1) qSwap(steps1, steps2);

//        double cumul = 0.0;

//        for (int i=steps1; i<=steps2; ++i) {
//            if (rect.contains(xVals.at(i), 0) && filter) continue;
//            cumul += pow(10.0, c->channel->yValues().at(i)/5.0);
//        }
//        cumul = 10.0*log10(sqrt(cumul));

//        double energy = 0.0;
//        for (int i=steps1; i<=steps2; ++i) {
//            if (rect.contains(xVals.at(i), 0) && filter) continue;
//            energy += pow(10.0, c->channel->yValues().at(i)/10.0);
//        }
//        energy = 10.0*log10(energy);

//        TrackingPanel::TrackInfo ti{c->channel->name(), c->channel->color(),
//                    xVals.at(steps),
//                    c->channel->yValues().at(steps),
//                    cumul, energy};
//        list << ti;
//        yValues << c->channel->yValues().at(steps);
//    }

//    cursors[index]->setYValues(yValues);

//    updateState(list, index);
    update();
}

void TrackingPanel::update()
{
//    if (!isVisible()) return;

    for (int i=0; i<4; ++i) {
        if (cursorBoxes[i]->checkState()==Qt::Checked && isVisible()) {
            cursors[i]->attach(plot);
        }
        else {
            cursors[i]->detach();
        }
    }

    double leftBorder = cursors[0]->xValue();
    double rightBorder = cursors[1]->xValue();
    double leftExclude = cursors[2]->xValue();
    double rightExclude = cursors[3]->xValue();
    if (leftBorder > rightBorder) std::swap(leftBorder , rightBorder);
    if (leftExclude > rightExclude) std::swap(leftExclude , rightExclude);
    cursorSpan1->setInterval(leftBorder, rightBorder);
    cursorSpan2->setInterval(leftExclude, rightExclude);

    const bool computeEnergy = cursorBoxes[0]->isChecked() && cursorBoxes[1]->isChecked();
    if (computeEnergy && isVisible()) cursorSpan1->attach(plot);
    else cursorSpan1->detach();

    const bool filter = cursorBoxes[2]->isChecked() && cursorBoxes[3]->isChecked();
    if (filter && isVisible()) cursorSpan2->attach(plot);
    else cursorSpan2->detach();


//    plot->replot();
    QRectF rect(leftExclude, 0, rightExclude, 1);

    //4. get the y values from all graphs
    QList<TrackingPanel::TrackInfo> list;
    QVector<QVector<double> > yValues(4);
//    qDebug()<<leftBorder<<rightBorder<<leftExclude<<rightExclude;

    foreach(Curve *c, plot->graphs) {
        QVector<int> steps(4);

        auto xVals = c->channel->xValues();
        auto iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), leftBorder);
        steps[0] = iter - xVals.begin();
        if (steps[0] < 0) steps[0] = xVals.size()-1;

        iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), rightBorder);
        steps[1] = iter - xVals.begin();
        if (steps[1] < 0) steps[1] = xVals.size()-1;

        iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), leftExclude);
        steps[2] = iter - xVals.begin();
        if (steps[2] < 0) steps[2] = xVals.size()-1;

        iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), rightExclude);
        steps[3] = iter - xVals.begin();
        if (steps[3] < 0) steps[3] = xVals.size()-1;

//        qDebug()<<steps;

        double cumul = 0.0;
        double energy = 0.0;
        if (computeEnergy) {
            for (int i=steps[0]; i<=steps[1]; ++i) {
                if (i>=steps[2] && i<=steps[3] && filter) continue;
                cumul += pow(10.0, c->channel->data()->yValue(i)/5.0);
                energy += pow(10.0, c->channel->data()->yValue(i)/10.0);
            }
            cumul  = 10.0*log10(sqrt(cumul));
            energy = 10.0*log10(energy);
        }

        QList<QPair<double, double>> values;
        for (int i=0; i<4; ++i) values << qMakePair<double, double>(xVals.at(steps[i]), c->channel->data()->yValue(steps[i]));

        TrackingPanel::TrackInfo ti{c->channel->name(), c->pen().color(),
                    values,
                    cumul, energy};
        list << ti;
        for (int i=0; i<4; ++i)
            yValues[i] << c->channel->data()->yValue(steps[i]);
    }
    for (int i=0; i<4; ++i)
        cursors[i]->setYValues(yValues.at(i));

    updateState(list);
}

//done
void TrackingPanel::setXValue(QwtPlotMarker *cursor, double value)
{DD;
    if (!cursor) return;

    int index = cursors.indexOf(dynamic_cast<TrackingCursor*>(cursor));
    if (index < 0 || index > 3) return;

    setX(value, index);
}

void TrackingPanel::setXValue(double value, bool second)
{DD;
    if (second) setX(value, 1);
    else setX(value, 0);
}

TrackingCursor::TrackingCursor(const QColor &col)
{
    setLineStyle( QwtPlotMarker::VLine );
    setLinePen( col, 1, /*Qt::DashDotLine*/Qt::SolidLine );
    setLabelAlignment(Qt::AlignBottom | Qt::AlignRight);
    showYValues = MainWindow::getSetting("cursorShowYValues", false).toBool();
}

void TrackingCursor::moveTo(const double xValue)
{DD;
    setValue(xValue, 0.0);
}

void TrackingCursor::setYValues(const QVector<double> &yValues)
{DD;
    this->yValues = yValues;
    updateLabel();
}

void TrackingCursor::updateLabel()
{DD;
    QwtText text;
    text.setBackgroundBrush(Qt::white);
    text.setBorderRadius(1.0);

    //text.setBorderPen(QPen(Qt::black));

    QStringList label;
    if (!yValues.isEmpty() && showYValues) {
        foreach (double v, yValues) {
            label << QString::number(v, 'f', 1);
        }
    }
    label << QString("<b>%1</b>").arg(QString::number(this->xValue(), 'f', 1));
    text.setText(label.join("<br>"),QwtText::RichText);

    setLabel(text);
}


void TrackingPanel::closeEvent(QCloseEvent *event)
{
    emit closeRequested();
    QWidget::closeEvent(event);
}

void TrackingPanel::hideEvent(QHideEvent *event)
{
    foreach(TrackingCursor *c, cursors)
        c->detach();
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
