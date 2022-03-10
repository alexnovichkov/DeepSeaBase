#include "trackingpanel.h"

#include <QtWidgets>
#include <app.h>

#include <qwt_text.h>
#include <qwt_plot_zoneitem.h>
#include "plot/plot.h"
#include "fileformats/dfdfiledescriptor.h"
#include "plot/curve.h"
#include "logging.h"
#include "plot/trackingcursor.h"
#include "plot/plotmodel.h"
#include "plot/clearablespinbox.h"

QString roundedBy(double value)
{
    QString s = QString::number(value, 'f', 1);

    if (s == "0.0" || s == "-0.0") return QString::number(value, 'g', 1);

    return s;
}

//возвращает ближайшее точное значение для заданного приближенного value
int stepsToClosest(double xBegin, double step, double value)
{
    if (qFuzzyIsNull(step)) return 0;

    int n = round((value - xBegin)/step);
    return n;
}

int stepsToClosest(Channel *c, double val)
{
    if (!c) return 0;

    if (c->data()->xValuesFormat() == DataHolder::XValuesUniform)
        return stepsToClosest(c->data()->xMin(), c->data()->xStep(), val);

    //необходимо скопировать значения, чтобы алгоритм std::min_element не падал
    auto xValues = c->data()->xValues();
    return closest(xValues.cbegin(), xValues.cend(), val) - xValues.cbegin();
}

TrackingPanel::TrackingPanel(Plot *parent) : QWidget(parent), plot(parent)
{DD;
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Курсор");

    for (int i=0; i<2; ++i)
        cursors.append(new TrackingCursor(QColor(40,40,150), Cursor::Style::Cross, nullptr));
    for (int i=2; i<4; ++i)
        cursors.append(new TrackingCursor(QColor(150,40,40), Cursor::Style::Cross, nullptr));

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
        connect(c, SIGNAL(stateChanged(int)), SLOT(updateState()));
        cursorBoxes << c;
    }

    for (int i=0; i<4; ++i) {
        auto *c = new ClearableSpinBox(this);
        c->moveTo({0.0, 0.0});
        connect(c, &ClearableSpinBox::valueChanged, [=](QPointF val){
            updateTrackingCursor(val, i);
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
        for (TrackingCursor *c: qAsConst(cursors)) {
            c->showYValues = state==Qt::Checked;
            c->updateLabel();
        }
    });
    yValuesCheckBox->setChecked(App->getSetting("cursorShowYValues", false).toBool());

    harmonics = new QCheckBox("Включить показ гармоник", this);
    connect(harmonics, &QCheckBox::stateChanged, [=](/*int state*/){
        updateState();
    });

    QGridLayout *lay = new QGridLayout;

    lay->addWidget(new QLabel("Курсоры", this), 0, 0, 1, 4);

    lay->addWidget(cursorBoxes.at(0), 1,0);
    lay->addWidget(new QLabel("X1", this), 1, 1);
    lay->addWidget(spins[0], 1,2);
    lay->addWidget(new QLabel("ЛКМ", this), 1, 3);

    lay->addWidget(cursorBoxes[1], 2,0);
    lay->addWidget(new QLabel("X2", this), 2, 1);
    lay->addWidget(spins[1], 2,2);
    lay->addWidget(new QLabel("Ctrl+ЛКМ", this), 2,3);

    lay->addWidget(new QLabel("Режекция", this), 3, 0, 1, 4);


    lay->addWidget(cursorBoxes[2], 4,0);
    lay->addWidget(new QLabel("X3", this), 4, 1);
    lay->addWidget(spins[2], 4,2);

    lay->addWidget(cursorBoxes[3], 5,0);
    lay->addWidget(new QLabel("X4", this), 5, 1);
    lay->addWidget(spins[3], 5,2);

    lay->addWidget(yValuesCheckBox, 7,0,1,4);
    lay->addWidget(harmonics, 8,0,1,4);

    lay->addWidget(tree, 0,4,9,2);

    lay->addWidget(button, 9,4,2,1);
    lay->addWidget(box, 9,5);
    lay->addWidget(box1, 10,5);

//    lay->setColumnStretch(1,1);
    lay->setColumnStretch(4,1);

    setLayout(lay);
    resize(680,250);
}

