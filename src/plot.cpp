#include "plot.h"

#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "curve.h"

#include "chartzoom.h"

#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>

#include <QtCore>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>
#include <QMenu>

#include "mainwindow.h"
#include "graphpropertiesdialog.h"

#include "colorselector.h"
#include "legend.h"

#include "plotpicker.h"
#include <QAction>

#include "logging.h"
#include "trackingpanel.h"

#include "dataiodevice.h"
#include <QAudioOutput>

#define LOG_MIN_MY 1.0e-3

static inline QwtInterval logInterval( double base, const QwtInterval &interval )
{
    return QwtInterval( log(interval.minValue()) / log( base ),
            log(interval.maxValue()) / log( base ) );
}

class LogScaleEngine : public QwtLogScaleEngine
{
    // QwtScaleEngine interface
public:

    virtual void autoScale(int maxNumSteps, double &x1, double &x2, double &stepSize) const
    {
        if ( x1 > x2 )
            qSwap( x1, x2 );

        const double logBase = base();

        QwtInterval interval( x1 / qPow( logBase, lowerMargin() ),
                              x2 * qPow( logBase, upperMargin() ) );

        if ( interval.maxValue() / interval.minValue() < logBase )
        {
            // scale width is less than one step -> try to build a linear scale

            QwtLinearScaleEngine linearScaler;
            linearScaler.setAttributes( attributes() );
            linearScaler.setReference( reference() );
            linearScaler.setMargins( lowerMargin(), upperMargin() );

            linearScaler.autoScale( maxNumSteps, x1, x2, stepSize );

            QwtInterval linearInterval = QwtInterval( x1, x2 ).normalized();
            linearInterval = linearInterval.limited( LOG_MIN, LOG_MAX );

            if ( linearInterval.maxValue() / linearInterval.minValue() < logBase )
            {
                // the aligned scale is still less than one step
                if ( stepSize < 0.0 )
                    stepSize = - log(qAbs( stepSize )) /log( logBase );
                else
                    stepSize = log(stepSize) / log( logBase );

                return;
            }
        }

        double logRef = 1.0;
        if ( reference() > LOG_MIN_MY / 2.0 )
            logRef = qMin( reference(), LOG_MAX / 2 );

        if ( testAttribute( QwtScaleEngine::Symmetric ) )
        {
            const double delta = qMax( interval.maxValue() / logRef,
                                       logRef / interval.minValue() );
            interval.setInterval( logRef / delta, logRef * delta );
        }

        if ( testAttribute( QwtScaleEngine::IncludeReference ) )
            interval = interval.extend( logRef );

        interval = interval.limited( LOG_MIN_MY, LOG_MAX );

        if ( interval.width() == 0.0 )
            interval = buildInterval( interval.minValue() );

        stepSize = divideInterval( logInterval( logBase, interval ).width(),
                                   qMax( maxNumSteps, 1 ) );
        if ( stepSize < 1.0 )
            stepSize = 1.0;

        if ( !testAttribute( QwtScaleEngine::Floating ) )
            interval = align( interval, stepSize );

        x1 = interval.minValue();
        x2 = interval.maxValue();

        if ( testAttribute( QwtScaleEngine::Inverted ) )
        {
            qSwap( x1, x2 );
            stepSize = -stepSize;
        }
    }
    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize) const
    {
        QwtInterval interval = QwtInterval( x1, x2 ).normalized();
        interval = interval.limited( LOG_MIN_MY, LOG_MAX );

        if ( interval.width() <= 0 )
            return QwtScaleDiv();

        const double logBase = base();

        if ( interval.maxValue() / interval.minValue() < logBase )
        {
            // scale width is less than one decade -> build linear scale

            QwtLinearScaleEngine linearScaler;
            linearScaler.setAttributes( attributes() );
            linearScaler.setReference( reference() );
            linearScaler.setMargins( lowerMargin(), upperMargin() );

            if ( stepSize != 0.0 )
            {
                if ( stepSize < 0.0 )
                    stepSize = -qPow( logBase, -stepSize );
                else
                    stepSize = qPow( logBase, stepSize );
            }

            return linearScaler.divideScale( x1, x2,
                                             maxMajorSteps, maxMinorSteps, stepSize );
        }

        stepSize = qAbs( stepSize );
        if ( stepSize == 0.0 )
        {
            if ( maxMajorSteps < 1 )
                maxMajorSteps = 1;

            stepSize = divideInterval(
                           logInterval( logBase, interval ).width(), maxMajorSteps );
            if ( stepSize < 1.0 )
                stepSize = 1.0; // major step must be >= 1 decade
        }

        QwtScaleDiv scaleDiv;
        if ( stepSize != 0.0 )
        {
            QList<double> ticks[QwtScaleDiv::NTickTypes];
            buildTicks( interval, stepSize, maxMinorSteps, ticks );

            scaleDiv = QwtScaleDiv( interval, ticks );
        }

        if ( x1 > x2 )
            scaleDiv.invert();

        return scaleDiv;
    }
};


