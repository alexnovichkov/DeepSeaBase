#include "spectrogram.h"
#include "plotmodel.h"
#include "zoomstack.h"
#include <QMenu>
#include <QMessageBox>
#include "colormapfactory.h"
#include "channelsmimedata.h"
#include "cursors.h"
#include "logging.h"
#include "qcpplot.h"
#include "curve.h"
#include "unitsconverter.h"
#include "checkablelegend.h"
#include "plottedmodel.h"
#include "picker.h"

Spectrogram::Spectrogram(QWidget *parent) : Plot(Enums::PlotType::Spectrogram, parent)
{DD;

}

void Spectrogram::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (!curve) return;
    if (curve->selected()) picker->deselect();

    if (m->deleteCurve(curve)) {
        PlottedModel::instance().remove(curve);
        emit curveDeleted(curve->channel); //->MainWindow.onChannelChanged
//        zoom->scaleBounds(Enums::AxisType::atColor)->removeToAutoscale(curve->channel->data()->yMin(-1), curve->channel->data()->yMax(-1));
//        zoom->scaleBounds(Enums::AxisType::atLeft)->removeToAutoscale(curve->channel->data()->zMin(), curve->channel->data()->zMax());
//        zoom->scaleBounds(Enums::AxisType::atBottom)->removeToAutoscale(curve->xMin(), curve->xMax());

//        zoom->autoscale(Enums::AxisType::atInvalid);

//        m_plot->updateSecondaryPlots({qQNaN(), qQNaN()});

        curve->detachFrom(this);
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
        if (!m->isEmpty()) xName.clear();
        if (m_plot) m_plot->setInfoVisible(m->size()==0);

        if (doReplot) update();
    }
}

QString Spectrogram::pointCoordinates(const QPointF &pos)
{DD;
    bool success = false;
    double y = 0.0;
    if (auto c = model()->curve(0))
        y = c->channel->data()->YforXandZ(pos.x(), pos.y(), success);
    if (success)
        return smartDouble(pos.x(), m_plot->tickDistance(Enums::AxisType::atBottom))+", "
                +smartDouble(pos.y(), m_plot->tickDistance(Enums::AxisType::atLeft)) + ", "
                +smartDouble(y, m_plot->tickDistance(Enums::AxisType::atRight));
    else
        return smartDouble(pos.x(), m_plot->tickDistance(Enums::AxisType::atBottom))+", "
                +smartDouble(pos.y(), m_plot->tickDistance(Enums::AxisType::atLeft));
}

void Spectrogram::updateAxesLabels()
{DD;
    m_plot->enableAxis(Enums::AxisType::atLeft, axisLabelsVisible);
    if (axisLabelsVisible)
        m_plot->setAxisTitle(Enums::AxisType::atLeft, yLeftName);
    else
        m_plot->setAxisTitle(Enums::AxisType::atLeft, "");

    //правая ось - цветовая шкала
    m_plot->enableAxis(Enums::AxisType::atRight, false);
    m_plot->enableColorBar(Enums::AxisType::atRight, true);
    QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
    QString text(QString("%1 %2").arg(yRightName).arg(suffix));
    if (axisLabelsVisible)
        m_plot->setColorBarTitle(text);
    else
        m_plot->setColorBarTitle("");

    if (m_plot->axisEnabled(Enums::AxisType::atBottom)) {
        m_plot->setAxisTitle(Enums::AxisType::atBottom, axisLabelsVisible ? xName : "");
    }
}

