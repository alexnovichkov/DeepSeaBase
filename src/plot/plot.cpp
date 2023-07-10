#include "plot.h"

#include "fileformats/filedescriptor.h"
#include "curve.h"

#include "zoomstack.h"
#include "colormapfactory.h"
#include "plottedmodel.h"

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
#include "logging.h"

#include "playpanel.h"
#include "channelsmimedata.h"
#include "imagerenderdialog.h"
#include "enums.h"

#include "unitsconverter.h"
#include "plotmodel.h"
#include "cursors.h"
#include "cursorbox.h"
#include "picker.h"
#include "qcpplot.h"
#include "canvaseventfilter.h"

Plot::Plot(Enums::PlotType type, QWidget *parent) :
    plotType(type)
{DD;
    m = new PlotModel(this);

    picker = new Picker(this);
    picker->setPickPriority(Picker::PickPriority::PickLast);
    picker->setEnabled(true);
    zoom = new ZoomStack(this);
    connect(zoom, &ZoomStack::replotNeeded, this, &Plot::replot);
    m_plot = new QCPPlot(this, parent);

    colors = new ColorSelector();

    axisLabelsVisible = se->getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;

    m_plot->createLegend();

    cursors = new Cursors(this);
    cursorBox = new CursorBox(cursors, this);
    cursorBox->setWindowTitle(parent->windowTitle());
    cursorBox->setVisible(false);

    if (type == Enums::PlotType::Octave) {
        m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::ThirdOctave);
    }

    connect(picker, &Picker::removeNeeded, this, &Plot::removeCursor);
    connect(this, &Plot::curvesCountChanged, cursors, &Cursors::update);
    connect(cursors, &Cursors::cursorPositionChanged, m_plot, qOverload<>(&QCPPlot::updateSecondaryPlots));
    connect(this, &Plot::curvesCountChanged, cursorBox, &CursorBox::updateLayout);
    connect(cursorBox,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));

    if (type == Enums::PlotType::Time) {
        playerPanel = new PlayPanel(this);
        connect(this, SIGNAL(curvesCountChanged()), playerPanel, SLOT(update()));
        connect(m_plot, SIGNAL(canvasDoubleClicked(QPoint)), playerPanel, SLOT(moveTo(QPoint)));
    }

    connect(se, &Settings::settingChanged, [this](const QString &key, const QVariant &val){
        Q_UNUSED(val);
        if (key == "plotOctaveAsHistogram") update();
    });
}

Plot::~Plot()
{DD;
    deleteAllCurves(true);

//    delete trackingPanel;
    delete cursorBox;
    delete picker;

    se->setSetting("axisLabelsVisible", axisLabelsVisible);
    se->setSetting("autoscale-x", !zoom->scaleBounds(Enums::AxisType::atBottom)->isFixed());
    se->setSetting("autoscale-y", !zoom->scaleBounds(Enums::AxisType::atLeft)->isFixed());
    se->setSetting("autoscale-y-slave", !zoom->scaleBounds(Enums::AxisType::atRight)->isFixed());
    delete zoom;
    delete colors;
}

QWidget *Plot::widget() const
{
    return dynamic_cast<QWidget*>(m_plot);
}

QCPPlot *Plot::impl() const
{
    return m_plot;
}

double Plot::screenToPlotCoordinates(Enums::AxisType axis, double value) const
{
    return m_plot->screenToPlotCoordinates(axis, value);
}

double Plot::plotToScreenCoordinates(Enums::AxisType axis, double value) const
{
    return m_plot->plotToScreenCoordinates(axis, value);
}

Range Plot::plotRange(Enums::AxisType axis) const
{
    return m_plot->plotRange(axis);
}

Range Plot::screenRange(Enums::AxisType axis) const
{
    return m_plot->screenRange(axis);
}

void Plot::replot()
{
    if (m_plot) m_plot->replot();
}

QWidget *Plot::toolBarWidget()
{
    return playerPanel;
}

void Plot::updateActions(int filesCount, int channelsCount)
{
    Q_UNUSED(filesCount);
    Q_UNUSED(channelsCount);
    if (playerPanel)
        playerPanel->setEnabled(!m->isEmpty());
}

