#include "plot.h"

#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_scale_widget.h>
#include <qwt_color_map.h>

#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "curve.h"
#include "linecurve.h"
#include "barcurve.h"
#include "spectrocurve.h"

#include "chartzoom.h"
#include "colormapfactory.h"

#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_text.h>

#include <QtCore>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>
#include <QMenu>
#include <QAction>
#include <QAudioOutput>

#include "mainwindow.h"
#include "curvepropertiesdialog.h"

#include "colorselector.h"
#include "legend.h"

#include "plottracker.h"
#include "logscaleengine.h"

#include "logging.h"
#include "trackingpanel.h"

#include "dataiodevice.h"
#include "picker.h"

#include "playpanel.h"


// простой фабричный метод создания кривой нужного типа
Curve * createCurve(const QString &legendName, FileDescriptor *descriptor, int channel)
{
    if (descriptor->channel(channel)->data()->blocksCount() > 1)
        return new SpectroCurve(legendName, descriptor, channel);

    // считаем, что шаг по оси х 0 только у октав и третьоктав
    if (descriptor->channel(channel)->xValuesFormat() == DataHolder::XValuesNonUniform)
        return new BarCurve(legendName, descriptor, channel);

    return new LineCurve(legendName, descriptor, channel);
}


Plot::Plot(QWidget *parent) :
    QwtPlot(parent), zoom(0)
{DD;
    _canvas = new QwtPlotCanvas();
    _canvas->setFocusIndicator(QwtPlotCanvas::CanvasFocusIndicator);
    _canvas->setPalette(Qt::white);
    _canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(_canvas);
    //setContextMenuPolicy(Qt::ActionsContextMenu);

    setAutoReplot(true);

    trackingPanel = new TrackingPanel(this);
    trackingPanel->setVisible(false);
    connect(trackingPanel,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), trackingPanel, SLOT(update()));


    playerPanel = new PlayPanel(this);
    playerPanel->setVisible(false);
    connect(playerPanel,SIGNAL(closeRequested()),SIGNAL(playerPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), playerPanel, SLOT(update()));

    axisLabelsVisible = MainWindow::getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;


//    setAxesCount(QwtAxis::xBottom,2);

    // grid
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::darkGray, 0, Qt::DotLine);
    grid->attach(this);

    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, SIGNAL(markedToMove(QwtPlotItem*)),this, SLOT(moveCurve(QwtPlotItem*)));
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);


    zoom = new ChartZoom(this);
    zoom->setZoomEnabled(true);
    connect(zoom,SIGNAL(updateTrackingCursor(double,bool)), trackingPanel, SLOT(setXValue(double,bool)));
    connect(zoom,SIGNAL(contextMenuRequested(QPoint,QwtAxisId)),SLOT(showContextMenu(QPoint,QwtAxisId)));
    connect(zoom,SIGNAL(moveCursor(bool)), trackingPanel, SLOT(moveCursor(bool)));

    tracker = new PlotTracker(this);
    tracker->setEnabled(MainWindow::getSetting("pickerEnabled", true).toBool());

    _picker = new Picker(this);
    _picker->setEnabled(MainWindow::getSetting("pickerEnabled", true).toBool());
    connect(_picker,SIGNAL(setZoomEnabled(bool)), zoom, SLOT(setZoomEnabled(bool)));
    connect(_picker,SIGNAL(cursorSelected(QwtPlotMarker*)), trackingPanel, SLOT(changeSelectedCursor(QwtPlotMarker*)));
    connect(_picker,SIGNAL(xAxisClicked(double,bool)),      trackingPanel, SLOT(setXValue(double,bool)));
    connect(_picker,SIGNAL(cursorMovedTo(double)),          trackingPanel, SLOT(setXValue(double)));
    connect(_picker,SIGNAL(moveCursor(bool)),               trackingPanel, SLOT(moveCursor(bool)));

    connect(_picker,SIGNAL(cursorSelected(QwtPlotMarker*)), playerPanel, SLOT(updateSelectedCursor(QwtPlotMarker*)));
    connect(_picker,SIGNAL(xAxisClicked(double,bool)),      playerPanel, SLOT(setXValue(double)));
    connect(_picker,SIGNAL(cursorMovedTo(double)),          playerPanel, SLOT(setXValue(double)));
}