Plot::Plot(QWidget *parent) :
    QwtPlot(parent), zoom(0)
{DD;
    canvas = new QwtPlotCanvas();
    canvas->setFocusIndicator( QwtPlotCanvas::CanvasFocusIndicator);
    canvas->setFocusPolicy( Qt::StrongFocus);
    canvas->setPalette(Qt::white);
    canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(canvas);
    //setContextMenuPolicy(Qt::ActionsContextMenu);

    setAutoReplot(true);

    trackingPanel = new TrackingPanel(this);
    trackingPanel->setVisible(false);
    connect(trackingPanel,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));
    connect(this, SIGNAL(graphsChanged()), trackingPanel, SLOT(update()));

    axisLabelsVisible = MainWindow::getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;


    interactionMode = ScalingInteraction;

    // grid
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::darkGray, 0, Qt::DotLine);
    grid->attach(this);

    xScale = false;
    y1Scale = false;
    y2Scale = false;

    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteGraph(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);


    zoom = new ChartZoom(this);
    zoom->setZoomEnabled(true);
    connect(zoom,SIGNAL(updateTrackingCursor(double,bool)), trackingPanel, SLOT(setXValue(double,bool)));
    connect(zoom,SIGNAL(contextMenuRequested(QPoint,int)),SLOT(showContextMenu(QPoint,int)));
    connect(zoom,SIGNAL(moveCursor(bool)), trackingPanel, SLOT(moveCursor(bool)));

    picker = new PlotPicker(canvas);
    connect(picker,SIGNAL(labelSelected(bool)),zoom,SLOT(labelSelected(bool)));
    connect(picker,SIGNAL(updateTrackingCursor(double,bool)),trackingPanel, SLOT(setXValue(double,bool)));
    connect(picker,SIGNAL(cursorMovedTo(QwtPlotMarker*,double)), trackingPanel, SLOT(setXValue(QwtPlotMarker*,double)));
    connect(picker,SIGNAL(cursorSelected(QwtPlotMarker*)), trackingPanel, SLOT(updateSelectedCursor(QwtPlotMarker*)));
    connect(picker,SIGNAL(moveCursor(bool)), trackingPanel, SLOT(moveCursor(bool)));
    // передаем информацию об установленном курсоре в picker, чтобы тот смог обновить инфу о выделенном курсоре
    connect(trackingPanel,SIGNAL(cursorSelected(QwtPlotMarker*)), picker, SLOT(updateSelectedCursor(QwtPlotMarker*)));



    picker->setEnabled(MainWindow::getSetting("pickerEnabled", true).toBool());

    audio = 0;
    audioData = 0;
}

Plot::~Plot()
{DD;
//    delete freeGraph;
    delete trackingPanel;
    qDeleteAll(graphs);
    delete grid;
    delete picker;

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
    if (leftGraphs.isEmpty())
        zoom->verticalScaleBounds->reset();
    if (rightGraphs.isEmpty())
        zoom->verticalScaleBoundsSlave->reset();
    if (!hasGraphs())
        zoom->horizontalScaleBounds->reset();

    if (!zoom->horizontalScaleBounds->isFixed())
        zoom->horizontalScaleBounds->autoscale();
    if (!leftGraphs.isEmpty() && !zoom->verticalScaleBounds->isFixed())
        zoom->verticalScaleBounds->autoscale();
    if (!rightGraphs.isEmpty() && !zoom->verticalScaleBoundsSlave->isFixed())
        zoom->verticalScaleBoundsSlave->autoscale();

    replot();
}

