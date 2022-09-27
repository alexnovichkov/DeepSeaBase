#include "spectrogram.h"
#include "plotmodel.h"
#include "zoomstack.h"
#include <QMenu>
#include <QMessageBox>
#include <qwt_scale_engine.h>
#include "logscaleengine.h"
#include "colormapfactory.h"
#include <qwt_scale_widget.h>
#include "channelsmimedata.h"
#include "cursors.h"
#include "logging.h"
#include "qwtplotimpl.h"
#include "curve.h"

Spectrogram::Spectrogram(QWidget *parent) : Plot(Enums::PlotType::Spectrogram, parent)
{DDD;

}

void Spectrogram::deleteCurve(Curve *curve, bool doReplot)
{DDD;
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
            m_plot->enableAxis(Enums::AxisType::atRight, false);
        }
        if (m->rightCurvesCount()==0) {
            yRightName.clear();
            m_plot->enableAxis(Enums::AxisType::atRight, false);
        }
        if (!hasCurves()) xName.clear();
        m_plot->setInfoVisible(true);
        if (doReplot) update();
    }
}

QString Spectrogram::pointCoordinates(const QPointF &pos)
{DDD;
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
{DDD;
    //две оси видны всегда
    m_plot->enableAxis(Enums::AxisType::atLeft, axisLabelsVisible);
    if (axisLabelsVisible)
        m_plot->setAxisTitle(Enums::AxisType::atLeft, yLeftName);
    else
        m_plot->setAxisTitle(Enums::AxisType::atLeft, "");

    //правая ось - цветовая шкала
    m_plot->enableAxis(Enums::AxisType::atRight, axisLabelsVisible);
    QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
    QString text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix));
    if (axisLabelsVisible)
        m_plot->setAxisTitle(Enums::AxisType::atRight, text);
    else
        m_plot->setAxisTitle(Enums::AxisType::atRight, "");

    if (m_plot->axisEnabled(Enums::AxisType::atBottom)) {
        m_plot->setAxisTitle(Enums::AxisType::atBottom, axisLabelsVisible ? xName : "");
    }
}

void Spectrogram::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{DDD;
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch))) {
        QMessageBox::warning(widget(), QString("Не могу построить канал"),
                             QString("Тип графика не подходит.\n"
                                     "Сначала очистите график."));
        return;
    }

    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(Enums::AxisType::atBottom, ch->xName());
    m_plot->enableAxis(Enums::AxisType::atBottom, true);

    yValuesPresentationLeft = DataHolder::ShowAsReals;
    yValuesPresentationRight = ch->data()->yValuesPresentation();
    ch->data()->setYValuesPresentation(yValuesPresentationRight);
    setAxis(Enums::AxisType::atLeft, ch->zName());
    m_plot->enableAxis(Enums::AxisType::atLeft, true);
    plotOnLeft = true;
    setAxis(Enums::AxisType::atRight, ch->yName());
    m_plot->enableAxis(Enums::AxisType::atRight, true);
    m_plot->enableColorBar(Enums::AxisType::atRight, true);

    Curve *g = m_plot->createCurve(ch->legendName(), ch);
    ch->setPlotted(true);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;
    g->setYAxis(Enums::AxisType::atLeft);

    m_plot->setColorMap(Enums::AxisType::atRight, {ch->data()->yMin(-1), ch->data()->yMax(-1)}, colorMap, g);

    zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
    zoom->verticalScaleBoundsSlave->add(g->yMin(), g->yMax());
    zoom->verticalScaleBounds->add(ch->data()->zMin(), ch->data()->zMax());


    m_plot->setInfoVisible(false);

    g->attach(this);

    update();
    updatePlottedIndexes();
//    updateCycled();
    emit channelPlotted(ch);
    emit curvesCountChanged(); //->MainWindow.updateActions
}

void Spectrogram::onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels)
{DDD;
    Q_UNUSED(plotOnLeft);
    emit needPlotChannels(true, channels);
}

void Spectrogram::updateBounds()
{DDD;
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
{DDD;
    Q_UNUSED(message);
    return ch->data()->blocksCount()>1 && !hasCurves();
}

bool Spectrogram::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{DDD;
    Q_UNUSED(ch);
    Q_UNUSED(message);
    return false;
}

void Spectrogram::setRightScale(Enums::AxisType id, double min, double max)
{DDD;
    if (!m->isEmpty() && id == Enums::AxisType::atRight) {
        m_plot->setColorMap(id, {min, max}, colorMap, m->curve(0));
    }
}

QMenu *Spectrogram::createMenu(Enums::AxisType axis, const QPoint &pos)
{DDD;
    Q_UNUSED(pos);
    QMenu *menu = new QMenu(widget());

    if (axis == Enums::AxisType::atBottom) {
        menu->addAction("Одинарный курсор", [=](){
            cursors->addSingleCursor(m_plot->localCursorPosition(pos), Cursor::Style::Cross);
        });
        menu->addAction("Гармонический курсор", [=](){
            cursors->addHarmonicCursor(m_plot->localCursorPosition(pos));
        });

        menu->addAction(xScaleIsLogarithmic?"Линейная шкала":"Логарифмическая шкала", [=](){
            if (xScaleIsLogarithmic)
                m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::Linear);
            else
                m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::Logarithmic);

            xScaleIsLogarithmic = !xScaleIsLogarithmic;
        });
    }

    bool curvesEmpty = true;
    bool leftCurves = true;
    int *ax = 0;

    if (axis == Enums::AxisType::atRight && m->leftCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationRight;
    }


    if (!curvesEmpty) {
        QAction *a = new QAction("Показывать как");
        QMenu *am = new QMenu(widget());
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

    if (axis == Enums::AxisType::atRight && m->leftCurvesCount()>0) {
        QAction *a = new QAction("Цветовая шкала");
        QMenu *am = new QMenu(widget());
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
                m_plot->setColorMap(map, m->curve(0, true));
                m_plot->replot();
            }
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    return menu;
}
