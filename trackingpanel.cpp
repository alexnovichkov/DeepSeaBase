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

    _trackingCursor = new TrackingCursor();
    _trackingCursor->setLineStyle( QwtPlotMarker::VLine );
    _trackingCursor->setLinePen( Qt::black, 1, Qt::DashDotLine );
    _trackingCursor->setLabelAlignment(Qt::AlignBottom | Qt::AlignRight);
    _trackingCursor->showYValues = MainWindow::getSetting("cursorShowYValues", false).toBool();

    _trackingCursor1 = new TrackingCursor();
    _trackingCursor1->setLineStyle( QwtPlotMarker::VLine );
    _trackingCursor1->setLinePen( Qt::black, 1, Qt::DashDotLine );
    _trackingCursor1->setLabelAlignment(Qt::AlignBottom | Qt::AlignLeft);
    _trackingCursor1->showYValues = MainWindow::getSetting("cursorShowYValues", false).toBool();

    cursorSpan = new QwtPlotZoneItem();
    cursorSpan->setOrientation(Qt::Vertical);


    mStep = 0.0;

    tree = new QTreeWidget(this);
    tree->setColumnCount(8);
    tree->setAlternatingRowColors(true);
    tree->setHeaderLabels(QStringList()<<""<<"Название"<<"X1"<<"Y1"<<"X2"<<"Y2"<<"СКЗ"<<"Энергия в полосе");
    tree->setRootIsDecorated(false);
    tree->setColumnWidth(0,50);
    tree->setColumnWidth(1,150);

    showFirst = new QCheckBox(this);
    showSecond = new QCheckBox(this);
    connect(showFirst, &QCheckBox::stateChanged, [=](int state){
        if (state==Qt::Checked) {
            _trackingCursor->attach(plot);
            if (showSecond->isChecked()) cursorSpan->attach(plot);
        }
        else {
            _trackingCursor->detach();
            cursorSpan->detach();
        }
    });
    connect(showSecond, &QCheckBox::stateChanged, [=](int state){
        if (state==Qt::Checked) {
            _trackingCursor1->attach(plot);
            if (showFirst->isChecked()) cursorSpan->attach(plot);
        }
        else {
            _trackingCursor1->detach();
            cursorSpan->detach();
        }
    });


    x1Spin = new QDoubleSpinBox(this);
    x1Spin->setMinimum(0.0);
    x1Spin->setMaximum(60000.0);
    x1Spin->setValue(0.0);
    x1Spin->setDecimals(5);
    x1Spin->setPrefix("X1  ");
    connect(x1Spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=](double val){
        updateTrackingCursor(val, false);
    });
    //x1Spin->setReadOnly(true);

    x2Spin = new QDoubleSpinBox(this);
    x2Spin->setMinimum(0.0);
    x2Spin->setMaximum(60000.0);
    x2Spin->setValue(0.0);
    x2Spin->setDecimals(5);
    x2Spin->setPrefix("X2  ");
    connect(x2Spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=](double val){
        updateTrackingCursor(val, true);
        showSecond->setChecked(true);
    });
    //x2Spin->setReadOnly(true);

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
        if (_trackingCursor) {
            _trackingCursor->showYValues = state==Qt::Checked;
            _trackingCursor->updateLabel();
        }
        if (_trackingCursor1) {
            _trackingCursor1->showYValues = state==Qt::Checked;
            _trackingCursor1->updateLabel();
        }
    });
    yValuesCheckBox->setChecked(MainWindow::getSetting("cursorShowYValues", false).toBool());

    harmonics = new QCheckBox("Включить показ гармоник", this);
    connect(harmonics, &QCheckBox::stateChanged, [=](int state){
        emit switchHarmonics(state==Qt::Checked);
    });

    QGridLayout *lay = new QGridLayout;
    lay->addWidget(new QLabel("Левая кнопка мыши - первая граница", this), 0, 0, 1, 2);
    lay->addWidget(new QLabel("Ctrl + левая кнопка мыши - вторая граница", this), 0, 2, 1, 2);

    lay->addWidget(showFirst, 1,0,1,1);
    lay->addWidget(x1Spin, 1,1);
    lay->addWidget(showSecond, 1,2);
    lay->addWidget(x2Spin, 1,3);


    lay->addWidget(tree, 2,0,1,4);
    lay->addWidget(harmonics, 3,0,1,2);
    lay->addWidget(yValuesCheckBox, 3,2,1,2);
    lay->addWidget(button, 4,0,2,2);
    lay->addWidget(box, 4,2,1,2);
    lay->addWidget(box1, 5,2,1,2);

    lay->setColumnStretch(1,1);
    lay->setColumnStretch(3,1);

    setLayout(lay);
    resize(550,300);
}