bool Plot::hasGraphs() const
{DD;
    return !graphs.isEmpty();
}

void Plot::deleteGraphs()
{DD;
    qDeleteAll(graphs);
    graphs.clear();
    leftGraphs.clear();
    rightGraphs.clear();
    ColorSelector::instance()->resetState();

    yLeftName.clear();
    yRightName.clear();
    xName.clear();

    update();
    emit graphsChanged();
}

void Plot::deleteGraphs(FileDescriptor *descriptor)
{DD;
    for (int i = graphs.size()-1; i>=0; --i) {
        Curve *graph = graphs[i];
        if (descriptor == graph->descriptor) {
            deleteGraph(graph, true);
        }
    }
}

void Plot::deleteGraph(QwtPlotItem*item)
{DD;
    Curve *c = dynamic_cast<Curve *>(item);
    if (c) {
        emit curveDeleted(c->descriptor, c->channelIndex);
        deleteGraph(c, true);
    }
}

void Plot::showContextMenu(const QPoint &pos, const int axis)
{DD;
    if (!hasGraphs()) return;

    QMenu *menu = new QMenu(this);
    bool *scale = 0;

    if (axis == QwtPlot::xBottom) scale = &xScale;
    if (axis == QwtPlot::yLeft) scale = &y1Scale;
    if (axis == QwtPlot::yRight) scale = &y2Scale;

    if (scale && axis == QwtPlot::xBottom)
    menu->addAction((*scale)?"Линейная шкала":"Логарифмическая шкала", [=](){
        QwtScaleEngine *engine = 0;
        if (!(*scale)) {
            engine = new LogScaleEngine();
            engine->setBase(2);
            //engine->setTransformation(new LogTransform);
        }
        else {
            engine = new QwtLinearScaleEngine();
        }
        setAxisScaleEngine(axis, engine);

        if (scale) *scale = !(*scale);
    });

    // определяем, все ли графики представляют временные данные
    bool time = true;

    foreach (Curve *c, graphs) {
        if (c->channel->type() != Descriptor::TimeResponse) {
            time = false;
            break;
        }
    }

    if (time && axis == QwtPlot::xBottom) {
        menu->addAction("Сохранить временной сегмент", [=](){
            double xStart = canvasMap(xBottom).s1();
            double xEnd = canvasMap(xBottom).s2();

            QList<FileDescriptor*> files;

            foreach(Curve *c, graphs) {
                if (!files.contains(c->descriptor))
                    files << c->descriptor;
            }

            emit saveTimeSegment(files, xStart, xEnd);
        });
    }

    QList<Curve*> list;
    int *ax = 0;
    if (axis == QwtPlot::yLeft && !leftGraphs.isEmpty()) {
        list = leftGraphs;
        ax = &yValuesPresentationLeft;
    }
    if (axis == QwtPlot::yRight && !rightGraphs.isEmpty()) {
        list = rightGraphs;
        ax = &yValuesPresentationRight;
    }
    if (!list.isEmpty()) {
        QAction *a = new QAction("Показывать как");
        QMenu *am = new QMenu(this);
        QActionGroup *ag = new QActionGroup(am);

        QAction *act3 = new QAction("Амплитуды линейные", ag);
        act3->setCheckable(true);
        if (*ax == DataHolder::ShowAsAmplitudes) act3->setChecked(true);
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
            this->recalculateScale(axis == QwtPlot::yLeft);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (!leftGraphs.isEmpty() && !rightGraphs.isEmpty()) {
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
            coords.coords.insert(zoom->masterV(), {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            QwtScaleMap rightMap = canvasMap(yRight);
            s1 = rightMap.s1();
            s2 = rightMap.s2();

            p1 = rightMap.p1();
            p2 = rightMap.p2();
            delta = rightMap.invTransform((p1+p2)/2.0);

            coords.coords.insert(zoom->slaveV(), {s1-delta, s2-delta});
            zoom->addZoom(coords, true);
        });
        menu->addAction("Совместить диапазоны левой и правой осей", [=](){
            QwtScaleMap leftMap = canvasMap(yLeft);
            double s1 = leftMap.s1();
            double s2 = leftMap.s2();

            QwtScaleMap rightMap = canvasMap(yRight);
            double s3 = rightMap.s1();
            double s4 = rightMap.s2();
            double s = std::min(s1,s3);
            double ss = std::min(s2,s4);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(zoom->masterV(), {s, ss});
            coords.coords.insert(zoom->slaveV(), {s, ss});

            zoom->addZoom(coords, true);
        });
    }

    menu->exec(pos);
}

void Plot::deleteGraph(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *graph = plotted(dfd, channel)) {
        deleteGraph(graph, doReplot);
    }
}