Plot::~Plot()
{DD;
    delete trackingPanel;
    delete playerPanel;
    qDeleteAll(curves);
    delete grid;
    delete tracker;
    delete _picker;

    MainWindow::setSetting("axisLabelsVisible", axisLabelsVisible);
    MainWindow::setSetting("autoscale-x", !zoom->horizontalScaleBounds->isFixed());
    MainWindow::setSetting("autoscale-y", !zoom->verticalScaleBounds->isFixed());
    MainWindow::setSetting("autoscale-y-slave", !zoom->verticalScaleBoundsSlave->isFixed());
    delete zoom;
}

void Plot::update()
{DD;
    updateAxes();
    updateAxesLabels();
    updateLegend();

    if (leftCurves.isEmpty()) {
        zoom->verticalScaleBounds->reset();
        if (spectrogram)
            zoom->verticalScaleBoundsSlave->reset();
    }
    if (rightCurves.isEmpty() && !spectrogram)
        zoom->verticalScaleBoundsSlave->reset();
    if (!hasCurves())
        zoom->horizontalScaleBounds->reset();

    if (!zoom->horizontalScaleBounds->isFixed())
        zoom->horizontalScaleBounds->autoscale();
    if (!leftCurves.isEmpty() && !zoom->verticalScaleBounds->isFixed())
        zoom->verticalScaleBounds->autoscale();
    if (!rightCurves.isEmpty() && !zoom->verticalScaleBoundsSlave->isFixed() && !spectrogram)
        zoom->verticalScaleBoundsSlave->autoscale();

    replot();
}

bool Plot::hasCurves() const
{DD;
    return !curves.isEmpty();
}

void Plot::deleteAllCurves(bool forceDeleteFixed)
{DD;
    int leftUndeleted = curves.size();

    for (int i=curves.size()-1; i>=0; --i) {
        Curve *c = curves[i];
        if (!c->fixed || forceDeleteFixed) {
            emit curveDeleted(c->descriptor, c->channelIndex);
            deleteCurve(c, true);
            leftUndeleted--;
        }
    }

//    qDeleteAll(curves);
//    curves.clear();
//    leftCurves.clear();
//    rightCurves.clear();
//    ColorSelector::instance()->resetState();
    playerPanel->reset();

//    yLeftName.clear();
//    yRightName.clear();
//    xName.clear();

//    update();
//    emit curvesChanged();
}

void Plot::deleteCurvesForDescriptor(FileDescriptor *descriptor)
{DD;
    for (int i = curves.size()-1; i>=0; --i) {
        Curve *curve = curves[i];
        if (descriptor == curve->descriptor) {
            deleteCurve(curve, true);
        }
    }
}

//не удаляем, если фиксирована
void Plot::deleteCurveFromLegend(QwtPlotItem*item)
{DD;
    if (Curve *c = dynamic_cast<Curve *>(item)) {
        if (!c->fixed) {
            emit curveDeleted(c->descriptor, c->channelIndex);
            deleteCurve(c, true);
        }
    }
}

void Plot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *curve = plotted(dfd, channel)) {
        deleteCurve(curve, doReplot);
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void Plot::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (curve) {
        ColorSelector::instance()->freeColor(curve->pen().color());

        int removed = leftCurves.removeAll(curve);
        if (removed > 0) {
            zoom->verticalScaleBounds->removeToAutoscale(curve->yMin(), curve->yMax());
            if (spectrogram) {
                zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->channel->data()->zMin(),
                                                                  curve->channel->data()->zMax());
            }
        }

        removed = rightCurves.removeAll(curve);
        if (removed > 0) {
            zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->yMin(), curve->yMax());
        }

        removed = curves.removeAll(curve);
        if (removed > 0) {
            zoom->horizontalScaleBounds->removeToAutoscale(curve->xMin(), curve->xMax());
        }
        QString title = curve->title();


        delete curve;
        curve = 0;
        checkDuplicates(title);

        if (leftCurves.isEmpty()) {
            yLeftName.clear();
            if (spectrogram) {
                yRightName.clear();
                enableAxis(QwtAxis::yRight, false);
            }
        }
        if (rightCurves.isEmpty()) {
            yRightName.clear();
            enableAxis(QwtAxis::yRight, false);
        }
        if (!hasCurves()) {
            xName.clear();
        }

        if (doReplot) {
            update();
        }
        emit curvesChanged();
    }
}