void Plot::updatePlottedIndexes()
{DD;
    if (!sergeiMode) m->updatePlottedIndexes();
    emit curvesCountChanged(); //->MainWindow.updateActions
}

void Plot::plotCurvesForDescriptor(FileDescriptor *d, int fileIndex)
{DD;
    if (sergeiMode) {
        deleteAllCurves(false);
        m->updatePlottedIndexes(d, fileIndex);
        const auto plotted = m->plottedIndexes();
        for (const auto &i: plotted) plotChannel(i.ch, i.onLeft, i.fileIndex);
    }
}

void Plot::update()
{DD;
    for (auto c: m->curves()) {
        c->updatePen();
    }
    updateLabels();
    updateAxesLabels();
    if (m_plot) m_plot->updateLegend();

    updateBounds();

    if (m_plot) m_plot->replot();
}

void Plot::updateBounds()
{DD;
    if (m->leftCurvesCount()==0)
        zoom->scaleBounds(Enums::AxisType::atLeft)->reset();
    if (m->rightCurvesCount()==0)
        zoom->scaleBounds(Enums::AxisType::atRight)->reset();
    if (m->isEmpty())
        zoom->scaleBounds(Enums::AxisType::atBottom)->reset();

    if (!zoom->scaleBounds(Enums::AxisType::atBottom)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atBottom)->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atLeft)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atLeft)->autoscale();
    if (m->rightCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atRight)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atRight)->autoscale();
}

QMenu *Plot::createMenu(Enums::AxisType axis, const QPoint &pos)
{DD;
    QMenu *menu = new QMenu(m_plot);

    if (axis == Enums::AxisType::atBottom) {
        auto scm = new QMenu("Одинарный курсор", menu);
        scm->addAction("Вертикальный", [=](){
            auto cursor = cursors->addSingleCursor(m_plot->localCursorPosition(pos), Cursor::Style::Vertical);
            m_plot->addCursorToSecondaryPlots(cursor);
        });
        scm->addAction("Перекрестье", [=]() {
            auto cursor = cursors->addSingleCursor(m_plot->localCursorPosition(pos), Cursor::Style::Cross);
            m_plot->addCursorToSecondaryPlots(cursor);
        });
        menu->addMenu(scm);

        auto dcm = new QMenu("Двойной курсор", menu);
        dcm->addAction("Стандартный", [=](){
            auto cursor = cursors->addDoubleCursor(m_plot->localCursorPosition(pos), Cursor::Style::Vertical);
            m_plot->addCursorToSecondaryPlots(cursor);
        });
        dcm->addAction("Режекция", [=](){
            auto cursor = cursors->addRejectCursor(m_plot->localCursorPosition(pos), Cursor::Style::Vertical);
            m_plot->addCursorToSecondaryPlots(cursor);
        });
        menu->addMenu(dcm);

        menu->addAction("Гармонический курсор", [=](){
            auto cursor = cursors->addHarmonicCursor(m_plot->localCursorPosition(pos));
            m_plot->addCursorToSecondaryPlots(cursor);
        });

        auto axisScaleAct = new QAction("Шкала", menu);
        auto axisScaleMenu = new QMenu(menu);
        axisScaleAct->setMenu(axisScaleMenu);
        auto scaleGroup = new QActionGroup(axisScaleMenu);

        auto linear = new QAction("Линейная", scaleGroup);
        linear->setCheckable(true);
        linear->setData(static_cast<int>(Enums::AxisScale::Linear));
        axisScaleMenu->addAction(linear);

        auto logar = new QAction("Логарифмическая", scaleGroup);
        logar->setCheckable(true);
        logar->setData(static_cast<int>(Enums::AxisScale::Logarithmic));
        axisScaleMenu->addAction(logar);

        auto oct = new QAction("Третьоктава", scaleGroup);
        oct->setCheckable(true);
        oct->setData(static_cast<int>(Enums::AxisScale::ThirdOctave));
        axisScaleMenu->addAction(oct);

        switch (m_plot->axisScale(axis)) {
            case Enums::AxisScale::Linear: linear->setChecked(true); break;
            case Enums::AxisScale::Logarithmic: logar->setChecked(true); break;
            case Enums::AxisScale::ThirdOctave: oct->setChecked(true); break;
        }

        connect(scaleGroup, &QActionGroup::triggered, [=](QAction *act){
            Enums::AxisScale scale = static_cast<Enums::AxisScale>(act->data().toInt());
            m_plot->setAxisScale(Enums::AxisType::atBottom, scale);
        });

        menu->addAction(axisScaleAct);
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
        QMenu *am = new QMenu(0);
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
            this->recalculateScale(axis);
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
            coords.insert(Enums::AxisType::atLeft, {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            range = plotRange(Enums::AxisType::atRight);
            s1 = range.min;
            s2 = range.max;

            range = screenRange(Enums::AxisType::atRight);

            p1 = range.min;
            p2 = range.max;
            delta = screenToPlotCoordinates(Enums::AxisType::atRight, (p1+p2)/2.0);

            coords.insert(Enums::AxisType::atRight, {s1-delta, s2-delta});

            range = plotRange(Enums::AxisType::atBottom);
            coords.insert(Enums::AxisType::atBottom, {range.min, range.max});

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
            coords.insert(Enums::AxisType::atLeft, {s, ss});
            coords.insert(Enums::AxisType::atRight, {s, ss});

            range = plotRange(Enums::AxisType::atBottom);
            coords.insert(Enums::AxisType::atBottom, {range.min, range.max});

            zoom->addZoom(coords, true);
        });
    }
    return menu;
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

    const bool sm = sergeiMode;
    sergeiMode = true;
    deleteAllCurves();
    for (const auto &c: m->plottedIndexes()) {
        plotChannel(c.ch, c.onLeft, c.fileIndex);
    }
    sergeiMode = sm;
}

