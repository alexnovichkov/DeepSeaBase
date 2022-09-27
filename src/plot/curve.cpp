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
{DDD;
    Q_UNUSED(title)

    this->channel = channel;
    this->duplicate = false;
    this->channel->curve = this;
    marker = new PointMarker(this);
}

Curve::~Curve()
{DDD;
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
{DDD;
    m_plot = plot;
    marker->attach(plot);
    marker->setVisible(false);
}

void Curve::addLabel(PointLabel *label)
{DDD;
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{DDD;
    if (labels.contains(label)) {
        labels.removeOne(label);
        label->detach();
        delete label;
    }
}

void Curve::removeLabels()
{DDD;
    foreach (PointLabel *label, labels) {
        label->detach();
        delete label;
    }
    labels.clear();
}

PointLabel *Curve::findLabel(const QPoint &pos/*, QwtAxisId yAxis*/)
{DDD;
    foreach (PointLabel *l, labels)
        if (l->contains(pos/*, yAxis*/))
            return l;

    return 0;
}

PointLabel *Curve::findLabel(SelectedPoint point)
{DDD;
    foreach (PointLabel *l, labels)
        if (l->point() == point)
            return l;

    return 0;
}

void Curve::moveToPos(QPoint pos, QPoint startPos)
{DDD;
    Q_UNUSED(startPos);
    if (m_plot->interactionMode != Enums::InteractionMode::DataInteraction) return;

    if (selectedPoint.x < 0 || selectedPoint.x >= samplesCount()) return;
    double y = m_plot->screenToPlotCoordinates(yAxis(), pos.y());
    if (channel->data()->setYValue(selectedPoint.x, y, selectedPoint.z)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection(selectedPoint);
    }
}

double Curve::yMin() const
{DDD;
    return channel->data()->yMin();
}

double Curve::yMax() const
{DDD;
    return channel->data()->yMax();
}

double Curve::xMin() const
{DDD;
    return channel->data()->xMin();
}

double Curve::xMax() const
{DDD;
    return channel->data()->xMax();
}

int Curve::samplesCount() const
{DDD;
    return channel->data()->samplesCount();
}

void Curve::updateLabels()
{
    for (auto label: labels) label->updateLabel();
}

void Curve::setVisible(bool visible)
{DDD;
    //d->setVisible(visible);
    foreach (PointLabel *label, labels) {
        label->setVisible(visible);
    }
    if (marker) marker->setVisible(visible);
}

void Curve::evaluateScale(int &from, int &to, double startX, double endX) const
{DDD;
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
            if (samplePoint({i,0}).x >= startX) {
                from = i-1;
                break;
            }
        }
        for (int i=to; i>=from; --i) {
            if (samplePoint({i,0}).x <= endX) {
                to = i+1;
                break;
            }
        }
    }
    if (from < 0) from = 0;
    if (to >= channel->data()->samplesCount()) to = channel->data()->samplesCount()-1;
}

void Curve::switchFixed()
{DDD;
    fixed = !fixed;
}

QMap<int, QVariant> Curve::commonLegendData() const
{DDD;
    QMap<int, QVariant> data;
    data.insert(QwtLegendData::UserRole+3, pen().color());
    data.insert(QwtLegendData::TitleRole, title());
    if (duplicate && fileNumber>0)
        data.insert(QwtLegendData::UserRole+1, fileNumber);
    data.insert(QwtLegendData::UserRole+2, selected());
    data.insert(QwtLegendData::UserRole+4, fixed);
    return data;
}



bool Curve::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DDD;
    SelectedPoint p = closest(pos, distanceX, distanceY);

    //no closest point for this pos
    if (!p.valid()) return false;

    if (distanceX && distanceY) {
        if ((*distanceX)*(*distanceX)+(*distanceY)*(*distanceY) < 25) {
            if (point) *point = p;
            return true;
        }
    }
    return false;
}

void Curve::moveLeft(int count)
{DDD;
    if (selectedPoint.x >= count) {
        selectedPoint.x -= count;
        updateSelection(selectedPoint);
    }
}

void Curve::moveRight(int count)
{DDD;
    if (selectedPoint.x >=0 && selectedPoint.x < samplesCount()-count) {
        selectedPoint.x += count;
        updateSelection(selectedPoint);
    }
}

void Curve::moveUp(int count)
{DDD;
    if (m_plot->interactionMode != Enums::InteractionMode::DataInteraction) return;
    if (selectedPoint.x < 0 || selectedPoint.x >= samplesCount()) return;

    auto val = samplePoint(selectedPoint);
    auto range = m_plot->plotRange(yAxis());
    double y = val.y+qAbs(range.min-range.max)/100.0*count;

    if (channel->data()->setYValue(selectedPoint.x, y, selectedPoint.z)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection(selectedPoint);
    }
}

void Curve::moveDown(int count)
{DDD;
    if (m_plot->interactionMode != Enums::InteractionMode::DataInteraction) return;
    if (selectedPoint.x < 0 || selectedPoint.x >= samplesCount()) return;

    auto val = samplePoint(selectedPoint);
    auto range = m_plot->plotRange(yAxis());
    double y = val.y - qAbs(range.min-range.max)/100*count;

    if (channel->data()->setYValue(selectedPoint.x, y, selectedPoint.z)) {
        channel->setDataChanged(true);
        channel->descriptor()->setDataChanged(true);
        resetCashedData();

        updateSelection(selectedPoint);
    }
}

void Curve::fix()
{DDD;
    if (selectedPoint.x >= 0 && selectedPoint.x < samplesCount()) {
//        auto val = samplePoint(selectedPoint);

        PointLabel *label = findLabel(selectedPoint);

        if (!label) {
            label = new PointLabel(m_plot, this);
            label->setPoint(selectedPoint);
//            label->setOrigin(val);
            addLabel(label);
            label->attach(m_plot->impl());
        }
    }
}

void Curve::remove()
{DDD;
    m_plot->deleteSelectedCurve(this);
}

bool Curve::draggable() const
{
    return false;
}

void Curve::updateSelection(SelectedPoint point)
{DDD;
    updatePen();

    selectedPoint = point;

    if (!selected()) marker->setVisible(false);
    else {
        marker->setVisible(true);
        auto val = samplePoint(selectedPoint);

        marker->moveTo({val.x, qIsNaN(val.z) ? val.y : val.z});
    }
}