TrackingPanel::~TrackingPanel()
{DD;
    App->setSetting("cursorShowYValues", yValuesCheckBox->checkState()==Qt::Checked);
    for (auto *cursor: qAsConst(cursors)) {
        cursor->detach();
        delete cursor;
    }
    cursors.clear();

    for (auto *d: qAsConst(_harmonics)) {
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
    // здесь value - произвольное число, соответствующее какому-то положению на осях X и Y
    double xVal = value.x(); //координата по оси X
    double yVal = value.y(); //координата по оси Y или Z, если это сонограмма
    const bool sonogram = plot->type()==Plot::PlotType::Spectrogram;

    if (!qIsNaN(xVal)) {
        auto list = plot->model()->plottedChannels();
        //ищем минимальный шаг по оси X
        Channel *first = nullptr;
        for (auto c: list) if (c->data()->xValuesFormat()==DataHolder::XValuesUniform) {
            first = c;
            break;
        }
        if (first) {
            for (auto c: list) {
                if (c->data()->xStep() < first->data()->xStep() && c->data()->xValuesFormat()==DataHolder::XValuesUniform)
                    first = c;
            }
            xVal = closest(first, xVal);
        }
    }
    if (!qIsNaN(yVal) && sonogram) {
        auto list = plot->model()->plottedChannels();
        //ищем минимальный шаг по оси Y/Z
        Channel *first = nullptr;
        for (auto c: list) if (c->data()->zValuesFormat()==DataHolder::XValuesUniform) {
            first = c;
            break;
        }
        if (first) {
            for (auto c: list) {
                if (c->data()->zStep() < first->data()->zStep() && c->data()->zValuesFormat()==DataHolder::XValuesUniform)
                    first = c;
            }
            yVal = closest(first, yVal, false);
        }
    }

    spins[index]->moveTo({xVal, yVal});
}

void TrackingPanel::setStep(double step)
{DD;
    mStep = step;
    for (int i=0; i<4; ++i) spins[i]->setStep(mStep);
}

void TrackingPanel::switchVisibility()
{DD;
    if (isVisible()) {
        setVisible(false);
        for (auto cursor: qAsConst(cursors)) cursor->detach();
        for (auto h: qAsConst(_harmonics)) h->detach();
        cursorSpan1->detach();
        cursorSpan2->detach();
    }
    else {
        setVisible(true);
        updateState();
    }
}

void TrackingPanel::updateTrackingCursor(QPointF val, int index)
{DD;
    if (!isVisible()) return;

    //проверяем, не нужно ли обновить значение по оси Y
    Curve *c = plot->model()->firstOf([](Curve *curve){return curve->selected();});
    if (c) {
        bool ok;
        double y = c->channel->data()->YforXandZ(val.x(), 0.0, ok);
        if (ok) val.setY(y);
    }

    cursors[index]->moveTo(val);
    for (int i=0; i<cursors.size(); ++i)
        cursors[i]->setSelected(i == index);

    for (int i=0; i<_harmonics.size(); ++i)
        _harmonics[i]->setValue(val.x()*(i+2), 0.0);

    updateState();
}

void TrackingPanel::update()
{DD;
    auto curves = plot->model()->curves();
    if (plot->model()->curves().count()==0) return;
    //ищем минимальный шаг по оси X
    auto first = plot->model()->firstOf([](Curve *c){return c->channel->data()->xValuesFormat()==DataHolder::XValuesUniform;});
    if (!first) first = curves.first();

    auto data = first->channel->data();
    //ищем минимальный шаг по оси X
    for (auto c: curves) {
        if (c->selected()) {
            //используем шаг по X для выбранного графика, иначе
            data = c->channel->data();
            break;
        }
        if (c->channel->data()->xStep() < data->xStep() &&
            c->channel->data()->xValuesFormat()==DataHolder::XValuesUniform)
            data = c->channel->data();
    }
    setStep(data->xStep());

    //ищем максимальный диапазон
    double xmin = data->xMin();
    double xmax = data->xMax();
    for (auto c: curves) {
        if (c->channel->data()->xMin() < xmin)
            xmin = c->channel->data()->xMin();
        if (c->channel->data()->xMax() > xmax)
            xmax = c->channel->data()->xMax();
    }

    //устанавливаем параметры
    for (auto spin: qAsConst(spins)) {
        if (data->xValuesFormat()==DataHolder::XValuesNonUniform)
            spin->setXValues(data->xValues());
        spin->setXRange(xmin,xmax);
    }

    //дополнительно для спектрограмм
    if (plot->type()==Plot::PlotType::Spectrogram) {
        data = curves.first()->channel->data();
        for (auto spin: qAsConst(spins)) {
            if (data->zValuesFormat()==DataHolder::XValuesNonUniform)
                spin->setYValues(data->zValues());
            spin->setYStep(data->zStep());
            spin->setYRange(data->zMin(),data->zMax());
        }
    }
}

void TrackingPanel::updateState()
{DD;
    for (int i=0; i<4; ++i) {
        if (cursorBoxes[i]->checkState()==Qt::Checked && isVisible()) {
            cursors[i]->attach(plot);
        }
        else {
            cursors[i]->detach();
        }
    }
    for (auto h: qAsConst(_harmonics)) {
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

    for (Curve *c: plot->model()->curves()) {
        QVector<int> steps(4);

        steps[minBorder] = stepsToClosest(c->channel, leftBorder);
        steps[maxBorder] = stepsToClosest(c->channel, rightBorder);
        steps[minExclude] = stepsToClosest(c->channel, leftExclude);
        steps[maxExclude] = stepsToClosest(c->channel, rightExclude);

        double cumul = 0.0;
        double energy = 0.0;
        double reject = 0.0;


        if (computeEnergy) {
            QVector<double> values = c->channel->data()->linears(0);
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
        for (int i=0; i<4; ++i) {
            QPair<double, double> p = {c->channel->data()->xValue(steps[i]), c->channel->data()->yValue(steps[i])};
            values << p;
        }

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
    for (auto c: qAsConst(cursors))
        c->setSelected(c == cursor);
}

void TrackingPanel::moveCursor(Enums::Direction direction)
{DD;
    for (int i=0; i<cursors.size(); ++i) {
        if (cursors[i]->selected()) {
            spins[i]->moveOneStep(direction);
        }
    }
}

// установка первых двух курсоров
//вызывается: 1. щелчком мыши по канве графика - сигнал PlotZoom->updateTrackingCursor
//            2. щелчком мыши по шкале Х - сигнал _picker,SIGNAL(axisClicked(QPointF,bool)
void TrackingPanel::setValue(QPointF value, bool second)
{DD;
    changeSelectedCursor(cursors[second?1:0]);
    setXY(value, second?1:0);
}

//установка любого курсора передвижением мышью
//вызывается: PlotPicker->cursorMovedTo
void TrackingPanel::setValue(QPointF value)
{DD;
    for (int i=0; i<cursors.size(); ++i) {
        if (cursors[i]->selected()) {
            setXY(value, i);
        }
    }
}

void TrackingPanel::closeEvent(QCloseEvent *event)
{DD;
    emit closeRequested();
    QWidget::closeEvent(event);
}

void TrackingPanel::hideEvent(QHideEvent *event)
{DD;
    for (auto *c: qAsConst(cursors))
        c->detach();
    for (auto *d: qAsConst(_harmonics))
        d->detach();

    cursorSpan1->detach();
    cursorSpan2->detach();
    QWidget::hideEvent(event);
}

ZoneSpan::ZoneSpan(const QColor &color)
{DD;
    setOrientation(Qt::Vertical);
    setBrush(color);
}

