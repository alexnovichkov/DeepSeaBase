#include "curve.h"
#include "qwt_symbol.h"
#include "pointlabel.h"

#include "fileformats/filedescriptor.h"
#include <qwt_curve_fitter.h>
#include "logging.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_clipper.h"
#include "dataholder.h"
#include "qwt_legend_data.h"
#include "pointmarker.h"
#include "plot.h"

Curve::Curve(const QString &title, Channel *channel)
{DD;
    Q_UNUSED(title)

    this->channel = channel;
    this->duplicate = false;
    this->channel->curve = this;
    marker = new PointMarker(this);
}

Curve::~Curve()
{DD;
    foreach(PointLabel *l, labels) l->detach();
    qDeleteAll(labels);
    labels.clear();
    delete marker;

    //maybe clear data that is over 1000000 samples
    if (channel) {
        channel->maybeClearData();
        channel->curve = nullptr;
    }
}

void Curve::attach(Plot *plot)
{
    m_plot = plot;
    marker->attach(plot);
    marker->setVisible(false);
    attachTo(plot);
}

void Curve::addLabel(PointLabel *label)
{DD;
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{DD;
    if (labels.contains(label)) {
        labels.removeOne(label);
        label->detach();
        delete label;
    }
}

void Curve::removeLabels()
{
    foreach (PointLabel *label, labels) {
        label->detach();
        delete label;
    }
    labels.clear();
}

PointLabel *Curve::findLabel(const QPoint &pos/*, QwtAxisId yAxis*/)
{DD;
    foreach (PointLabel *l, labels)
        if (l->contains(pos/*, yAxis*/))
            return l;

    return 0;
}

PointLabel *Curve::findLabel(const int point)
{DD;
    foreach (PointLabel *l, labels)
        if (l->point() == point)
            return l;

    return 0;
}

void Curve::moveToPos(QPoint pos, QPoint startPos)
{
    Q_UNUSED(startPos);
    if (m_plot->interactionMode != Plot::DataInteraction) return;

    if (selectedPoint < 0 || selectedPoint >= samplesCount()) return;
    double y = m_plot->invTransform(yAxis(), pos.y());
    if (channel->data()->setYValue(selectedPoint, y)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection();
    }
}

double Curve::yMin() const
{DD;
    return channel->data()->yMin();
}

double Curve::yMax() const
{DD;
    return channel->data()->yMax();
}

double Curve::xMin() const
{DD;
    return channel->data()->xMin();
}

double Curve::xMax() const
{DD;
    return channel->data()->xMax();
}

int Curve::samplesCount() const
{DD;
    return channel->data()->samplesCount();
}



void Curve::setVisible(bool visible)
{DD;
    //d->setVisible(visible);
    foreach (PointLabel *label, labels) {
        label->setVisible(visible);
    }
    if (marker) marker->setVisible(visible);
}

void Curve::evaluateScale(int &from, int &to, const QwtScaleMap &xMap) const
{DD;
    const double startX = xMap.s1();
    const double endX = xMap.s2();

    if (channel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        const auto min = channel->data()->xMin();
        const auto step = channel->data()->xStep();
        if (!qFuzzyIsNull(step)) {
            from = qRound((startX - min)/step)-1;
            to = qRound((endX - min)/step)+1;
        }
    }
    else {
        //допустимо использовать обычные циклы, потому что при неравномерной шкале
        //отсчетов всегда небольшое число (<50)
        for (int i=0; i<to; ++i) {
            if (samplePoint(i).x() >= startX) {
                from = i-1;
                break;
            }
        }
        for (int i=to; i>=from; --i) {
            if (samplePoint(i).x() <= endX) {
                to = i+1;
                break;
            }
        }
    }
    if (from < 0) from = 0;
    if (to >= channel->data()->samplesCount()) to = channel->data()->samplesCount()-1;
}

void Curve::switchFixed()
{DD;
    fixed = !fixed;
}

QMap<int, QVariant> Curve::commonLegendData() const
{
    QMap<int, QVariant> data;
    data.insert(QwtLegendData::UserRole+3, pen().color());
    data.insert(QwtLegendData::TitleRole, title());
    if (duplicate && fileNumber>0)
        data.insert(QwtLegendData::UserRole+1, fileNumber);
    data.insert(QwtLegendData::UserRole+2, selected());
    data.insert(QwtLegendData::UserRole+4, fixed);
    return data;
}



bool Curve::underMouse(const QPoint &pos, double *distanceX, double *distanceY) const
{
    selectedPoint = closest(pos, distanceX, distanceY);

    //no closest point for this pos
    if (selectedPoint == -1) return false;

    if (distanceX && distanceY) {
        if ((*distanceX)*(*distanceX)+(*distanceY)*(*distanceY) < 25)
            return true;
    }
    return false;
}

void Curve::moveLeft(int count)
{
    if (selectedPoint >= count) {
        selectedPoint -= count;
        updateSelection();
    }
}

void Curve::moveRight(int count)
{
    if (selectedPoint >=0 && selectedPoint < samplesCount()-count) {
        selectedPoint += count;
        updateSelection();
    }
}

void Curve::moveUp(int count)
{
    if (m_plot->interactionMode != Plot::DataInteraction) return;
    if (selectedPoint < 0 || selectedPoint >= samplesCount()) return;

    QPointF val = samplePoint(selectedPoint);
    double y = val.y()+(m_plot->canvasMap(yAxis()).sDist())/100*count;

    if (channel->data()->setYValue(selectedPoint, y)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection();
    }
}

void Curve::moveDown(int count)
{
    if (m_plot->interactionMode != Plot::DataInteraction) return;
    if (selectedPoint < 0 || selectedPoint >= samplesCount()) return;

    QPointF val = samplePoint(selectedPoint);
    double y = val.y()-(m_plot->canvasMap(yAxis()).sDist())/100*count;

    if (channel->data()->setYValue(selectedPoint, y)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection();
    }
}

void Curve::fix()
{
    if (selectedPoint >= 0 && selectedPoint < samplesCount()) {
        QPointF val = samplePoint(selectedPoint);

        PointLabel *label = findLabel(selectedPoint);

        if (!label) {
            label = new PointLabel(m_plot, this);
            label->setPoint(selectedPoint);
            label->setOrigin(val);
            addLabel(label);

            label->attach(m_plot);
        }
    }
}

void Curve::remove()
{
    m_plot->deleteSelectedCurve(this);
}

void Curve::updateSelection()
{
    updatePen();

    if (!selected()) marker->setVisible(false);
    else {
        marker->setVisible(true);
        marker->moveTo(samplePoint(selectedPoint));
    }
}