int Plot::curvesCount(int type) const
{DD;
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

    updatePlottedIndexes();
}

void Plot::deleteCurvesForDescriptor(FileDescriptor *descriptor)
{DD;
    for (int i = m->size()-1; i>=0; --i) {
        if (Curve *curve = m->curve(i); descriptor == curve->channel->descriptor()) {
            deleteCurve(curve, true);
        }
    }
    updatePlottedIndexes();
}

//не удаляем, если фиксирована
void Plot::deleteCurveFromLegend(Curve *curve)
{DD;
    if (!curve->fixed) {
        deleteCurve(curve, true);
        updatePlottedIndexes();
    }
}

void Plot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *curve = m->plotted(dfd->channel(channel))) {
        deleteCurve(curve, doReplot);
        updatePlottedIndexes();
    }
}

void Plot::deleteSelectedCurve(Selectable *selected)
{DD;
    if (Curve *curve = dynamic_cast<Curve*>(selected)) {
        deleteCurve(curve, true);
        updatePlottedIndexes();
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void Plot::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (!curve) return;
    if (curve->selected()) picker->deselect();

    bool removedFromLeft = true;
    if (m->deleteCurve(curve, &removedFromLeft)) {
        PlottedModel::instance().remove(curve);
        emit curveDeleted(curve->channel); //->MainWindow.onChannelChanged
        colors->freeColor(curve->pen().color());

        if (removedFromLeft > 0) {
            zoom->scaleBounds(Enums::AxisType::atLeft)->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        else {
            zoom->scaleBounds(Enums::AxisType::atRight)->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        zoom->scaleBounds(Enums::AxisType::atBottom)->removeToAutoscale(curve->xMin(), curve->xMax());

        curve->detachFrom(this);
        delete curve;
        m_plot->setCurrentCurve(nullptr);

        if (m->leftCurvesCount()==0) {
            yLeftName.clear();
        }
        if (m->rightCurvesCount()==0) {
            yRightName.clear();
            if (m_plot) m_plot->enableAxis(Enums::AxisType::atRight, false);
        }
        if (m->isEmpty()) xName.clear();
        if (m_plot) m_plot->setInfoVisible(m->size()==0);
        if (doReplot) update();
    }
}

void Plot::showContextMenu(const QPoint &pos, Enums::AxisType axis)
{DD;
    if (m->isEmpty()) return;

    QMenu *menu = createMenu(axis, pos);

    if (!menu->actions().isEmpty())
        menu->exec(pos);
    menu->deleteLater();
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{DD;
    if (m->isEmpty()) return true;

    //не можем строить временные графики на графике спектров и наоборот
    if (ch->type() == Descriptor::TimeResponse && type() != Enums::PlotType::Time) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }
    if (ch->type() != Descriptor::TimeResponse && type() == Enums::PlotType::Time) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0) return true;
        if (yLeftName.isEmpty()) return true;
        if (PhysicalUnits::Units::unitsAreSame(ch->yName(), yLeftName)) return true;
        if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{DD;
    if (m->isEmpty()) return true;

    //не можем строить временные графики на графике спектров и наоборот
    if (ch->type() == Descriptor::TimeResponse && type() != Enums::PlotType::Time) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }
    if (ch->type() != Descriptor::TimeResponse && type() == Enums::PlotType::Time) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->rightCurvesCount()==0) return true;
        if (yRightName.isEmpty()) return true;
        if (PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName)) return true;
        if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

void Plot::focusPlot()
{
    emit focusThisPlot();
}

void Plot::addSelectable(Selectable *item)
{
    if (!selectables.contains(item))
        selectables.append(item);
}

void Plot::removeSelectable(Selectable *item)
{
    selectables.removeOne(item);
}

void Plot::deselect()
{
    for (auto candidate: selectables) {
        candidate->setSelected(false, SelectedPoint());
    }
}

Selected Plot::findSelected(QPoint pos) const
{
    //Ищем элемент под курсором мыши
    Selected selected {nullptr, SelectedPoint()};

    //сначала ищем метки, курсоры и т.д., то есть не кривые
    {
        double minDist = qInf();
        for (auto candidate: selectables) {
            if (m_plot->isCurve(candidate)) continue;
            double distx = 0.0;
            double disty = 0.0;
            SelectedPoint point;
            if (candidate->underMouse(pos, &distx, &disty, &point)) {
                double dist = 0.0;
                if (distx == qInf()) dist = disty;
                else if (disty == qInf()) dist = distx;
                else dist = sqrt(distx*distx+disty*disty);
                if (!selected.object || dist < minDist) {
                    selected.object = candidate;
                    selected.point = point;
                    minDist = dist;
                }
            }
        }
    }
    if (!selected.object) {
        double minDist = qInf();
        for (auto candidate: selectables) {
            if (!m_plot->isCurve(candidate)) continue;

            double distx = 0.0;
            double disty = 0.0;
            SelectedPoint point;
            if (candidate->underMouse(pos, &distx, &disty, &point)) {
                double dist = 0.0;
                if (distx == qInf()) dist = disty;
                else if (disty == qInf()) dist = distx;
                else dist = sqrt(distx*distx+disty*disty);
                if (!selected.object || dist < minDist) {
                    selected.object = candidate;
                    selected.point = point;
                    minDist = dist;
                }
            }
        }
    }
    return selected;
}

QString Plot::pointCoordinates(const QPointF &pos)
{DD;
    return smartDouble(pos.x(), m_plot->tickDistance(Enums::AxisType::atBottom))+", "+
            smartDouble(pos.y(), m_plot->tickDistance(Enums::AxisType::atLeft));
}

double Plot::tickDistance(Enums::AxisType axisType) const
{
    if (m_plot) return m_plot->tickDistance(axisType);
    return 0.0;
}

void Plot::setAxis(Enums::AxisType axis, const QString &name)
{DD;
    switch (axis) {
        case Enums::AxisType::atLeft: yLeftName = name; break;
        case Enums::AxisType::atRight: yRightName = name; break;
        case Enums::AxisType::atBottom: xName = name; break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{DD;
    if (!m_plot) return;

    if (m->leftCurvesCount()==0) m_plot->enableAxis(Enums::AxisType::atLeft, false);
    else {
        m_plot->enableAxis(Enums::AxisType::atLeft, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
//        QString text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix));
        QString text(QString("%1 %2").arg(yLeftName).arg(suffix));
        if (axisLabelsVisible)
            m_plot->setAxisTitle(Enums::AxisType::atLeft, text);
        else
            m_plot->setAxisTitle(Enums::AxisType::atLeft, "");
    }

    if (m->rightCurvesCount()==0) m_plot->enableAxis(Enums::AxisType::atRight, false);
    else {
        m_plot->enableAxis(Enums::AxisType::atRight, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
//        QString text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix));
        QString text(QString("%1 %2").arg(yRightName).arg(suffix));
        if (axisLabelsVisible)
            m_plot->setAxisTitle(Enums::AxisType::atRight, text);
        else
            m_plot->setAxisTitle(Enums::AxisType::atRight, "");
    }

    if (m_plot->axisEnabled(Enums::AxisType::atBottom)) {
        m_plot->setAxisTitle(Enums::AxisType::atBottom, axisLabelsVisible ? xName : "");
    }
    m_plot->replot();
}

void Plot::setScale(Enums::AxisType id, double min, double max, double step)
{DD;
    if (m_plot) m_plot->setAxisRange(id, min, max, step);
}

void Plot::removeLabels()
{DD;
    m->removeLabels();
    if (m_plot) m_plot->replot();
}

void Plot::moveCurve(Curve *curve, Enums::AxisType axis)
{DD;
    if (type()==Enums::PlotType::Spectrogram) return;

    if ((axis == Enums::AxisType::atLeft && canBePlottedOnLeftAxis(curve->channel))
        || (axis == Enums::AxisType::atRight && canBePlottedOnRightAxis(curve->channel))) {
        if (m_plot) m_plot->enableAxis(axis, true);
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
    else QMessageBox::warning(widget(), "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");
}

QColor Plot::getNextColor()
{DD;
    return colors->getColor();
}

QString Plot::yValuesPresentationSuffix(int yValuesPresentation) const
{DD;
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

void Plot::saveSpectrum(double zVal)
{DD;
    QVector<double> vals;
    if (qIsNaN(zVal)) {
        //сохранение для всех курсоров
        for (auto c: cursors->cursors()) {
            if (c->type() != Cursor::Type::Single) continue;

            vals << c->currentPosition().y();
        }
    }
    else {
        vals << zVal;
    }
    emit saveHorizontalSlice(vals);
}

void Plot::saveThroughput(double xVal)
{DD;
    QVector<double> vals;
    if (qIsNaN(xVal)) {
        //сохранение для всех курсоров
        for (auto c: cursors->cursors()) {
            if (c->type() != Cursor::Type::Single) continue;

            vals << c->currentPosition().x();
        }
    }
    else {
        vals << xVal;
    }
    emit saveVerticalSlice(vals);
}

void Plot::updateLabels()
{
    for (auto curve: m->curves()) {
        curve->updateLabels();
    }
}

void Plot::removeCursor(Selectable *selected)
{
    if (auto cursor = cursors->cursorFor(selected)) {
        m_plot->removeCursorFromSecondaryPlots(cursor);
        cursors->removeCursor(cursor);
    }
}

void Plot::recalculateScale(Enums::AxisType axis)
{DD;
    auto bounds = zoom->scaleBounds(axis);
    if (!bounds) return;

    bounds->reset();

    const bool left = axis == Enums::AxisType::atColor || axis == Enums::AxisType::atLeft;
    const auto count = left ? m->leftCurvesCount() : m->rightCurvesCount();

    for (int i=0; i<count; ++i) {
        auto curve = m->curve(i, left);
        bounds->add(curve->yMin(), curve->yMax());
    }
}

void Plot::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{DD;
    if (!ch || !m_plot) return;
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    QString message;
    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch, &message)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch, &message))) {
        QMessageBox::warning(widget(), QString("Не могу построить канал"),
                             QString("%1.\nСначала очистите график.").arg(message));
        return;
    }
    m_plot->setInfoVisible(false);

    if (!ch->populated()) {
        ch->populate();
    }

    auto axX = Enums::AxisType::atBottom;

    setAxis(axX, ch->xName());
    m_plot->enableAxis(axX, true);

    auto axY = Enums::AxisType::atLeft;
    // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
    if (plotOnLeft && m->leftCurvesCount()==0 && !sergeiMode) {
        yValuesPresentationLeft = ch->data()->yValuesPresentation();
    }
    if (!plotOnLeft && m->rightCurvesCount()==0 && !sergeiMode) {
        yValuesPresentationRight = ch->data()->yValuesPresentation();
    }

    if (plotOnLeft) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
    else ch->data()->setYValuesPresentation(yValuesPresentationRight);

    axY = plotOnLeft ? Enums::AxisType::atLeft : Enums::AxisType::atRight;

    m_plot->enableColorBar(Enums::AxisType::atRight, false);

    setAxis(axY, ch->yName());
    m_plot->enableAxis(axY, true);




    Curve *g = m_plot->createCurve(ch, axX, axY);
    QColor nextColor = getNextColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    PlottedModel::instance().add(ch,g);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;
//    g->setYAxis(axY);

    if (auto bounds = zoom->scaleBounds(axY))
        bounds->add(g->yMin(), g->yMax());
    zoom->scaleBounds(Enums::AxisType::atBottom)->add(g->xMin(), g->xMax());

    g->setLegend(m_plot->checkableLegend);
    g->attachTo(this);

    update();
    updatePlottedIndexes();
    emit channelPlotted(ch);
}

QString Plot::axisTitleText(Enums::AxisType id) const
{DD;
    return m_plot->axisTitle(id);
}

void Plot::switchLabelsVisibility()
{DD;
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DD;
    m->updateTitles();
    if (m_plot) m_plot->updateLegend();
}

void Plot::savePlot() /*SLOT*/
{DD;
    ImageRenderDialog dialog(true, type()==Enums::PlotType::Spectrogram, nullptr);
    if (dialog.exec()) {
        if (m_plot) m_plot->importPlot(dialog.getPath(), dialog.getSize(), dialog.getResolution(), dialog.graphOnly());
    }
}

void Plot::copyToClipboard(bool useDialog) /*SLOT*/
{DD;

    QTemporaryFile file(QDir::tempPath()+"/DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        QSize size = ImageRenderDialog::defaultSize();
        int resolution = ImageRenderDialog::defaultResolution();
        bool graphOnly = false;

        if (useDialog) {
            ImageRenderDialog dialog(false, type()==Enums::PlotType::Spectrogram, 0);

            if (dialog.exec())  {
                size = dialog.getSize();
                resolution = dialog.getResolution();
                graphOnly = dialog.graphOnly();
            }
        }
        if (m_plot) m_plot->importPlot(fileName, size, resolution, graphOnly);
        QImage img;
        if (img.load(fileName))
            qApp->clipboard()->setImage(img);
        else {
            QMessageBox::critical(0, "Копирование рисунка", "Не удалось скопировать рисунок");
            LOG(ERROR)<<"Could not load image from"<<fileName;
        }

    }
    else QMessageBox::critical(0, "Копирование рисунка", "Не удалось создать временный файл");
}

void Plot::print() /*SLOT*/
{DD;
    QPrinter printer;
    if (printer.isValid())
        m_plot->importPlot(printer, ImageRenderDialog::defaultSize(), ImageRenderDialog::defaultResolution(), true);
    else
        QMessageBox::warning(widget(), "Deepsea Base", "Не удалось найти подходящий принтер");
}

void Plot::switchInteractionMode()
{DD;
    if (interactionMode == Enums::InteractionMode::ScalingInteraction) {
        interactionMode = Enums::InteractionMode::DataInteraction;
    }
    else {
        interactionMode = Enums::InteractionMode::ScalingInteraction;
    }
}

void Plot::switchCursorBox()
{DD;
    if (cursorBox) cursorBox->setVisible(!cursorBox->isVisible());
}

void Plot::toggleAutoscale(Enums::AxisType axis, bool toggled)
{DD;
    if (auto b = zoom->scaleBounds(axis)) b->setFixed(!toggled);

    replot();
}

void Plot::autoscale(Enums::AxisType axis)
{DD;
    zoom->autoscale(axis);
}

void Plot::editLegendItem(Curve *curve)
{DD;
    if (curve->type == Curve::Type::Spectrogram) return; //у спектрограммы нет свойств кривой

    CurvePropertiesDialog dialog(curve, this);
    if (cursorBox) connect(&dialog, SIGNAL(curveChanged(Curve*)), cursorBox, SLOT(updateLayout()));
    dialog.exec();
}

void Plot::onDropEvent(bool plotOnLeft, const QVector<Channel*> &channels)
{DD;
    //посылаем сигнал о том, что нужно построить эти каналы. Список каналов попадает
    //в mainWindow и возможно будет расширен за счет нажатого Ctrl
    //далее эти каналы попадут обратно в plot.
    emit needPlotChannels(plotOnLeft, channels);
}