void Spectrogram::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{DD;
    if (!ch || !m_plot) return;
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    QString message;
    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch, &message)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch, &message))) {
        QMessageBox::warning(widget(), QString("Не могу построить канал"),
                             QString("Тип графика не подходит.\n"
                                     "Сначала очистите график."));
        return;
    }

    //скрываем все до этого построенные каналы
    for (auto c: m->curves()) {
        c->setVisible(false);
        if (auto p = dynamic_cast<QCPAbstractPlottable*>(c)) p->setVisible(false);
        c->updateInLegend();
    }


    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(Enums::AxisType::atBottom, ch->xName());
    m_plot->enableAxis(Enums::AxisType::atBottom, true);

    yValuesPresentationLeft = DataHolder::ShowAsReals;
    if (m->isEmpty()) {
        yValuesPresentationRight = ch->data()->yValuesPresentation();
    }
    ch->data()->setYValuesPresentation(yValuesPresentationRight);
    setAxis(Enums::AxisType::atLeft, ch->zName());
    m_plot->enableAxis(Enums::AxisType::atLeft, true);
    plotOnLeft = true;
    setAxis(Enums::AxisType::atRight, ch->yName());
    m_plot->enableColorBar(Enums::AxisType::atRight, true);

    Curve *g = m_plot->createCurve(ch, Enums::AxisType::atBottom, Enums::AxisType::atLeft);
    PlottedModel::instance().add(ch, g);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;

    m_plot->setColorMap(colorMap, g);

    zoom->scaleBounds(Enums::AxisType::atBottom)->add(g->xMin(), g->xMax(), true);
    zoom->scaleBounds(Enums::AxisType::atColor)->add(ch->data()->yMin(-1), ch->data()->yMax(-1), true);
    zoom->scaleBounds(Enums::AxisType::atLeft)->add(ch->data()->zMin(), ch->data()->zMax(), true);

    m_plot->setInfoVisible(false);

    g->setLegend(m_plot->checkableLegend);
    g->attachTo(this);
    m_plot->setCurrentCurve(g);
//    m_plot->updateSecondaryPlots({qQNaN(), qQNaN()});
    update();
    updatePlottedIndexes();
    emit channelPlotted(ch);
}

void Spectrogram::onDropEvent(bool plotOnLeft, const QVector<Channel *> &channels)
{DD;
    Q_UNUSED(plotOnLeft);
    emit needPlotChannels(true, channels);
}

void Spectrogram::updateBounds()
{DD;
    if (m->leftCurvesCount()==0) {
        zoom->scaleBounds(Enums::AxisType::atLeft)->reset();
        zoom->scaleBounds(Enums::AxisType::atColor)->reset();
    }
    if (m->isEmpty())
        zoom->scaleBounds(Enums::AxisType::atBottom)->reset();
    if (!zoom->scaleBounds(Enums::AxisType::atBottom)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atBottom)->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atLeft)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atLeft)->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atColor)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atColor)->autoscale();
}

bool Spectrogram::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{DD;
    Q_UNUSED(message);
    if (m->isEmpty()) return true;

    if (ch->data()->blocksCount()<=1) return false;

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0 || yLeftName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->zName(), yLeftName))
            return true;
        else if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

bool Spectrogram::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{DD;
    Q_UNUSED(ch);
    Q_UNUSED(message);
    if (m->isEmpty()) return true;

    return false;
}

QMenu *Spectrogram::createMenu(Enums::AxisType axis, const QPoint &pos)
{DD;
    Q_UNUSED(pos);
    QMenu *menu = new QMenu(widget());

    if (axis == Enums::AxisType::atBottom) {
        menu->addAction("Одинарный курсор", [=](){
            auto cursor = cursors->addSingleCursor(m_plot->localCursorPosition(pos), Cursor::Style::Cross);
            m_plot->addCursorToSecondaryPlots(cursor);
        });
        menu->addAction("Гармонический курсор", [=](){
            auto cursor = cursors->addHarmonicCursor(m_plot->localCursorPosition(pos));
            m_plot->addCursorToSecondaryPlots(cursor);
        });

        menu->addAction(m_plot->axisScale(axis)==Enums::AxisScale::Linear?"Линейная шкала":"Логарифмическая шкала", [=]() {
            if (m_plot->axisScale(axis)==Enums::AxisScale::Logarithmic)
                m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::Linear);
            else
                m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::Logarithmic);
        });
    }

    bool curvesEmpty = true;
    int *ax = 0;

    if (axis == Enums::AxisType::atColor && m->leftCurvesCount()>0) {
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
            m->setYValuesPresentation(true, presentation);
            *ax = presentation;
            this->recalculateScale(axis);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (axis == Enums::AxisType::atColor && m->leftCurvesCount()>0) {
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
