#include "plot.h"

#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_map.h>
#include <qwt_color_map.h>

#include "fileformats/filedescriptor.h"
#include "curve.h"
#include "linecurve.h"
#include "barcurve.h"
#include "spectrocurve.h"

#include "zoomstack.h"
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
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>
#include <QMenu>
#include <QAction>
#include <QAudioOutput>

#include "mainwindow.h"
#include "curvepropertiesdialog.h"
#include "settings.h"
#include "colorselector.h"
#include "legend.h"

#include "plottracker.h"
#include "logscaleengine.h"

#include "logging.h"
//#include "trackingpanel.h"
#include "trackingcursor.h"

#include "dataiodevice.h"
#include "picker.h"

#include "playpanel.h"
#include "channelsmimedata.h"
#include "imagerenderdialog.h"
#include "enums.h"

#include "unitsconverter.h"
#include "canvaseventfilter.h"
#include "dragzoom.h"
#include "wheelzoom.h"
#include "axiszoom.h"
#include "plotzoom.h"
#include "scaledraw.h"
#include "grid.h"
#include "plotinfooverlay.h"
#include "plotmodel.h"
#include "cursors.h"
#include "cursorbox.h"

Plot::Plot(PlotType type, QWidget *parent) :
    QwtPlot(parent), plotType(type)
{DDD;
    m = new PlotModel(this);

    QVariantList list = Settings::getSetting("colors").toList();
    colors = new ColorSelector(list);
    _canvas = new QwtPlotCanvas(this);
    _canvas->setFocusIndicator(QwtPlotAbstractCanvas::CanvasFocusIndicator);
    _canvas->setFrameStyle(QFrame::NoFrame);
    plotLayout()->setCanvasMargin(0);
    setCanvas(_canvas);
    setCanvasBackground(Qt::white);



    setAutoReplot(true);
    setAcceptDrops(true);

    infoOverlay = new PlotInfoOverlay(this);
    infoOverlay->setVisible(true);

    setAxisScaleDraw(QwtAxis::XBottom, new ScaleDraw());
    setAxisScaleDraw(QwtAxis::YLeft, new ScaleDraw());
    setAxisScaleDraw(QwtAxis::YRight, new ScaleDraw());

    axisWidget(QwtAxis::XBottom)->setMouseTracking(true);
    axisWidget(QwtAxis::YLeft)->setMouseTracking(true);
    axisWidget(QwtAxis::YRight)->setMouseTracking(true);

    leftOverlay = new LeftAxisOverlay(this);
    rightOverlay = new RightAxisOverlay(this);

    axisLabelsVisible = Settings::getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;


    // grid
    grid = new Grid(this);

    createLegend();

    cursors = new Cursors(this);
    connect(this, &Plot::curvesCountChanged, cursors, &Cursors::update);

    cursorBox = new CursorBox(cursors, this);
    cursorBox->setWindowTitle(parent->windowTitle());
    cursorBox->setVisible(false);
    connect(this, &Plot::curvesCountChanged, cursorBox, &CursorBox::updateLayout);
    connect(cursorBox,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));


    zoom = new ZoomStack(this);

    tracker = new PlotTracker(this);
//    tracker->setEnabled(Settings::getSetting("pickerEnabled", true).toBool());

    picker = new Picker(this);
    picker->setPickPriority(Picker::PickPriority::PickLast);
    picker->setEnabled(Settings::getSetting("pickerEnabled", true).toBool());
    connect(picker, &Picker::removeNeeded, cursors, qOverload<Selectable*>(&Cursors::removeCursor));

    dragZoom = new DragZoom(this);
    wheelZoom = new WheelZoom(this);
    plotZoom = new PlotZoom(this);

    axisZoom = new AxisZoom(this);
    connect(axisZoom, &AxisZoom::hover, this, &Plot::hoverAxis);

    canvasFilter = new CanvasEventFilter(this);
    canvasFilter->setZoom(zoom);
    canvasFilter->setDragZoom(dragZoom);
    canvasFilter->setWheelZoom(wheelZoom);
    canvasFilter->setAxisZoom(axisZoom);
    canvasFilter->setPlotZoom(plotZoom);
    canvasFilter->setPicker(picker);
    connect(canvasFilter, &CanvasEventFilter::hover, this, &Plot::hoverAxis);
    connect(canvasFilter, &CanvasEventFilter::contextMenuRequested, this, &Plot::showContextMenu);
}