TrackingPanel::~TrackingPanel()
{DD;
    MainWindow::setSetting("cursorShowYValues", yValuesCheckBox->checkState()==Qt::Checked);
    _trackingCursor->detach();
    _trackingCursor1->detach();
    delete _trackingCursor;
    delete _trackingCursor1;
    cursorSpan->detach();
    delete cursorSpan;
}

void TrackingPanel::updateState(const QList<TrackingPanel::TrackInfo> &curves, bool second)
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
        if (second) {
            item->setText(4, QString::number(curves[i].xval));
            item->setText(5, QString::number(curves[i].yval, 'f', 1));
        }
        else {
            item->setText(2, QString::number(curves[i].xval));
            item->setText(3, QString::number(curves[i].yval, 'f', 1));
        }
        item->setText(6, QString::number(curves[i].skz, 'f', 1));
        item->setText(7, QString::number(curves[i].energy, 'f', 1));
    }
    for (int i=tree->topLevelItemCount()-1; i>=curves.size(); --i)
        delete tree->takeTopLevelItem(i);
    for (int i=0; i<tree->columnCount(); ++i)
        if (i!=1) tree->resizeColumnToContents(i);
}

void TrackingPanel::setX(double xVal, bool second)
{DD;
    // здесь xVal - произвольное число, соответствующее какому-то положению на оси X

    if (!plot->curves().isEmpty()) {
        //first we need to find the closest point to xVal;

        //ищем минимальный шаг по оси X
        double xstep = plot->curves().at(0)->channel->xStep();

        for (int i=1; i<plot->curvesCount(); ++i) {
            if (plot->curves().at(i)->channel->xStep()<xstep)
                xstep = plot->curves().at(i)->channel->xStep();
        }
        if (xstep==0.0)
            xstep = plot->curves().first()->channel->xStep();

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
        else {//xstep == 0 -> третьоктава или еще что-нибудь, проверяем тип файла
            DfdFileDescriptor *dfd = static_cast<DfdFileDescriptor *>(plot->curves().first()->descriptor);
            if (!dfd || (dfd->DataType != ToSpectr && dfd->DataType != OSpectr))
                return;
            auto iter = closest<QVector<double>::iterator, double>(dfd->channels[0]->XValues.begin(),
                    dfd->channels[0]->XValues.end(), xVal);

            xVal = *iter;
        }
    }

    if (second)
        x2Spin->setValue(xVal);
    else
        x1Spin->setValue(xVal);
}

void TrackingPanel::setStep(double step)
{DD;
    mStep = step;
    x1Spin->setSingleStep(mStep);
    x2Spin->setSingleStep(mStep);
}

void TrackingPanel::switchVisibility()
{DD;
    if (isVisible()) {
        setVisible(false);
        _trackingCursor->detach();
        _trackingCursor1->detach();
        cursorSpan->detach();
    }
    else {
        setVisible(true);
        if (showFirst->isChecked()) _trackingCursor->attach(plot);
        if (showSecond->isChecked()) _trackingCursor1->attach(plot);
        cursorSpan->attach(plot);
    }
}