void Plot::showContextMenu(const QPoint &pos, QwtAxisId axis)
{DD;
    if (!hasCurves()) return;

    QMenu *menu = new QMenu(this);

    if (axis.pos == QwtAxis::xBottom)
    menu->addAction(xScaleIsLogarithmic?"Линейная шкала":"Логарифмическая шкала", [=](){
        QwtScaleEngine *engine = 0;
        if (!xScaleIsLogarithmic) {
            engine = new LogScaleEngine();
//            engine = new QwtLogScaleEngine();
            engine->setBase(2);
            //engine->setTransformation(new LogTransform);
        }
        else {
            engine = new QwtLinearScaleEngine();
        }
        if (engine)
            setAxisScaleEngine(xBottomAxis, engine);

        xScaleIsLogarithmic = !xScaleIsLogarithmic;
    });

    // определяем, все ли графики представляют временные данные
    bool time = true;

    foreach (Curve *c, curves) {
        if (c->channel->type() != Descriptor::TimeResponse) {
            time = false;
            break;
        }
    }

    if (time && axis.pos == QwtAxis::xBottom) {
        menu->addAction("Сохранить временной сегмент", [=](){
            double xStart = canvasMap(axis).s1();
            double xEnd = canvasMap(axis).s2();

            QList<FileDescriptor*> files;

            foreach(Curve *c, curves) {
                if (!files.contains(c->descriptor))
                    files << c->descriptor;
            }

            emit saveTimeSegment(files, xStart, xEnd);
        });
    }

    QList<Curve*> list;
    int *ax = 0;
    if (spectrogram) {//всё наоборот
        if (axis.pos == QwtAxis::yRight && !leftCurves.isEmpty()) {
            list = leftCurves;
            ax = &yValuesPresentationRight;
        }
    }
    else {
        if (axis.pos == QwtAxis::yLeft && !leftCurves.isEmpty()) {
            list = leftCurves;
            ax = &yValuesPresentationLeft;
        }
        if (axis.pos == QwtAxis::yRight && !rightCurves.isEmpty()) {
            list = rightCurves;
            ax = &yValuesPresentationRight;
        }
    }

    if (!list.isEmpty()) {
        QAction *a = new QAction("Показывать как");
        QMenu *am = new QMenu(this);
        QActionGroup *ag = new QActionGroup(am);

        QAction *act3 = new QAction("Амплитуды линейные", ag);
        act3->setCheckable(true);
        if (ax && *ax == DataHolder::ShowAsAmplitudes) act3->setChecked(true);
        act3->setData(DataHolder::ShowAsAmplitudes);
        am->addAction(act3);

        QAction *act4 = new QAction("Амплитуды в дБ", ag);
        act4->setCheckable(true);
        if (*ax == DataHolder::ShowAsAmplitudesInDB) act4->setChecked(true);
        act4->setData(DataHolder::ShowAsAmplitudesInDB);
        am->addAction(act4);

        QAction *act5 = new QAction("Фазы", ag);
        act5->setCheckable(true);
        if (*ax == DataHolder::ShowAsPhases) act5->setChecked(true);
        act5->setData(DataHolder::ShowAsPhases);
        am->addAction(act5);

        QAction *act1 = new QAction("Действит.", ag);
        act1->setCheckable(true);
        if (*ax == DataHolder::ShowAsReals) act1->setChecked(true);
        act1->setData(DataHolder::ShowAsReals);
        am->addAction(act1);

        QAction *act2 = new QAction("Мнимые", ag);
        act2->setCheckable(true);
        if (*ax == DataHolder::ShowAsImags) act2->setChecked(true);
        act2->setData(DataHolder::ShowAsImags);
        am->addAction(act2);

        connect(ag, &QActionGroup::triggered, [=](QAction*act){
            int presentation = act->data().toInt();
            foreach (Curve *c, list) {
                c->channel->data()->setYValuesPresentation(presentation);
            }
            *ax = presentation;
            this->recalculateScale(axis.pos == QwtAxis::yLeft);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (spectrogram && axis.pos == QwtAxis::yRight && !leftCurves.isEmpty()) {
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(leftCurves.first())) {
            QAction *a = new QAction("Цветовая шкала");
            QMenu *am = new QMenu(this);
            QActionGroup *ag = new QActionGroup(am);

            const QStringList l = ColorMapFactory::names();
            for (int i=0; i<l.size(); ++i) {
                QAction *act1 = new QAction(l.at(i), ag);
                act1->setCheckable(true);
                if (i == colorMap) act1->setChecked(true);
                act1->setData(i);
                am->addAction(act1);
            }

            connect(ag, &QActionGroup::triggered, [=](QAction*act){
                int map = act->data().toInt();
                if (map != colorMap) {
                    colorMap = map;
                    axisWidget(yRightAxis)->setColorMap(c->colorInterval(), ColorMapFactory::map(map));
                    c->setColorMap(ColorMapFactory::map(map));
                    replot();
                }
            });

            a->setMenu(am);
            menu->addAction(a);
        }
    }

    if (!leftCurves.isEmpty() && !rightCurves.isEmpty()) {
        menu->addSection("Левая и правая оси");
        menu->addAction("Совместить нули левой и правой осей", [=](){
            // 1. Центруем нуль левой оси
            QwtScaleMap leftMap = canvasMap(yLeft);
            double s1 = leftMap.s1();
            double s2 = leftMap.s2();

            double p1 = leftMap.p1();
            double p2 = leftMap.p2();
            double delta = leftMap.invTransform((p1+p2)/2.0);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtAxis::yLeft, {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            QwtScaleMap rightMap = canvasMap(yRight);
            s1 = rightMap.s1();
            s2 = rightMap.s2();

            p1 = rightMap.p1();
            p2 = rightMap.p2();
            delta = rightMap.invTransform((p1+p2)/2.0);

            coords.coords.insert(QwtAxis::yRight, {s1-delta, s2-delta});
            zoom->addZoom(coords, true);
        });
        menu->addAction("Совместить диапазоны левой и правой осей", [=](){
            QwtScaleMap leftMap = canvasMap(QwtAxis::yLeft);
            double s1 = leftMap.s1();
            double s2 = leftMap.s2();

            QwtScaleMap rightMap = canvasMap(QwtAxis::yRight);
            double s3 = rightMap.s1();
            double s4 = rightMap.s2();
            double s = std::min(s1,s3);
            double ss = std::min(s2,s4);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtAxis::yLeft, {s, ss});
            coords.coords.insert(QwtAxis::yRight, {s, ss});

            zoom->addZoom(coords, true);
        });
    }
    if (!menu->actions().isEmpty())
        menu->exec(pos);
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch)
{DD;
    if (!hasCurves()) // нет графиков - можем построить что угодно
        return true;
    //особый случай - спектрограмма - всегда одна на графике
    if (ch->data()->blocksCount()>1 && hasCurves()) return false;
    //особый случай - спектрограмма - всегда одна на графике
    if (hasCurves() && curves.constFirst()->channel->data()->blocksCount()>1)
        return false;

    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (leftCurves.isEmpty() || yLeftName.isEmpty() || ch->yName() == yLeftName)
            return true;
    }
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch)
{DD;
    if (!hasCurves()) // нет графиков - всегда на левой оси
        return true;
    //особый случай - спектрограмма - всегда одна на графике
    if (ch->data()->blocksCount()>1 && hasCurves()) return false;
    //особый случай - спектрограмма - всегда одна на графике
    if (hasCurves() && curves.constFirst()->channel->data()->blocksCount()>1)
        return false;

    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (rightCurves.isEmpty() || yRightName.isEmpty() || ch->yName() == yRightName)
            return true;
    }
    return false;
}