Plot::~Plot()
{DDD;
    deleteAllCurves(true);

//    delete trackingPanel;
    delete cursorBox;
    delete grid;
    delete tracker;
    delete picker;

    Settings::setSetting("axisLabelsVisible", axisLabelsVisible);
    Settings::setSetting("autoscale-x", !zoom->horizontalScaleBounds->isFixed());
    Settings::setSetting("autoscale-y", !zoom->verticalScaleBounds->isFixed());
    Settings::setSetting("autoscale-y-slave", !zoom->verticalScaleBoundsSlave->isFixed());
    delete zoom;
    delete dragZoom;
    delete wheelZoom;
    delete axisZoom;
    delete canvasFilter;
    delete colors;
}

double Plot::screenToPlotCoordinates(Enums::AxisType axis, double value)
{
    return invTransform(::toQwtAxisType(axis), value);
}

double Plot::plotToScreenCoordinates(Enums::AxisType axis, double value)
{
    return transform(::toQwtAxisType(axis), value);
}

Range Plot::plotRange(Enums::AxisType axis)
{
    auto map = canvasMap(toQwtAxisType(axis));
    return {map.s1(), map.s2()};
}

Range Plot::screenRange(Enums::AxisType axis)
{
    auto map = canvasMap(toQwtAxisType(axis));
    return {map.p1(), map.p2()};
}

void Plot::updatePlottedIndexes()
{DDD;
    if (!sergeiMode) m->updatePlottedIndexes();
}

void Plot::plotCurvesForDescriptor(FileDescriptor *d, int fileIndex)
{DD;
    m->updatePlottedIndexes(d, fileIndex);
    const auto plotted = m->plottedIndexes();
    for (const auto &i: plotted) plotChannel(i.ch, i.onLeft, i.fileIndex);
}

void Plot::update()
{DDD;
    updateAxes();
    updateLabels();
    updateAxesLabels();
    updateLegend();

    updateBounds();

    replot();
}

void Plot::updateBounds()
{DDD;
    if (m->leftCurvesCount()==0)
        zoom->verticalScaleBounds->reset();
    if (m->rightCurvesCount()==0)
        zoom->verticalScaleBoundsSlave->reset();
    if (!hasCurves())
        zoom->horizontalScaleBounds->reset();

    if (!zoom->horizontalScaleBounds->isFixed())
        zoom->horizontalScaleBounds->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->verticalScaleBounds->isFixed())
        zoom->verticalScaleBounds->autoscale();
    if (m->rightCurvesCount()>0 && !zoom->verticalScaleBoundsSlave->isFixed())
        zoom->verticalScaleBoundsSlave->autoscale();
}

void Plot::setRightScale(Enums::AxisType id, double min, double max)
{DDD;
    Q_UNUSED(id);
    Q_UNUSED(min);
    Q_UNUSED(max);
}