void Plot::deleteGraph(Channel *c, bool doReplot)
{DD;
    if (Curve *graph = plotted(c)) {
        deleteGraph(graph, doReplot);
    }
}

void Plot::deleteGraph(Curve *graph, bool doReplot)
{DD;
    if (graph) {
        ColorSelector::instance()->freeColor(graph->pen().color());

        int removed = leftGraphs.removeAll(graph);
        if (removed > 0) {
            zoom->verticalScaleBounds->removeToAutoscale(graph->yMin(), graph->yMax());
        }

        removed = rightGraphs.removeAll(graph);
        if (removed > 0) {
            zoom->verticalScaleBoundsSlave->removeToAutoscale(graph->yMin(), graph->yMax());
        }
        removed = graphs.removeAll(graph);
        if (removed > 0) {
            zoom->horizontalScaleBounds->removeToAutoscale(graph->xMin(), graph->xMax());
        }
        QString title = graph->title().text();


        delete graph;
        graph = 0;
        checkDuplicates(title);

        if (leftGraphs.isEmpty()) {
            yLeftName.clear();
        }
        if (rightGraphs.isEmpty()) {
            yRightName.clear();
            enableAxis(QwtPlot::yRight, false);
        }
        if (!hasGraphs()) {
            xName.clear();
        }

        if (doReplot) {
            update();
        }
        emit graphsChanged();
    }
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch)
{DD;
    if (!hasGraphs()) // нет графиков - можем построить что угодно
        return true;
    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (leftGraphs.isEmpty() || yLeftName.isEmpty() || ch->yName() == yLeftName)
            return true;
    }
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch)
{DD;
    if (!hasGraphs()) // нет графиков - всегда на левой оси
        return true;
    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (rightGraphs.isEmpty() || yRightName.isEmpty() || ch->yName() == yRightName)
            return true;
    }
    return false;
}

void Plot::prepareAxis(int axis)
{DD;
    if (!axisEnabled(axis))
        enableAxis(axis);
}

void Plot::setAxis(int axis, const QString &name)
{DD;
    switch (axis) {
        case QwtPlot::yLeft: yLeftName = name; break;
        case QwtPlot::yRight: yRightName = name; break;
        case QwtPlot::xBottom: xName = name; break;
        default: break;
    }
}