void Plot::prepareAxis(QwtAxisId axis)
{DD;
    if (!isAxisVisible(axis)) setAxisVisible(axis);
}

void Plot::setAxis(QwtAxisId axis, const QString &name)
{DD;
    switch (axis.pos) {
        case QwtAxis::yLeft: yLeftName = name; break;
        case QwtAxis::yRight: yRightName = name; break;
        case QwtAxis::xBottom: xName = name; break;
        default: break;
    }
}

void Plot::moveToAxis(int axis, double min, double max)
{DD;
    //запрещаем переносить спектрограмму
    if (spectrogram) return;

    switch (axis) {
        case QwtAxis::xBottom:
            zoom->horizontalScaleBounds->add(min, max);
            if (!zoom->horizontalScaleBounds->isFixed()) zoom->horizontalScaleBounds->autoscale();
            break;
        case QwtAxis::yLeft:
            zoom->verticalScaleBoundsSlave->removeToAutoscale(min, max);
            zoom->verticalScaleBounds->add(min, max);
            if (!zoom->verticalScaleBounds->isFixed()) zoom->verticalScaleBounds->autoscale();
            break;
        case QwtAxis::yRight:
            zoom->verticalScaleBounds->removeToAutoscale(min, max);
            zoom->verticalScaleBoundsSlave->add(min, max);
            if (!zoom->verticalScaleBoundsSlave->isFixed()) zoom->verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{DD;
    if (!spectrogram) {
        if (leftCurves.isEmpty()) enableAxis(QwtAxis::yLeft, false);
        else {
            enableAxis(QwtAxis::yLeft, axisLabelsVisible);
            QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
            QwtText text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix), QwtText::RichText);
            if (axisLabelsVisible)
                setAxisTitle(QwtAxis::yLeft, text);
            else
                setAxisTitle(QwtAxis::yLeft, "");
        }

        if (rightCurves.isEmpty()) enableAxis(QwtAxis::yRight, false);
        else {
            enableAxis(QwtAxis::yRight, axisLabelsVisible);
            QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
            QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
            if (axisLabelsVisible)
                setAxisTitle(QwtAxis::yRight, text);
            else
                setAxisTitle(QwtAxis::yRight, "");
        }
    }
    else {
        //две оси видны всегда
        enableAxis(QwtAxis::yLeft, axisLabelsVisible);
        if (axisLabelsVisible)
            setAxisTitle(QwtAxis::yLeft, QwtText(yLeftName));
        else
            setAxisTitle(QwtAxis::yLeft, "");

        //правая ось - цветовая шкала
        enableAxis(QwtAxis::yRight, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
        QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtAxis::yRight, text);
        else
            setAxisTitle(QwtAxis::yRight, "");
    }

    if (axisEnabled(QwtAxis::xBottom)) {
        setAxisTitle(QwtAxis::xBottom, axisLabelsVisible ? xName : "");
    }
}