QMenu *Plot::createMenu(Enums::AxisType axis, const QPoint &pos)
{DDD;
    QMenu *menu = new QMenu(this);

    if (axis == Enums::AxisType::atBottom) {
        auto scm = new QMenu("Одинарный курсор", this);
        scm->addAction("Вертикальный", [=](){
            cursors->addSingleCursor(canvas()->mapFromGlobal(pos), Cursor::Style::Vertical);
        });
        scm->addAction("Перекрестье", [=](){
            cursors->addSingleCursor(canvas()->mapFromGlobal(pos), Cursor::Style::Cross);
        });
        menu->addMenu(scm);
        if (type() != PlotType::Spectrogram) {
            auto dcm = new QMenu("Двойной курсор", this);
            dcm->addAction("Стандартный", [=](){
                cursors->addDoubleCursor(canvas()->mapFromGlobal(pos), Cursor::Style::Vertical);
            });
            dcm->addAction("Режекция", [=](){
                cursors->addRejectCursor(canvas()->mapFromGlobal(pos), Cursor::Style::Vertical);
            });
            menu->addMenu(dcm);
        }
        menu->addAction("Гармонический курсор", [=](){
            cursors->addHarmonicCursor(canvas()->mapFromGlobal(pos));
        });

        menu->addAction(xScaleIsLogarithmic?"Линейная шкала":"Логарифмическая шкала", [=](){
            if (xScaleIsLogarithmic)
                setAxisScaleEngine(QwtAxis::XBottom, new QwtLinearScaleEngine());
            else
                setAxisScaleEngine(QwtAxis::XBottom, new LogScaleEngine(2));

            xScaleIsLogarithmic = !xScaleIsLogarithmic;
        });
    }

    // определяем, все ли графики представляют временные данные
    if (const auto type = m->curvesDataType();
        type == Descriptor::TimeResponse && axis == Enums::AxisType::atBottom) {
        menu->addAction("Сохранить временной сегмент", [=](){
            auto range = plotRange(axis);

            emit saveTimeSegment(m->plottedDescriptors(), range.min, range.max);
        });
    }

    bool curvesEmpty = true;
    bool leftCurves = true;
    int *ax = 0;

    if (axis == Enums::AxisType::atLeft && m->leftCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationLeft;
    }
    if (axis == Enums::AxisType::atRight && m->rightCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationRight;
        leftCurves = false;
    }

    if (!curvesEmpty) {
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
            m->setYValuesPresentation(leftCurves, presentation);
            *ax = presentation;
            this->recalculateScale(axis == Enums::AxisType::atLeft);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (m->leftCurvesCount()>0 && m->rightCurvesCount()>0) {
        menu->addSection("Левая и правая оси");
        menu->addAction("Совместить нули левой и правой осей", [=](){
            // 1. Центруем нуль левой оси
            auto range = plotRange(Enums::AxisType::atLeft);
            double s1 = range.min;
            double s2 = range.max;

            range = screenRange(Enums::AxisType::atLeft);

            double p1 = range.min;
            double p2 = range.max;
            double delta = screenToPlotCoordinates(Enums::AxisType::atLeft, (p1+p2)/2.0);

            ZoomStack::zoomCoordinates coords;
            coords.coords.insert(Enums::AxisType::atLeft, {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            range = plotRange(Enums::AxisType::atRight);
            s1 = range.min;
            s2 = range.max;

            range = screenRange(Enums::AxisType::atRight);

            p1 = range.min;
            p2 = range.max;
            delta = screenToPlotCoordinates(Enums::AxisType::atRight, (p1+p2)/2.0);

            coords.coords.insert(Enums::AxisType::atRight, {s1-delta, s2-delta});
            zoom->addZoom(coords, true);
        });
        menu->addAction("Совместить диапазоны левой и правой осей", [=](){
            auto range = plotRange(Enums::AxisType::atLeft);
            double s1 = range.min;
            double s2 = range.max;

            range = plotRange(Enums::AxisType::atRight);
            double s3 = range.min;
            double s4 = range.max;
            double s = std::min(s1,s3);
            double ss = std::min(s2,s4);

            ZoomStack::zoomCoordinates coords;
            coords.coords.insert(Enums::AxisType::atLeft, {s, ss});
            coords.coords.insert(Enums::AxisType::atRight, {s, ss});

            zoom->addZoom(coords, true);
        });
    }
    return menu;
}

void Plot::setInfoVisible(bool visible)
{DDD;
    infoOverlay->setVisible(visible);
}

void Plot::cycleChannels(bool up)
{DD;
    //есть список кривых, возможно, из разных записей. Необходимо для каждой записи
    //получить список индексов, сдвинуть этот список вверх или вниз,
    //а затем удалить имеющиеся кривые и построить сдвинутые, причем соблюдая порядок
    //отображения.
    //Т.о. :
    //- для каждой кривой определяем запись, номер канала. Если возможно, сдвигаем
    //номер канала, запоминаем канал


    m->cycleChannels(up);

    sergeiMode = true;
    deleteAllCurves();
    for (const auto &c: m->plottedIndexes()) {
        plotChannel(c.ch, c.onLeft, c.fileIndex);
    }
    sergeiMode = false;
}

bool Plot::hasCurves() const
{DDD;
    return !m->isEmpty();
}

int Plot::curvesCount(int type) const
{DDD;
    return m->size(type);
}

void Plot::deleteAllCurves(bool forceDeleteFixed)
{DD;
    for (int i=m->size()-1; i>=0; --i) {
        Curve *c = m->curve(i);
        if (!c->fixed || forceDeleteFixed) {
            deleteCurve(c, i==0);
        }
    }

    if (!sergeiMode) {
        updatePlottedIndexes();
       // playerPanel->reset();
        emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

void Plot::deleteCurvesForDescriptor(FileDescriptor *descriptor)
{DDD;
    for (int i = m->size()-1; i>=0; --i) {
        if (Curve *curve = m->curve(i); descriptor == curve->channel->descriptor()) {
            deleteCurve(curve, true);
        }
    }
    updatePlottedIndexes();
    emit curvesCountChanged(); //->MainWindow.updateActions
}

//не удаляем, если фиксирована
void Plot::deleteCurveFromLegend(QwtPlotItem *item)
{DDD;
    if (Curve *c = dynamic_cast<Curve *>(item); !c->fixed) {
        deleteCurve(c, true);
        updatePlottedIndexes();
        emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

void Plot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DDD;
    if (Curve *curve = m->plotted(dfd->channel(channel))) {
        deleteCurve(curve, doReplot);
        updatePlottedIndexes();
        emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

void Plot::deleteSelectedCurve(Selectable *selected)
{DDD;
    if (Curve *curve = dynamic_cast<Curve*>(selected)) {
        deleteCurve(curve, true);
        updatePlottedIndexes();
        emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void Plot::deleteCurve(Curve *curve, bool doReplot)
{DDD;
    if (!curve) return;
    if (curve->selected()) picker->deselect();

    bool removedFromLeft = true;
    if (m->deleteCurve(curve, &removedFromLeft)) {
        emit curveDeleted(curve->channel); //->MainWindow.onChannelChanged
        colors->freeColor(curve->pen().color());

        if (removedFromLeft > 0) {
            zoom->verticalScaleBounds->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        else {
            zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        zoom->horizontalScaleBounds->removeToAutoscale(curve->xMin(), curve->xMax());

        delete curve;

        if (m->leftCurvesCount()==0) {
            yLeftName.clear();
        }
        if (m->rightCurvesCount()==0) {
            yRightName.clear();
            enableAxis(QwtAxis::YRight, false);
        }
        if (!hasCurves()) xName.clear();
        setInfoVisible(m->size()==0);
        if (doReplot) update();
    }
}

void Plot::showContextMenu(const QPoint &pos, Enums::AxisType axis)
{DDD;
    if (!hasCurves()) return;

    QMenu *menu = createMenu(axis, pos);

    if (!menu->actions().isEmpty())
        menu->exec(pos);
    menu->deleteLater();
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{DDD;
//    if (!hasCurves()) // нет графиков - можем построить что угодно
//        return true;
//    //особый случай - спектрограмма - всегда одна на графике
//    if (ch->data()->blocksCount()>1 && hasCurves()) return false;
//    //особый случай - спектрограмма - всегда одна на графике
//    if (auto curve = m->curve(0); curve->channel->data()->blocksCount()>1)
//        return false;

    //не можем строить временные графики на графике спектров
    if (ch->type() == Descriptor::TimeResponse) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0 || yLeftName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yLeftName))
            return true;
        else if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{DDD;
//    if (!hasCurves()) // нет графиков - всегда на левой оси
//        return true;
//    //особый случай - спектрограмма - всегда одна на графике
//    if (ch->data()->blocksCount()>1 && hasCurves()) return false;
//    //особый случай - спектрограмма - всегда одна на графике
//    if (auto curve = m->curve(0); curve->channel->data()->blocksCount()>1)
//        return false;

    //не можем строить временные графики на графике спектров
    if (ch->type() == Descriptor::TimeResponse) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->rightCurvesCount()==0 || yRightName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName))
            return true;
        else if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

QString Plot::pointCoordinates(const QPointF &pos)
{DDD;
    return smartDouble(pos.x())+", "+smartDouble(pos.y());
}

Curve *Plot::createCurve(const QString &legendName, Channel *channel)
{DDD;
    //стандартный график может отображать как обычные кривые, так и третьоктавы

    // считаем, что шаг по оси х 0 только у октав и третьоктав
    if (channel->data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
        if (Settings::getSetting("plotOctaveAsHistogram", false).toBool())
            return new BarCurve(legendName, channel);
    }

    return new LineCurve(legendName, channel);
}

void Plot::prepareAxis(Enums::AxisType axis)
{DDD;
    //if (!isAxisVisible(axis)) setAxisVisible(axis);
}

void Plot::setAxis(Enums::AxisType axis, const QString &name)
{DDD;
    switch (axis) {
        case Enums::AxisType::atLeft: yLeftName = name; break;
        case Enums::AxisType::atRight: yRightName = name; break;
        case Enums::AxisType::atBottom: xName = name; break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{DDD;
    if (m->leftCurvesCount()==0) enableAxis(QwtAxis::YLeft, false);
    else {
        enableAxis(QwtAxis::YLeft, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
        QwtText text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtAxis::YLeft, text);
        else
            setAxisTitle(QwtAxis::YLeft, "");
    }

    if (m->rightCurvesCount()==0) enableAxis(QwtAxis::YRight, false);
    else {
        enableAxis(QwtAxis::YRight, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
        QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtAxis::YRight, text);
        else
            setAxisTitle(QwtAxis::YRight, "");
    }

    if (axisEnabled(QwtAxis::XBottom)) {
        setAxisTitle(QwtAxis::XBottom, axisLabelsVisible ? xName : "");
    }
}

void Plot::setScale(Enums::AxisType id, double min, double max, double step)
{DDD;
    setRightScale(id, min, max);
    setAxisScale(toQwtAxisType(id), min, max, step);
}

void Plot::removeLabels()
{DDD;
    m->removeLabels();
    replot();
}

void Plot::moveCurve(Curve *curve, Enums::AxisType axis)
{DDD;
    if (type()==Plot::PlotType::Spectrogram) return;

    if ((axis == Enums::AxisType::atLeft && canBePlottedOnLeftAxis(curve->channel))
        || (axis == Enums::AxisType::atRight && canBePlottedOnRightAxis(curve->channel))) {
        prepareAxis(axis);
        setAxis(axis, curve->channel->yName());
        curve->setYAxis(axis);

        if (axis == Enums::AxisType::atRight && m->rightCurvesCount()==0)
            yValuesPresentationRight = curve->channel->data()->yValuesPresentation();
        if (axis == Enums::AxisType::atLeft && m->leftCurvesCount()==0)
            yValuesPresentationLeft = curve->channel->data()->yValuesPresentation();

        bool moved = m->moveToOtherAxis(curve);
        if (moved) {
            curve->channel->data()->setYValuesPresentation(axis == Enums::AxisType::atRight ? yValuesPresentationRight
                                                                                   : yValuesPresentationLeft);
        }
        emit curvesCountChanged();

        updateAxesLabels();
        zoom->moveToAxis(axis, curve->channel->data()->yMin(), curve->channel->data()->yMax());
    }
    else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");
}

void Plot::fixCurve(QwtPlotItem *curve)
{DDD;
    if (Curve *c = dynamic_cast<Curve*>(curve)) {
        c->switchFixed();
        updateLegend();
    }
}

void Plot::hoverAxis(Enums::AxisType axis, int hover)
{DDD;
    if (ScaleDraw * scale = dynamic_cast<ScaleDraw*>(axisScaleDraw(toQwtAxisType(axis)))) {
        if (scale->hover != hover) {
            scale->hover = hover;
            axisWidget(toQwtAxisType(axis))->update();
        }
    }
}

QColor Plot::getNextColor()
{DDD;
    return colors->getColor();
}

//void Plot::checkDuplicates(const QString name)
//{DDD;
//    Curve *c1 = nullptr;
//    int found = 0;
//    for (auto c: qAsConst(curves)) {
//        if (c->title() == name) {
//            if (found)
//                c->duplicate = true;
//            else
//                c1 = c;
//            found++;
//        }
//    }
//    if (c1) c1->duplicate = found>1;
//}

QString Plot::yValuesPresentationSuffix(int yValuesPresentation) const
{DDD;
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

void Plot::createLegend()
{DDD;
    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, &CheckableLegend::markedToMove, this, [this](QwtPlotItem*curve){
        if (Curve *c = dynamic_cast<Curve*>(curve))
            moveCurve(c, c->yAxis() == Enums::AxisType::atLeft ? Enums::AxisType::atRight : Enums::AxisType::atLeft);
    });
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void Plot::saveSpectrum(double zVal)
{DDD;
    emit saveHorizontalSlice(zVal);
}

void Plot::saveThroughput(double xVal)
{DDD;
    emit saveVerticalSlice(xVal);
}

void Plot::updateLabels()
{
    for (auto curve: m->curves()) {
        curve->updateLabels();
    }
}

void Plot::recalculateScale(bool leftAxis)
{DDD;
    ZoomStack::ScaleBounds *ybounds = 0;
    if (leftAxis) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;
    ybounds->reset();

    const bool left = leftAxis || type()==Plot::PlotType::Spectrogram;
    const auto count = left?m->leftCurvesCount():m->rightCurvesCount();
    for (int i=0; i<count; ++i) {
        auto curve = m->curve(i,left);
        ybounds->add(curve->yMin(), curve->yMax());;
    }
}

void Plot::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{DD;
    if (!ch) return;
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    QString message;
    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch, &message)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch, &message))) {
        QMessageBox::warning(this, QString("Не могу построить канал"),
                             QString("%1.\nСначала очистите график.").arg(message));
        return;
    }

    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(Enums::AxisType::atBottom, ch->xName());
    prepareAxis(Enums::AxisType::atBottom);

    Enums::AxisType ax = Enums::AxisType::atLeft;
    // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
    if (plotOnLeft && m->leftCurvesCount()==0) {
        yValuesPresentationLeft = ch->data()->yValuesPresentation();
    }
    if (!plotOnLeft && m->rightCurvesCount()==0) {
        yValuesPresentationRight = ch->data()->yValuesPresentation();
    }

    if (plotOnLeft) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
    else ch->data()->setYValuesPresentation(yValuesPresentationRight);

    ax = plotOnLeft ? Enums::AxisType::atLeft : Enums::AxisType::atRight;

    axisWidget(QwtAxis::YRight)->setColorBarEnabled(false);

    setAxis(ax, ch->yName());
    prepareAxis(ax);




    Curve *g = createCurve(ch->legendName(), ch);
    QColor nextColor = getNextColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    ch->setPlotted(true);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;
    g->setYAxis(ax);

    ZoomStack::ScaleBounds *ybounds = 0;
    if (zoom->verticalScaleBounds->axis == ax) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
    if (ybounds) ybounds->add(g->yMin(), g->yMax());

    setInfoVisible(false);

    g->attach(this);

    update();
    updatePlottedIndexes();
    emit channelPlotted(ch);
    emit curvesCountChanged(); //->MainWindow.updateActions
}

QString Plot::axisTitleText(Enums::AxisType id) const
{DDD;
    return axisTitle(toQwtAxisType(id)).text();
}

void Plot::switchLabelsVisibility()
{DDD;
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DDD;
    m->updateTitles();
    updateLegend();
}

void Plot::savePlot() /*SLOT*/
{DDD;
    ImageRenderDialog dialog(true, this);
    if (dialog.exec()) {
        importPlot(dialog.getPath(), dialog.getSize(), dialog.getResolution());
        Settings::setSetting("lastPicture", dialog.getPath());
    }
}

void Plot::copyToClipboard() /*SLOT*/
{DDD;

    QTemporaryFile file(QDir::tempPath()+"/DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        ImageRenderDialog dialog(false, this);

        if (dialog.exec()) {
            importPlot(fileName, dialog.getSize(), dialog.getResolution());
            QImage img;
            if (img.load(fileName))
                qApp->clipboard()->setImage(img);
            else
                QMessageBox::critical(this, "Копирование рисунка", "Не удалось скопировать рисунок");
//                qDebug()<<"Could not load image from"<<fileName;
        }
    }
    else QMessageBox::critical(this, "Копирование рисунка", "Не удалось создать временный файл");
}

void Plot::print() /*SLOT*/
{DDD;
    QPrinter printer;
    if (printer.isValid())
        importPlot(printer, ImageRenderDialog::defaultSize(), ImageRenderDialog::defaultResolution());
    else
        QMessageBox::warning(this, "Deepsea Base", "Не удалось найти подходящий принтер");
}

void Plot::importPlot(const QString &fileName, const QSize &size, int resolution) /*private*/
{DDD;
    setAutoReplot(false);
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtAxis::YLeft);
    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

//    for (Curve *curve: curves) {
//        QPen pen = curve->pen();
//        if (pen.width()<2) pen.setWidth(2);
//        pen.setColor(pen.color().lighter(120));
//        curve->setPen(pen);
//    }

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);


    QString format = fileName.section(".", -1,-1);
    renderer.renderDocument(this, fileName, format, size, resolution);


    axisfont.setPointSize(axisfont.pointSize()+1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

//    for (Curve *curve: curves) {
//        curve->setPen(curve->oldPen);
//    }

    createLegend();
    setAutoReplot(true);
}

void Plot::importPlot(QPrinter &printer, const QSize &size, int resolution) /*private*/
{DDD;
    Q_UNUSED(size);
    Q_UNUSED(resolution);

    setAutoReplot(false);
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec()) {
        qreal left,right,top,bottom;
        printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        printer.setPageMargins(15, 15, 15, bottom, QPrinter::Millimeter);

        //настройка отображения графиков
        QFont axisfont = axisFont(QwtAxis::YLeft);
        axisfont.setPointSize(axisfont.pointSize()-2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (axisEnabled(i)) setAxisFont(i, axisfont);

        //настройка линий сетки
        grid->adaptToPrinter();

        insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
        renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);
        renderer.renderTo(this, printer);

        //восстанавливаем параметры графиков
        axisfont.setPointSize(axisfont.pointSize()+2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (axisEnabled(i)) setAxisFont(i, axisfont);
        grid->restore();

        createLegend();
    }

    setAutoReplot(true);
}

void Plot::switchInteractionMode()
{DDD;
    if (interactionMode == ScalingInteraction) {
        setInteractionMode(DataInteraction);
    }
    else {
        setInteractionMode(ScalingInteraction);
    }
}

void Plot::switchTrackingCursor()
{DDD;
//    if (trackingPanel) trackingPanel->switchVisibility();
    if (cursorBox) cursorBox->setVisible(!cursorBox->isVisible());
}

void Plot::toggleAutoscale(int axis, bool toggled)
{DDD;
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

void Plot::autoscale(Enums::AxisType axis)
{DDD;
    zoom->autoscale(axis, type()==Plot::PlotType::Spectrogram);
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DDD;
    interactionMode = mode;
    if (_canvas) _canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::switchCursor()
{DDD;
    if (!picker) return;

    bool pickerEnabled = picker->isEnabled();
    picker->setEnabled(!pickerEnabled);
    if (tracker) tracker->setEnabled(!pickerEnabled);
    Settings::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(QwtPlotItem *item)
{DDD;
    if (Curve *c = dynamic_cast<Curve *>(item)) {
        CurvePropertiesDialog dialog(c, this);
//        if (trackingPanel) connect(&dialog,SIGNAL(curveChanged(Curve*)), trackingPanel, SLOT(update()));
        if (cursorBox) connect(&dialog,SIGNAL(curveChanged(Curve*)), cursorBox, SLOT(updateLayout()));
        dialog.exec();
    }
}

void Plot::onDropEvent(bool plotOnLeft, const QVector<Channel*> &channels)
{DDD;
    //посылаем сигнал о том, что нужно построить эти каналы. Список каналов попадает
    //в mainWindow и возможно будет расширен за счет нажатого Ctrl
    //далее эти каналы попадут обратно в plot.
    emit needPlotChannels(plotOnLeft, channels);
}

void Plot::dropEvent(QDropEvent *event)
{DDD;
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        int w = 0;
        if (auto axis = axisWidget(QwtAxis::YLeft); axis->isVisible())
            w = axis->width();
        bool plotOnLeft = event->pos().x() <= w + canvas()->rect().x()+canvas()->rect().width()/2;
        onDropEvent(plotOnLeft, myData->channels);
        event->acceptProposedAction();
    }
    leftOverlay->setVisibility(false);
    rightOverlay->setVisibility(false);
}

void Plot::dragEnterEvent(QDragEnterEvent *event)
{DDD;
    emit focusThisPlot();
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить все каналы на левой или правой оси
        bool canOnLeft = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return canBePlottedOnLeftAxis(c);});
        bool canOnRight = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return canBePlottedOnRightAxis(c);});
        if (canOnLeft || canOnRight)
            event->acceptProposedAction();

    }
}

void Plot::dragMoveEvent(QDragMoveEvent *event)
{DDD;
    const ChannelsMimeData *myData = dynamic_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить каналы на левой или правой оси
        //определяем, в какой области мы находимся
        int w = 0;
        if (auto axis = axisWidget(QwtAxis::YLeft); axis->isVisible())
            w = axis->width();
        bool plotOnLeft = event->pos().x() <= w + canvas()->rect().x()+canvas()->rect().width()/2;
//        bool can = true;
//        for (auto c: myData->channels) {
//            if ((plotOnLeft && !canBePlottedOnLeftAxis(c)) || (!plotOnLeft && !canBePlottedOnRightAxis(c))) {
//                can = false;
//                break;
//            }
//        }
        leftOverlay->setVisibility(plotOnLeft);
        rightOverlay->setVisibility(!plotOnLeft);
//        if (can)
            event->acceptProposedAction();
    }
}

void Plot::dragLeaveEvent(QDragLeaveEvent *event)
{DDD;
    Q_UNUSED(event);
    leftOverlay->setVisibility(false);
    rightOverlay->setVisibility(false);
}

Enums::AxisType toAxisType(QwtAxisId id) {
    switch (id) {
        case QwtAxis::XTop : return Enums::AxisType::atTop;
        case QwtAxis::XBottom : return Enums::AxisType::atBottom;
        case QwtAxis::YLeft : return Enums::AxisType::atLeft;
        case QwtAxis::YRight : return Enums::AxisType::atRight;
    }
    return Enums::AxisType::atInvalid;
}

QwtAxisId toQwtAxisType(Enums::AxisType type) {
    switch (type) {
        case Enums::AxisType::atTop: return QwtAxis::XTop;
        case Enums::AxisType::atBottom: return QwtAxis::XBottom;
        case Enums::AxisType::atLeft: return QwtAxis::YLeft;
        case Enums::AxisType::atRight: return QwtAxis::YRight;
        case Enums::AxisType::atInvalid: break;
    }
    return -1;
}