void Plot::moveToAxis(int axis, double min, double max)
{DD;
    switch (axis) {
        case QwtPlot::xBottom:
            zoom->horizontalScaleBounds->add(min, max);
            if (!zoom->horizontalScaleBounds->isFixed()) zoom->horizontalScaleBounds->autoscale();
            break;
        case QwtPlot::yLeft:
            zoom->verticalScaleBoundsSlave->removeToAutoscale(min, max);
            zoom->verticalScaleBounds->add(min, max);
            if (!zoom->verticalScaleBounds->isFixed()) zoom->verticalScaleBounds->autoscale();
            break;
        case QwtPlot::yRight:
            zoom->verticalScaleBounds->removeToAutoscale(min, max);
            zoom->verticalScaleBoundsSlave->add(min, max);
            if (!zoom->verticalScaleBoundsSlave->isFixed()) zoom->verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{DD;
    if (leftGraphs.isEmpty()) enableAxis(QwtPlot::yLeft, false);
    else {
        enableAxis(QwtPlot::yLeft, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
        QwtText text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtPlot::yLeft, text);
        else
            setAxisTitle(QwtPlot::yLeft, "");
    }

    if (rightGraphs.isEmpty()) enableAxis(QwtPlot::yRight, false);
    else {
        enableAxis(QwtPlot::yRight, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
        QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtPlot::yRight, text);
        else
            setAxisTitle(QwtPlot::yRight, "");
    }
    if (axisEnabled(QwtPlot::xBottom)) {
        setAxisTitle(QwtPlot::xBottom, axisLabelsVisible ? xName : "");
    }
}

void Plot::removeLabels()
{
    foreach (Curve *c, graphs) {
        c->removeLabels();
    }
   replot();
}

void Plot::moveGraph(Curve *curve)
{DD;
    if (leftGraphs.contains(curve)) {
        leftGraphs.removeAll(curve);
        if (rightGraphs.isEmpty()) {
            yValuesPresentationRight = curve->channel->data()->yValuesPresentation();
        }
        curve->channel->data()->setYValuesPresentation(yValuesPresentationRight);
        rightGraphs.append(curve);
    }
    else if (rightGraphs.contains(curve)) {
        rightGraphs.removeAll(curve);
        if (leftGraphs.isEmpty()) {
            yValuesPresentationLeft = curve->channel->data()->yValuesPresentation();
        }
        curve->channel->data()->setYValuesPresentation(yValuesPresentationLeft);
        leftGraphs.append(curve);
    }
    emit graphsChanged();
}

bool Plot::hasDuplicateNames(const QString name) const
{
    int count = 0;
    foreach(Curve *c, graphs) {
        if (c->title().text() == name) count++;
    }
    return (count > 1);
}

void Plot::checkDuplicates(const QString name)
{
    QList<int> l;
    for( int i=0; i<graphs.size(); ++i) {
        if (graphs[i]->title().text() == name) l << i;
    }
    foreach(int i, l) graphs[i]->duplicate = l.size()>1;
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
    if (leftAxis) l = leftGraphs;
    else l = rightGraphs;

    ybounds->reset();
    foreach (Curve *c, l) {
        ybounds->add(c->yMin(), c->yMax());
    }

}

void Plot::playChannel(Channel *ch)
{DDD;
    if (!ch) return;

    if (ch->type()!=Descriptor::TimeResponse) return;

    if (audio) {
        audio->stop();
        delete audio;
        audio = 0;
    }

    if (audioData) {
        delete audioData;
        audioData = 0;
    }

    audioData = new DataIODevice(ch, this);
    audioData->open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(qRound(1.0/ch->xStep()));
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioOutput(format, this);
    audio->setBufferSize(2 * qRound(1.0/ch->xStep()));
    audio->setNotifyInterval(1000);

    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(audioStateChanged(QAudio::State)));
    connect(audio, SIGNAL(notify()), this, SLOT(audioPosChanged()));
    audio->start(audioData);
    audio->setVolume(0.01);
}

void Plot::audioStateChanged(QAudio::State state)
{DDD;
    DebugPrint(state);

    switch (state) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            audio->stop();
            audioData->close();
            delete audio;
            audio = 0;
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (audio->error() != QAudio::NoError) {
                // Error handling
                qDebug()<<"audio stopped:"<<audio->error();
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}

void Plot::audioPosChanged()
{
    if (!audioData) return;
    qDebug() << "currently at" << audioData->position()<<","<<audioData->positionSec();
}

bool Plot::plotChannel(FileDescriptor *descriptor, int channel, QColor *col, bool plotOnRight, int fileNumber)
{DD;
    if (plotted(descriptor, channel)) return false;

    Channel *ch = descriptor->channel(channel);
    if (!ch->populated()) {
        ch->populate();
    }

    bool plotOnFirstYAxis = canBePlottedOnLeftAxis(ch);
    bool plotOnSecondYAxis = plotOnFirstYAxis ? false : canBePlottedOnRightAxis(ch);
    if (plotOnRight) {
        plotOnFirstYAxis = false;
        plotOnSecondYAxis = canBePlottedOnRightAxis(ch);
    }

    static bool skipped = false;
    if (!plotOnFirstYAxis && !plotOnSecondYAxis) {
        if (!skipped) {
            QMessageBox::warning(this, QString("Не могу построить канал"),
                                 QString("Тип графика не подходит.\n"
                                         "Сначала очистите график."));
            skipped = true;
        }
        return false;
    }
    else
        skipped = false;

    setAxis(QwtPlot::xBottom, descriptor->xName());
    prepareAxis(QwtPlot::xBottom);

    // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
    if (plotOnFirstYAxis && leftGraphs.isEmpty()) {
        yValuesPresentationLeft = ch->data()->yValuesPresentation();
    }
    if (plotOnSecondYAxis && rightGraphs.isEmpty()) {
        yValuesPresentationRight = ch->data()->yValuesPresentation();
    }

    if (plotOnFirstYAxis) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
    else ch->data()->setYValuesPresentation(yValuesPresentationRight);

    QwtPlot::Axis ax = plotOnFirstYAxis ? QwtPlot::yLeft : QwtPlot::yRight;

    setAxis(ax, ch->yName());
    prepareAxis(ax);


    Curve *g = new Curve(ch->legendName(), descriptor, channel);
    QColor nextColor = ColorSelector::instance()->getColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    g->oldPen = pen;
    if (col) *col = nextColor;

    graphs << g;
    g->fileNumber = fileNumber;
    checkDuplicates(g->title().text());
    if (hasDuplicateNames(g->title().text())) {
        g->duplicate = true;
    }

    g->setYAxis(ax);
    if (plotOnSecondYAxis) {
        rightGraphs << g;
    }
    else {
        leftGraphs << g;
    }


    ChartZoom::ScaleBounds *ybounds = 0;
    if (zoom->verticalScaleBounds->axis == ax) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
    ybounds->add(g->yMin(), g->yMax());

    g->attach(this);
    update();
    emit graphsChanged();

    playChannel(ch);
    return true;
}

Curve * Plot::plotted(FileDescriptor *dfd, int channel) const
{DD;
    foreach (Curve *graph, graphs) {
        if (graph->descriptor == dfd && graph->channelIndex == channel) return graph;
    }
    return 0;
}

Curve * Plot::plotted(Channel *channel) const
{DD;
    foreach (Curve *graph, graphs) {
        if (graph->channel == channel) return graph;
    }
    return 0;
}

Range Plot::xRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::xBottom);
    return {sm.s1(), sm.s2()};
}