void Plot::setScale(QwtAxisId id, double min, double max, double step)
{
    setAxisScale(id, min, max, step);
    if (!curves.isEmpty()) {
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(curves.first())) {
            if (id == yRightAxis) {
                c->setColorInterval(min, max);
                axisWidget(id)->setColorMap(QwtInterval(min, max), ColorMapFactory::map(colorMap));
                replot();
            }
        }
    }
}

void Plot::removeLabels()
{
    foreach (Curve *c, curves) {
        c->removeLabels();
    }
   replot();
}

void Plot::moveCurve(Curve *curve, int axis)
{DD;
    if (spectrogram) return;

    if ((axis == QwtAxis::yLeft && canBePlottedOnLeftAxis(curve->channel))
        || (axis == QwtAxis::yRight && canBePlottedOnRightAxis(curve->channel))) {
        prepareAxis(axis);
        setAxis(axis, curve->channel->yName());
        curve->setYAxis(axis);

        if (leftCurves.contains(curve)) {
            leftCurves.removeAll(curve);
            if (rightCurves.isEmpty()) {
                yValuesPresentationRight = curve->channel->data()->yValuesPresentation();
            }
            curve->channel->data()->setYValuesPresentation(yValuesPresentationRight);
            rightCurves.append(curve);
        }
        else if (rightCurves.contains(curve)) {
            rightCurves.removeAll(curve);
            if (leftCurves.isEmpty()) {
                yValuesPresentationLeft = curve->channel->data()->yValuesPresentation();
            }
            curve->channel->data()->setYValuesPresentation(yValuesPresentationLeft);
            leftCurves.append(curve);
        }
//        emit curvesChanged();

        updateAxesLabels();
        moveToAxis(axis, curve->channel->data()->yMin(), curve->channel->data()->yMax());
    }
    else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");


}

