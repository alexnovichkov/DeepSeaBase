#include "spectrogram.h"
#include "spectrocurve.h"
#include "plotmodel.h"
#include "zoomstack.h"
#include <QMenu>
#include <QMessageBox>
#include <qwt_scale_engine.h>
#include "logscaleengine.h"
#include "colormapfactory.h"
#include <qwt_scale_widget.h>
#include "channelsmimedata.h"

Spectrogram::Spectrogram(QWidget *parent) : Plot(Plot::PlotType::Spectrogram, parent)
{

}

Curve *Spectrogram::createCurve(const QString &legendName, Channel *channel)
{
    return new SpectroCurve(legendName, channel);
}

void Spectrogram::deleteCurve(Curve *curve, bool doReplot)
{
    if (!curve) return;

    bool removedFromLeft = true;
    if (m->deleteCurve(curve, &removedFromLeft)) {
        emit curveDeleted(curve->channel); //->MainWindow.onChannelChanged

        if (removedFromLeft > 0) {
            zoom->verticalScaleBounds->removeToAutoscale(curve->yMin(), curve->yMax());
            zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->channel->data()->zMin(),
                                                              curve->channel->data()->zMax());
        }
        else {
            zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        zoom->horizontalScaleBounds->removeToAutoscale(curve->xMin(), curve->xMax());

        delete curve;

        if (m->leftCurvesCount()==0) {
            yLeftName.clear();
            yRightName.clear();
            enableAxis(QwtAxis::YRight, false);
        }
        if (m->rightCurvesCount()==0) {
            yRightName.clear();
            enableAxis(QwtAxis::YRight, false);
        }
        if (!hasCurves()) xName.clear();
        setInfoVisible(true);
        if (doReplot) update();
    }
}

QString Spectrogram::pointCoordinates(const QPointF &pos)
{
    bool success = false;
    double y = 0.0;
    if (auto c = model()->curve(0))
        y = c->channel->data()->YforXandZ(pos.x(), pos.y(), success);
    if (success)
        return smartDouble(pos.x())+", "+smartDouble(pos.y()) + ", "+smartDouble(y);
    else
        return smartDouble(pos.x())+", "+smartDouble(pos.y());
}

void Spectrogram::updateAxesLabels()
{
    //две оси видны всегда
    enableAxis(QwtAxis::YLeft, axisLabelsVisible);
    if (axisLabelsVisible)
        setAxisTitle(QwtAxis::YLeft, QwtText(yLeftName));
    else
        setAxisTitle(QwtAxis::YLeft, "");

    //правая ось - цветовая шкала
    enableAxis(QwtAxis::YRight, axisLabelsVisible);
    QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
    QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
    if (axisLabelsVisible)
        setAxisTitle(QwtAxis::YRight, text);
    else
        setAxisTitle(QwtAxis::YRight, "");

    if (axisEnabled(QwtAxis::XBottom)) {
        setAxisTitle(QwtAxis::XBottom, axisLabelsVisible ? xName : "");
    }
}

void Spectrogram::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch))) {
        QMessageBox::warning(this, QString("Не могу построить канал"),
                             QString("Тип графика не подходит.\n"
                                     "Сначала очистите график."));
        return;
    }

    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(xBottomAxis, ch->xName());
    prepareAxis(xBottomAxis);

    yValuesPresentationLeft = DataHolder::ShowAsReals;
    yValuesPresentationRight = ch->data()->yValuesPresentation();
    ch->data()->setYValuesPresentation(yValuesPresentationRight);
    setAxis(yLeftAxis, ch->zName());
    prepareAxis(yLeftAxis);
    plotOnLeft = true;
    setAxis(yRightAxis, ch->yName());
    setAxisVisible(yRightAxis);
    axisWidget(yRightAxis)->setColorBarEnabled(true);


    Curve *g = createCurve(ch->legendName(), ch);
    ch->setPlotted(true);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;
    g->setYAxis(yLeftAxis);

    axisWidget(yRightAxis)->setColorMap(QwtInterval(ch->data()->yMin(-1), ch->data()->yMax(-1)),
                                        ColorMapFactory::map(colorMap));
    if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(g)) {
        c->setColorMap(ColorMapFactory::map(colorMap));
    }

    zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
    zoom->verticalScaleBoundsSlave->add(g->yMin(), g->yMax());
    zoom->verticalScaleBounds->add(ch->data()->zMin(), ch->data()->zMax());


    setInfoVisible(false);

    g->attach(this);

    update();
    updatePlottedIndexes();
//    updateCycled();
    emit channelPlotted(ch);
    emit curvesCountChanged(); //->MainWindow.updateActions
}

void Spectrogram::onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels)
{
    Q_UNUSED(plotOnLeft);
    emit needPlotChannels(true, channels);
}

void Spectrogram::updateBounds()
{
    if (m->leftCurvesCount()==0) {
        zoom->verticalScaleBounds->reset();
        zoom->verticalScaleBoundsSlave->reset();
    }
    if (!hasCurves())
        zoom->horizontalScaleBounds->reset();
    if (!zoom->horizontalScaleBounds->isFixed())
        zoom->horizontalScaleBounds->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->verticalScaleBounds->isFixed())
        zoom->verticalScaleBounds->autoscale();
}

bool Spectrogram::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{
    Q_UNUSED(message);
    return ch->data()->blocksCount()>1 && !hasCurves();
}

bool Spectrogram::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{
    Q_UNUSED(ch);
    Q_UNUSED(message);
    return false;
}

void Spectrogram::setRightScale(QwtAxisId id, double min, double max)
{
    if (!m->isEmpty() && id == yRightAxis) {
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(m->curve(0))) {
            c->setColorInterval(min, max);
            axisWidget(id)->setColorMap(QwtInterval(min, max), ColorMapFactory::map(colorMap));
        }
    }
}

QMenu *Spectrogram::createMenu(QwtAxisId axis, const QPoint &pos)
{
    Q_UNUSED(pos);
    QMenu *menu = new QMenu(this);

    if (axis == QwtAxis::XBottom) {
        menu->addAction(xScaleIsLogarithmic?"Линейная шкала":"Логарифмическая шкала", [=](){
            if (xScaleIsLogarithmic)
                setAxisScaleEngine(xBottomAxis, new QwtLinearScaleEngine());
            else
                setAxisScaleEngine(xBottomAxis, new LogScaleEngine(2));

            xScaleIsLogarithmic = !xScaleIsLogarithmic;
        });
    }

    bool curvesEmpty = true;
    bool leftCurves = true;
    int *ax = 0;

    if (axis == QwtAxis::YRight && m->leftCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationRight;
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
            this->recalculateScale(axis == QwtAxis::YLeft);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (axis == QwtAxis::YRight && m->leftCurvesCount()>0) {
        if (SpectroCurve *c = dynamic_cast<SpectroCurve *>(m->curve(0, true))) {
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

    return menu;
}