Range Plot::yLeftRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::yLeft);
    return {sm.s1(), sm.s2()};
}

Range Plot::yRightRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::yRight);
    return {sm.s1(), sm.s2()};
}

void Plot::switchLabelsVisibility()
{
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DD;
    foreach (Curve *graph, leftGraphs) {
        graph->setTitle(graph->channel->legendName());
    }
    foreach (Curve *graph, rightGraphs) {
        graph->setTitle(graph->channel->legendName());
    }
    updateLegend();
}

void Plot::savePlot()
{DD;
    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture, "Изображения (*.bmp)");
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
        default:
            break;
    }
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DD;
    interactionMode = mode;
    if (picker) picker->setMode(mode);
    if (zoom)
        zoom->setZoomEnabled(mode == ScalingInteraction);
    if (canvas) canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::importPlot(const QString &fileName)
{DD;
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtPlot::yLeft);
    axisfont.setPointSize(axisfont.pointSize()+1);

    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *graph, graphs) {
        QPen pen = graph->pen();
        if (pen.width()<2) pen.setWidth(2);
        pen.setColor(pen.color().lighter(120));
        graph->setPen(pen);
//        graph->setTitle(QwtText("<font size=5>"+graph->channel->legendName()+"</font>"));
    }

    QwtAbstractLegend *leg = new Legend();
    insertLegend(leg, QwtPlot::BottomLegend);


    renderer.renderDocument(this, fileName, QSizeF(400,200), qApp->desktop()->logicalDpiX());


    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *graph, graphs) {
        graph->setPen(graph->oldPen);
        graph->setTitle(QwtText(graph->channel->legendName()));
    }

    leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteGraph(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void Plot::switchCursor()
{DD;
    if (!picker) return;

    bool pickerEnabled = picker->isEnabled();
    picker->setEnabled(!pickerEnabled);
    MainWindow::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(const QVariant &itemInfo, int index)
{DD;
    Q_UNUSED(index)

    QwtPlotItem *item = infoToItem(itemInfo);
    if (item) {
        Curve *c = dynamic_cast<Curve *>(item);
        if (c) {
            GraphPropertiesDialog dialog(c, this);
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
        GraphPropertiesDialog dialog(c, this);
        connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
        connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
        dialog.exec();
    }
}