void Plot::moveCurve(QwtPlotItem *curve)
{
    if (Curve *c = dynamic_cast<Curve*>(curve))
        moveCurve(c, c->yAxis() == QwtAxis::yLeft ? QwtAxis::yRight : QwtAxis::yLeft);
}

void Plot::fixCurve(QwtPlotItem *curve)
{
    if (Curve *c = dynamic_cast<Curve*>(curve)) {
        c->switchFixed();
        updateLegend();
    }

}

bool Plot::hasDuplicateNames(const QString name) const
{
    int count = 0;
    foreach(Curve *c, curves) {
        if (c->title() == name) count++;
    }
    return (count > 1);
}

void Plot::checkDuplicates(const QString name)
{
    QList<int> l;
    for(int i=0; i<curves.size(); ++i) {
        if (curves[i]->title() == name) l << i;
    }
    foreach(int i, l) curves[i]->duplicate = l.size()>1;
}

QString Plot::yValuesPresentationSuffix(int yValuesPresentation) const
{
    switch (DataHolder::YValuesPresentation(yValuesPresentation)) {
        case DataHolder::ShowAsDefault: return QString();
        case DataHolder::ShowAsReals: return " [Re]";
        case DataHolder::ShowAsImags: return " [Im]";
        case DataHolder::ShowAsAmplitudes: return " [Abs]";
        case DataHolder::ShowAsAmplitudesInDB: return " [dB]";
        case DataHolder::ShowAsPhases: return " [Phase]";
        default: break;
    }
    return QString();
}

void Plot::recalculateScale(bool leftAxis)
{
    ChartZoom::ScaleBounds *ybounds = 0;
    if (leftAxis) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    QList<Curve*> l;
    if (leftAxis || spectrogram) l = leftCurves;
    else l = rightCurves;

    ybounds->reset();
    foreach (Curve *c, l) {
        ybounds->add(c->yMin(), c->yMax());
    }
}

