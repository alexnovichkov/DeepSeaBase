#include "plot.h"

#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_scale_widget.h>
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
#include "app.h"
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

// простой фабричный метод создания кривой нужного типа
Curve * createCurve(const QString &legendName, Channel *channel)
{
    if (channel->data()->blocksCount() > 1)
        return new SpectroCurve(legendName, channel);

    // считаем, что шаг по оси х 0 только у октав и третьоктав
    if (channel->data()->xValuesFormat() == DataHolder::XValuesNonUniform)
        return new BarCurve(legendName, channel);

    return new LineCurve(legendName, channel);
}

#include <qwt_painter.h>
class ScaleDraw : public QwtScaleDraw
{
public:
    ScaleDraw()
    {

    }
    int hover = 0; //0=none, 1=first, 2=second

protected:
    virtual void drawBackbone(QPainter *painter) const override
    {
        const int pw = qMax(qRound(penWidthF()),1);

        const qreal len = length();
        const QPointF _pos = pos();

        switch (alignment()) {
            case QwtScaleDraw::LeftScale:
            {
                const qreal x = qRound( _pos.x() - ( pw - 1 ) / 2 );
                QwtPainter::drawLine( painter, x, _pos.y(), x, _pos.y() + len );
                if (hover==1) {
                    QwtPainter::drawLine( painter, x-1, _pos.y(), x-1, _pos.y() + len/2.0 );
                }
                else if (hover==2) {
                    QwtPainter::drawLine( painter, x-1, _pos.y() + len/2.0, x-1, _pos.y() + len );
                }

                break;
            }
            case QwtScaleDraw::RightScale:
            {
                const qreal x = qRound( _pos.x() + pw / 2 );
                QwtPainter::drawLine( painter, x, _pos.y(), x, _pos.y() + len );
                if (hover==1) {
                    QwtPainter::drawLine( painter, x+1, _pos.y(), x+1, _pos.y() + len/2.0 );
                }
                else if (hover==2) {
                    QwtPainter::drawLine( painter, x+1, _pos.y() + len/2.0, x+1, _pos.y() + len );
                }
                break;
            }
            case QwtScaleDraw::TopScale:
            {
                const qreal y = qRound( _pos.y() - ( pw - 1 ) / 2 );
                QwtPainter::drawLine( painter, _pos.x(), y, _pos.x() + len, y );
                if (hover==1) {
                    QwtPainter::drawLine( painter, _pos.x(), y-1, _pos.x() + len/2, y-1 );
                }
                else if (hover==2) {
                    QwtPainter::drawLine( painter, _pos.x()+len/2, y-1, _pos.x() + len, y-1 );
                }
                break;
            }
            case QwtScaleDraw::BottomScale:
            {
                const qreal y = qRound( _pos.y() + pw / 2 );
                QwtPainter::drawLine( painter, _pos.x(), y, _pos.x() + len, y );
                if (hover==1) {
                    QwtPainter::drawLine( painter, _pos.x(), y+1, _pos.x() + len/2, y+1 );
                }
                else if (hover==2) {
                    QwtPainter::drawLine( painter, _pos.x()+len/2, y+1, _pos.x() + len, y+1 );
                }
                break;
            }
        }
    }
};