void TrackingPanel::updateTrackingCursor(double xVal, bool second)
{DD;
    if (!isVisible()) return;

    if (second) showSecond->setChecked(true);
    else showFirst->setChecked(true);

    //3. update our cursors
    if (second) {
        _trackingCursor1->moveTo(xVal);
        _trackingCursor1->attach(plot);
    }
    else {
        _trackingCursor->moveTo(xVal);
        _trackingCursor->attach(plot);
    }

    double leftBorder = _trackingCursor->xValue();
    double rightBorder = _trackingCursor1->xValue();

    if (leftBorder>rightBorder)
        std::swap(leftBorder,rightBorder);
    cursorSpan->setInterval(leftBorder, rightBorder);
    if (!showFirst->isChecked() || !showSecond->isChecked())
        cursorSpan->detach();
    else
        cursorSpan->attach(plot);
    plot->replot();

    //4. get the y values from all graphs
    QList<TrackingPanel::TrackInfo> list;
    QVector<double> yValues;
    foreach(Curve *c, plot->curves()) {
        int steps, steps1, steps2;
        const double channelStep = c->channel->xStep();

        if (channelStep==0.0) {
            auto &xVals = c->channel->xValues();
            auto iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), xVal);
            steps = iter - xVals.begin();
            if (steps < 0) steps = xVals.size()-1;

            iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), leftBorder);
            steps1 = iter - xVals.begin();
            if (steps1 < 0) steps1 = xVals.size()-1;

            iter = closest<QVector<double>::iterator, double>(xVals.begin(), xVals.end(), rightBorder);
            steps2 = iter - xVals.begin();
            if (steps2 < 0) steps2 = xVals.size()-1;
        } else {
            const quint32 samplesCount = c->channel->samplesCount();
            steps = int(xVal/channelStep);
            if (steps < 0) steps=0;
            if (qAbs(channelStep*(steps+1)-xVal) < qAbs(channelStep*steps-xVal)) steps++;
            if (steps > samplesCount) steps=samplesCount;

            steps1 = int(leftBorder/channelStep);
            if (steps1 < 0) steps1=0;
            if (qAbs(channelStep*(steps1+1)-leftBorder) < qAbs(channelStep*steps1-leftBorder)) steps1++;
            if (steps1>samplesCount) steps1=samplesCount;

            steps2 = int(rightBorder/channelStep);
            if (steps2 < 0) steps2=0;
            if (qAbs(channelStep*(steps2+1)-rightBorder) < qAbs(channelStep*steps2-rightBorder)) steps2++;
            if (steps2>samplesCount) steps2=samplesCount;
        }
        if (steps2<steps1) qSwap(steps1, steps2);

        double cumul = pow(10.0, c->channel->yValues().at(steps1)/5.0);

        for (int i=steps1+1; i<=steps2; ++i) {
            cumul += pow(10.0, c->channel->yValues().at(i)/5.0);
        }
        cumul = sqrt(cumul);
        cumul = 10.0*log10(cumul);

        double energy = pow(10.0, c->channel->yValues().at(steps1)/10.0);
        for (int i=steps1+1; i<=steps2; ++i) {
            energy += pow(10.0, c->channel->yValues().at(i)/10.0);
        }
        energy = 10.0*log10(energy);

        double xValue = 0.0;
        if (c->channel->xValues().isEmpty())
            xValue = c->channel->xBegin() + c->channel->xStep()*steps;
        else
            xValue = c->channel->xValues().at(steps);

        TrackingPanel::TrackInfo ti{c->channel->name(), c->channel->color(),
                    xValue,
                    c->channel->yValues().at(steps),
                    cumul, energy};
        list << ti;
        yValues << c->channel->yValues().at(steps);
    }

    if (second) {
        _trackingCursor1->setYValues(yValues);
    }
    else {
        _trackingCursor->setYValues(yValues);
    }

    updateState(list, second);

}

void TrackingPanel::updateTrackingCursor(QwtPlotMarker *cursor, double newVal)
{DD;
    if (!cursor) return;

    if (cursor == _trackingCursor)
        setX(newVal, false);
    else
        if (cursor == _trackingCursor1)
            setX(newVal, true);
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
    _trackingCursor->detach();
    _trackingCursor1->detach();
    cursorSpan->detach();
    QWidget::hideEvent(event);
}