bool Plot::plotCurve(FileDescriptor *descriptor, int channel, QColor *col, bool &plotOnRight, int fileNumber)
{DD;
    if (plotted(descriptor, channel)) return false;

    Channel *ch = descriptor->channel(channel);
    if (!ch->populated()) {
        ch->populate();
    }

    bool spectrogr = ch->data()->blocksCount()>1;

    bool plotOnFirstYAxis = canBePlottedOnLeftAxis(ch);
    bool plotOnSecondYAxis = canBePlottedOnRightAxis(ch);
    bool skipped = false;

    if (!plotOnRight) {//trying to plot on left
        if (!plotOnFirstYAxis) {//cannot plot on left
            if (!plotOnSecondYAxis) {//cannot plot on right either
                skipped = true;
            }
            else plotOnRight = true; //confirm plotting on right
        }
    }
    else /*if (plotOnRight)*/ {//trying to plot on right
        if (!plotOnSecondYAxis) {//cannot plot on right
            if (!plotOnFirstYAxis) {//cannot plot on left either
                skipped = true;
            }
            else plotOnRight = false; //confirm plotting on left
        }
    }

    if (skipped) {
        QMessageBox::warning(this, QString("Не могу построить канал"),
                             QString("Тип графика не подходит.\n"
                                     "Сначала очистите график."));
        return false;
    }
    spectrogram = spectrogr;

    setAxis(xBottomAxis, descriptor->xName());
    prepareAxis(xBottomAxis);

    QwtAxisId ax = yLeftAxis;
    if (!spectrogram) {
        // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
        if (!plotOnRight && leftCurves.isEmpty()) {
            yValuesPresentationLeft = ch->data()->yValuesPresentation();
        }
        if (plotOnRight && rightCurves.isEmpty()) {
            yValuesPresentationRight = ch->data()->yValuesPresentation();
        }

        if (!plotOnRight) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
        else ch->data()->setYValuesPresentation(yValuesPresentationRight);

        ax = plotOnRight ? yRightAxis : yLeftAxis;

        axisWidget(yRightAxis)->setColorBarEnabled(false);

        setAxis(ax, ch->yName());
        prepareAxis(ax);
    }
    else {
        yValuesPresentationLeft = DataHolder::ShowAsReals;
        yValuesPresentationRight = ch->data()->yValuesPresentation();
        ch->data()->setYValuesPresentation(yValuesPresentationRight);
        ax = yLeftAxis;
        setAxis(ax, ch->zName());
        prepareAxis(ax);
        plotOnRight = false;

        setAxis(yRightAxis, ch->yName());
        setAxisVisible(yRightAxis);

        axisWidget(yRightAxis)->setColorBarEnabled(true);
//        plotLayout()->setAlignCanvasToScales(true);
    }


    Curve *g = createCurve(ch->legendName(), descriptor, channel);
    QColor nextColor = ColorSelector::instance()->getColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    g->oldPen = pen;
    if (col) *col = nextColor;

    curves << g;
    g->fileNumber = fileNumber;
    checkDuplicates(g->title());
    if (hasDuplicateNames(g->title())) {
        g->duplicate = true;
    }

    g->setYAxis(ax);
    if (plotOnRight) {
        rightCurves << g;
    }
    else {
        leftCurves << g;
    }


    if (spectrogram) {
        axisWidget(yRightAxis)->setColorMap(QwtInterval(ch->data()->yMin(-1), ch->data()->yMax(-1)),
                                             ColorMapFactory::map(colorMap));
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(g)) {
            c->setColorMap(ColorMapFactory::map(colorMap));
        }

        zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
        zoom->verticalScaleBoundsSlave->add(g->yMin(), g->yMax());
        zoom->verticalScaleBounds->add(g->channel->data()->zMin(), g->channel->data()->zMax());
    }
    else {
        ChartZoom::ScaleBounds *ybounds = 0;
        if (zoom->verticalScaleBounds->axis == ax.pos) ybounds = zoom->verticalScaleBounds;
        else ybounds = zoom->verticalScaleBoundsSlave;

        zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
        if (ybounds) ybounds->add(g->yMin(), g->yMax());
    }




    g->attachTo(this);

    update();
    emit curvesChanged();

    return true;
}

Curve * Plot::plotted(FileDescriptor *dfd, int channel) const
{DD;
    foreach (Curve *curve, curves) {
        if (curve->descriptor == dfd && curve->channelIndex == channel) return curve;
    }
    return 0;
}

Curve * Plot::plotted(Channel *channel) const
{DD;
    foreach (Curve *curve, curves) {
        if (curve->channel == channel) return curve;
    }
    return 0;
}

Range Plot::xRange() const
{
    QwtScaleMap sm = canvasMap(QwtAxis::xBottom);
    return {sm.s1(), sm.s2()};
}

Range Plot::yLeftRange() const
{
    QwtScaleMap sm = canvasMap(QwtAxis::yLeft);
    return {sm.s1(), sm.s2()};
}

Range Plot::yRightRange() const
{
    QwtScaleMap sm = canvasMap(QwtAxis::yRight);
    return {sm.s1(), sm.s2()};
}

void Plot::switchLabelsVisibility()
{
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DD;
    foreach (Curve *curve, leftCurves) {
        curve->setTitle(curve->channel->legendName());
    }
    foreach (Curve *curve, rightCurves) {
        curve->setTitle(curve->channel->legendName());
    }
    updateLegend();
}

void Plot::savePlot()
{DD;
    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture,
                                               "Растровые изображения (*.bmp);;Файлы pdf (*.pdf);;Файлы svg (*.svg)");
    if (lastPicture.isEmpty()) return;

    importPlot(lastPicture);

    MainWindow::setSetting("lastPicture", lastPicture);
}

void Plot::copyToClipboard()
{DD;
    QTemporaryFile file("DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        importPlot(fileName);
        QImage img;
        if (img.load(fileName)) {
            qApp->clipboard()->setImage(img);
        }
    }
}