Plot::Plot(QWidget *parent) :
    QwtPlot(parent), zoom(0)
{DD;
    _canvas = new QwtPlotCanvas();
    _canvas->setFocusIndicator(QwtPlotCanvas::CanvasFocusIndicator);
    _canvas->setPalette(Qt::white);
    _canvas->setFrameStyle(QFrame::StyledPanel);
    //_canvas->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    setCanvas(_canvas);

    setAutoReplot(true);
    setAcceptDrops(true);

    setAxisScaleDraw(xBottomAxis, new ScaleDraw());
    setAxisScaleDraw(yLeftAxis, new ScaleDraw());
    setAxisScaleDraw(yRightAxis, new ScaleDraw());

    axisWidget(xBottomAxis)->setMouseTracking(true);
    axisWidget(yLeftAxis)->setMouseTracking(true);
    axisWidget(yRightAxis)->setMouseTracking(true);

    trackingPanel = new TrackingPanel(this);
    trackingPanel->setVisible(false);
    connect(trackingPanel,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), trackingPanel, SLOT(update()));


    playerPanel = new PlayPanel(this);
    playerPanel->setVisible(false);
    connect(playerPanel,SIGNAL(closeRequested()),SIGNAL(playerPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), playerPanel, SLOT(update()));

    axisLabelsVisible = App->getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;


    // grid
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::darkGray, 0, Qt::DotLine);
    grid->attach(this);

    createLegend();


    zoom = new ZoomStack(this);

    tracker = new PlotTracker(this);
    tracker->setEnabled(App->getSetting("pickerEnabled", true).toBool());

    _picker = new Picker(this);
    _picker->setEnabled(App->getSetting("pickerEnabled", true).toBool());
    connect(_picker,SIGNAL(cursorSelected(TrackingCursor*)), trackingPanel, SLOT(changeSelectedCursor(TrackingCursor*)));
    connect(_picker,SIGNAL(axisClicked(QPointF,bool)),       trackingPanel, SLOT(setValue(QPointF,bool)));
    connect(_picker,SIGNAL(cursorMovedTo(QPointF)),          trackingPanel, SLOT(setValue(QPointF)));
    connect(_picker,SIGNAL(cursorSelected(TrackingCursor*)), playerPanel, SLOT(updateSelectedCursor(TrackingCursor*)));
    connect(_picker,SIGNAL(axisClicked(QPointF,bool)),       playerPanel, SLOT(setValue(QPointF)));
    connect(_picker,SIGNAL(cursorMovedTo(QPointF)),          playerPanel, SLOT(setValue(QPointF)));

    dragZoom = new DragZoom(this);
    wheelZoom = new WheelZoom(this);
    plotZoom = new PlotZoom(this);

    axisZoom = new AxisZoom(this);
    connect(axisZoom,SIGNAL(axisClicked(QPointF,bool)),   trackingPanel, SLOT(setValue(QPointF,bool)));
    connect(axisZoom,SIGNAL(hover(QwtAxisId,int)), SLOT(hoverAxis(QwtAxisId,int)));

    canvasFilter = new CanvasEventFilter(this);
    canvasFilter->setZoom(zoom);
    canvasFilter->setDragZoom(dragZoom);
    canvasFilter->setWheelZoom(wheelZoom);
    canvasFilter->setAxisZoom(axisZoom);
    canvasFilter->setPlotZoom(plotZoom);
    canvasFilter->setPicker(_picker);
    connect(canvasFilter,SIGNAL(hover(QwtAxisId,int)), SLOT(hoverAxis(QwtAxisId,int)));
    connect(canvasFilter,SIGNAL(contextMenuRequested(QPoint,QwtAxisId)), SLOT(showContextMenu(QPoint,QwtAxisId)));
    connect(canvasFilter,SIGNAL(moveCursor(Enums::Direction)), trackingPanel, SLOT(moveCursor(Enums::Direction)));
}

Plot::~Plot()
{DD;

    delete trackingPanel;
    delete playerPanel;
    qDeleteAll(curves);
    delete grid;
    delete tracker;
    delete _picker;

    App->setSetting("axisLabelsVisible", axisLabelsVisible);
    App->setSetting("autoscale-x", !zoom->horizontalScaleBounds->isFixed());
    App->setSetting("autoscale-y", !zoom->verticalScaleBounds->isFixed());
    App->setSetting("autoscale-y-slave", !zoom->verticalScaleBoundsSlave->isFixed());
    delete zoom;
    delete dragZoom;
    delete wheelZoom;
    delete axisZoom;
    delete canvasFilter;
}