void Plot::print()
{DD;
    QTemporaryFile file("DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        importPlot(fileName);

        QImage img;
        if (img.load(fileName)) {
            QPrinter printer;
            printer.setOrientation(QPrinter::Landscape);

            QPrintDialog printDialog(&printer, this);
            if (printDialog.exec()) {
                qreal left,right,top,bottom;
                printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
                left = 15.0;
                top = 40.0;
                printer.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
                QPainter painter(&printer);
                QRect rect = painter.viewport();
                QSize size = img.size();
                size.scale(rect.size(), Qt::KeepAspectRatio);
                painter.setViewport(rect.x(), rect.y(),
                                    size.width(), size.height());
                painter.setWindow(img.rect());
                painter.drawImage(0, 0, img);
            }
        }
    }
}

void Plot::switchInteractionMode()
{DD;
    if (interactionMode == ScalingInteraction) {
        setInteractionMode(DataInteraction);
    }
    else {
        setInteractionMode(ScalingInteraction);
    }
}

void Plot::switchTrackingCursor()
{DD;
    trackingPanel->switchVisibility();
}

void Plot::switchPlayerVisibility()
{DD;
    playerPanel->switchVisibility();
}

void Plot::toggleAutoscale(int axis, bool toggled)
{
    switch (axis) {
        case 0: // x axis
            zoom->horizontalScaleBounds->setFixed(!toggled);
            break;
        case 1: // y axis
            zoom->verticalScaleBounds->setFixed(!toggled);
            break;
        case 2: // y slave axis
            zoom->verticalScaleBoundsSlave->setFixed(!toggled);
            break;
        default:
            break;
    }
}

void Plot::autoscale(int axis)
{
    switch (axis) {
        case 0: // x axis
            zoom->horizontalScaleBounds->autoscale();
            break;
        case 1: // y axis
            zoom->verticalScaleBounds->autoscale();
            break;
        case 2: // y slave axis
            zoom->verticalScaleBoundsSlave->autoscale();
            break;
        case -1:
            zoom->horizontalScaleBounds->autoscale();
            zoom->verticalScaleBounds->autoscale();
            if (!spectrogram) zoom->verticalScaleBoundsSlave->autoscale();
            break;
        default:
            break;
    }
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DD;
    interactionMode = mode;
    if (_picker) _picker->setMode(mode);
    if (zoom)
        zoom->setZoomEnabled(mode == ScalingInteraction);
    if (_canvas) _canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::importPlot(const QString &fileName)
{DD;
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtAxis::yLeft);
    axisfont.setPointSize(axisfont.pointSize()+1);

    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *curve, curves) {
        QPen pen = curve->pen();
        if (pen.width()<2) pen.setWidth(2);
        pen.setColor(pen.color().lighter(120));
        curve->setPen(pen);
//        curve->setTitle(QwtText("<font size=5>"+curve->channel->legendName()+"</font>"));
    }

    QwtAbstractLegend *leg = new QwtLegend();
    insertLegend(leg, QwtPlot::BottomLegend);


    QString format = fileName.section(".", -1,-1);
    renderer.renderDocument(this, fileName, format,
                            QSizeF(400,200), qApp->desktop()->logicalDpiX());


    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *curve, curves) {
        curve->setPen(curve->oldPen);
        curve->setTitle(curve->channel->legendName());
    }

    leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, SIGNAL(markedToMove(QwtPlotItem*)),this, SLOT(moveCurve(QwtPlotItem*)));
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void Plot::switchCursor()
{DD;
    if (!_picker) return;

    bool pickerEnabled = _picker->isEnabled();
    _picker->setEnabled(!pickerEnabled);
    tracker->setEnabled(!pickerEnabled);
    MainWindow::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(const QVariant &itemInfo, int index)
{DD;
    Q_UNUSED(index)

    QwtPlotItem *item = infoToItem(itemInfo);
    if (item) {
        Curve *c = dynamic_cast<Curve *>(item);
        if (c) {
            CurvePropertiesDialog dialog(c, this);
            connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
            connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
            dialog.exec();
        }
    }
}

void Plot::editLegendItem(QwtPlotItem *item)
{DD;
    Curve *c = dynamic_cast<Curve *>(item);
    if (c) {
        CurvePropertiesDialog dialog(c, this);
        connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
        connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
        dialog.exec();
    }
}