QVector<Channel *> Plot::plottedChannels() const
{
    QVector<Channel*> result;
    result.reserve(curves.size());
    for (auto c: curves) result << c->channel;
    return result;
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
    if (rightCurves.isEmpty() && !spectrogram) {
        zoom->verticalScaleBoundsSlave->reset();
        axisWidget(yRightAxis)->setColorBarEnabled(false);
    }
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

int Plot::curvesCount(int type) const
{DD;
    return std::count_if(curves.cbegin(), curves.cend(),
                         [type](Curve *c){return int(c->channel->type()) == type;});
}

void Plot::deleteAllCurves(bool forceDeleteFixed)
{DD;
    for (int i=curves.size()-1; i>=0; --i) {
        Curve *c = curves[i];
        if (!c->fixed || forceDeleteFixed) {
            deleteCurve(c, i==0);
        }
    }

    playerPanel->reset();

    emit curvesCountChanged();
}

void Plot::deleteCurvesForDescriptor(FileDescriptor *descriptor)
{DD;
    for (int i = curves.size()-1; i>=0; --i) {
        if (Curve *curve = curves[i]; descriptor == curve->channel->descriptor()) {
            deleteCurve(curve, true);
        }
    }
    emit curvesCountChanged(); //->MainWindow.updateActions
}

//не удаляем, если фиксирована
void Plot::deleteCurveFromLegend(QwtPlotItem *item)
{DD;
    if (Curve *c = dynamic_cast<Curve *>(item); !c->fixed) {
        deleteCurve(c, true);
        emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

void Plot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *curve = plotted(dfd->channel(channel))) {
        deleteCurve(curve, doReplot);
        if (doReplot) emit curvesCountChanged(); //->MainWindow.updateActions
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void Plot::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (curve) {
        emit curveDeleted(curve->channel); //->MainWindow.onCurveDeleted
        App->colors()->freeColor(curve->pen().color());

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

    if (axis.pos == QwtAxis::xBottom) {
        menu->addAction(xScaleIsLogarithmic?"Линейная шкала":"Логарифмическая шкала", [=](){
            if (xScaleIsLogarithmic)
                setAxisScaleEngine(xBottomAxis, new QwtLinearScaleEngine());
            else
                setAxisScaleEngine(xBottomAxis, new LogScaleEngine(2));

            xScaleIsLogarithmic = !xScaleIsLogarithmic;
        });
    }

    // определяем, все ли графики представляют временные данные
    const bool time = std::all_of(curves.cbegin(), curves.cend(), [](Curve *c){
          return c->channel->type() == Descriptor::TimeResponse;
    });

    if (time && axis.pos == QwtAxis::xBottom) {
        menu->addAction("Сохранить временной сегмент", [=](){
            double xStart = canvasMap(axis).s1();
            double xEnd = canvasMap(axis).s2();

            QList<FileDescriptor*> files;

            for (Curve *c: qAsConst(curves)) {
                if (!files.contains(c->channel->descriptor()))
                    files << c->channel->descriptor();
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
            for (Curve *c: list) {
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

            ZoomStack::zoomCoordinates coords;
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

            ZoomStack::zoomCoordinates coords;
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
    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (leftCurves.isEmpty() || yLeftName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yLeftName))
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

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (rightCurves.isEmpty() || yRightName.isEmpty() || PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName))
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
{DD;
    if (!curves.isEmpty() && id == yRightAxis) {
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(curves.first())) {
            c->setColorInterval(min, max);
            axisWidget(id)->setColorMap(QwtInterval(min, max), ColorMapFactory::map(colorMap));
        }
    }
    setAxisScale(id, min, max, step);
    replot();
}

void Plot::removeLabels()
{DD;
    for (Curve *c: qAsConst(curves))
        c->removeLabels();

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
        zoom->moveToAxis(axis, curve->channel->data()->yMin(), curve->channel->data()->yMax());
    }
    else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");
}

void Plot::fixCurve(QwtPlotItem *curve)
{DD;
    if (Curve *c = dynamic_cast<Curve*>(curve)) {
        c->switchFixed();
        updateLegend();
    }
}

void Plot::hoverAxis(QwtAxisId axis, int hover)
{DD;
    if (ScaleDraw * scale = dynamic_cast<ScaleDraw*>(axisScaleDraw(axis))) {
        if (scale->hover != hover) {
            scale->hover = hover;
            axisWidget(axis)->update();
        }
    }
}

void Plot::checkDuplicates(const QString name)
{DD;
    Curve *c1 = nullptr;
    int found = 0;
    for (auto c: qAsConst(curves)) {
        if (c->title() == name) {
            if (found)
                c->duplicate = true;
            else
                c1 = c;
            found++;
        }
    }
    if (c1) c1->duplicate = found>1;
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

void Plot::createLegend()
{DD;
    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, &CheckableLegend::markedToMove, this, [this](QwtPlotItem*curve){
        if (Curve *c = dynamic_cast<Curve*>(curve))
            moveCurve(c, c->yAxis() == QwtAxis::yLeft ? QwtAxis::yRight : QwtAxis::yLeft);
    });
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void Plot::recalculateScale(bool leftAxis)
{DD;
    ZoomStack::ScaleBounds *ybounds = 0;
    if (leftAxis) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    QList<Curve*> l;
    if (leftAxis || spectrogram) l = leftCurves;
    else l = rightCurves;

    ybounds->reset();
    for (Curve *c: qAsConst(l)) {
        ybounds->add(c->yMin(), c->yMax());
    }
}

bool Plot::plotCurve(Channel * ch, QColor *col, bool &plotOnRight, int fileNumber)
{DD;
    if (plotted(ch)) return false;



    spectrogram = ch->data()->blocksCount()>1;

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

    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(xBottomAxis, ch->xName());
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


    Curve *g = createCurve(ch->legendName(), ch);
    QColor nextColor = App->colors()->getColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    g->oldPen = pen;
    if (col) *col = nextColor;

    curves << g;
    g->fileNumber = fileNumber;
    checkDuplicates(g->title());

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
        zoom->verticalScaleBounds->add(ch->data()->zMin(), ch->data()->zMax());
    }
    else {
        ZoomStack::ScaleBounds *ybounds = 0;
        if (zoom->verticalScaleBounds->axis == ax.pos) ybounds = zoom->verticalScaleBounds;
        else ybounds = zoom->verticalScaleBoundsSlave;

        zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
        if (ybounds) ybounds->add(g->yMin(), g->yMax());
    }




    g->attachTo(this);

    update();
    emit curvesChanged();
    emit curvesCountChanged();

    return true;
}

//Curve * Plot::plotted(FileDescriptor *dfd, int channel) const
//{DD;
//    foreach (Curve *curve, curves) {
//        if (curve->descriptor == dfd && curve->channelIndex == channel) return curve;
//    }
//    return 0;
//}

Curve * Plot::plotted(Channel *channel) const
{DD;
    for (Curve *curve: qAsConst(curves)) {
        if (curve->channel == channel) return curve;
    }
    return 0;
}

Range Plot::xRange() const
{DD;
    QwtScaleMap sm = canvasMap(QwtAxis::xBottom);
    return {sm.s1(), sm.s2()};
}

Range Plot::yLeftRange() const
{DD;
    QwtScaleMap sm = canvasMap(QwtAxis::yLeft);
    return {sm.s1(), sm.s2()};
}

Range Plot::yRightRange() const
{DD;
    QwtScaleMap sm = canvasMap(QwtAxis::yRight);
    return {sm.s1(), sm.s2()};
}

void Plot::switchLabelsVisibility()
{DD;
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DD;
    for (Curve *curve: qAsConst(curves))
        curve->setTitle(curve->channel->legendName());

    updateLegend();
}

void Plot::savePlot() /*SLOT*/
{DD;
    ImageRenderDialog dialog(true, this);
    if (dialog.exec()) {
        importPlot(dialog.getPath(), dialog.getSize(), dialog.getResolution());
        App->setSetting("lastPicture", dialog.getPath());
    }
}

void Plot::copyToClipboard() /*SLOT*/
{DD;
    QTemporaryFile file("DeepSeaBase-XXXXXX.bmp");
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
                qDebug()<<"Could not load image from"<<fileName;
        }
    }
}

void Plot::print() /*SLOT*/
{DD;
    QPrinter printer;
    if (printer.isValid())
        importPlot(printer, ImageRenderDialog::defaultSize(), ImageRenderDialog::defaultResolution());
    else
        QMessageBox::warning(this, "Deepsea Base", "Не удалось найти подходящий принтер");
}

void Plot::importPlot(const QString &fileName, const QSize &size, int resolution) /*private*/
{DD;
    setAutoReplot(false);
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtAxis::yLeft);
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
{DD;
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
        QFont axisfont = axisFont(QwtAxis::yLeft);
        axisfont.setPointSize(axisfont.pointSize()-2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (axisEnabled(i)) setAxisFont(i, axisfont);

        insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
        renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);
        renderer.renderTo(this, printer);

        //восстанавливаем параметры графиков
        axisfont.setPointSize(axisfont.pointSize()+2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (axisEnabled(i)) setAxisFont(i, axisfont);

        createLegend();
    }

    setAutoReplot(true);
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
{DD;
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
{DD;
    zoom->autoscale(axis, spectrogram);
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DD;
    interactionMode = mode;
    if (_picker) _picker->setMode(mode);
    if (_canvas) _canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::switchCursor()
{DD;
    if (!_picker) return;

    bool pickerEnabled = _picker->isEnabled();
    _picker->setEnabled(!pickerEnabled);
    tracker->setEnabled(!pickerEnabled);
    App->setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(QwtPlotItem *item)
{DD;
    if (Curve *c = dynamic_cast<Curve *>(item)) {
        CurvePropertiesDialog dialog(c, this);
        connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
        connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
        dialog.exec();
    }
}


void Plot::dropEvent(QDropEvent *event)
{DD;
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
//        qDebug()<<myData->channels;
        emit needPlotChannels();
        event->acceptProposedAction();
    }
}

void Plot::dragEnterEvent(QDragEnterEvent *event)
{DD;
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
//        qDebug()<<myData->channels;
        event->acceptProposedAction();
    }
}
