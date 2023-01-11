#include "curve.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "dataholder.h"
#include "qcppointmarker.h"
#include "plot.h"
#include "checkablelegend.h"

QString Curve::markerShapeDescription(Curve::MarkerShape shape)
{
    switch (shape) {
        case MarkerShape::NoMarker: return "Без маркера";
        case MarkerShape::Dot: return "Точка";
        case MarkerShape::Cross: return "Крест";
        case MarkerShape::Plus: return "Плюс";
        case MarkerShape::Circle: return "Окружность";
        case MarkerShape::Disc: return "Диск";
        case MarkerShape::Square: return "Квадрат";
        case MarkerShape::Diamond: return "Ромб";
        case MarkerShape::Star: return "Звезда";
        case MarkerShape::Triangle: return "Треугольник";
        case MarkerShape::TriangleInverted: return "Перевернутый треугольник";
        case MarkerShape::CrossSquare: return "Квадрат с крестом";
        case MarkerShape::PlusSquare: return "Квадрат с плюсом";
        case MarkerShape::CrossCircle: return "Окружность с крестом";
        case MarkerShape::PlusCircle: return "Окружность с плюсом";
        case MarkerShape::Peace: return "Пацифик";
    }
    return "";
}

Curve::Curve(Channel *channel)
{DD;
    this->channel = channel;
    this->duplicate = false;
}

Curve::~Curve()
{DD;
    //maybe clear data that is over 1000000 samples
    if (channel) {
        channel->maybeClearData();
    }
}

void Curve::attachTo(Plot *plot)
{DD;
    m_plot = plot;
    m_plot->addSelectable(this);
    m_pointMarker = new PointLabel(plot, this);
    m_pointMarker->setMode(PointLabel::Mode::XValue);
    m_pointMarker->setVisible(false);
}

void Curve::detachFrom(Plot *plot)
{
    //detach labels
    for(PointLabel *l: labels)
        l->detachFrom(plot);

    //detach marker
    if (m_pointMarker)
        m_pointMarker->detachFrom(plot);
    plot->removeSelectable(this);
}

void Curve::setMarkerShape(Curve::MarkerShape markerShape)
{
    if (markerShape != m_markerShape) {
        m_markerShape = markerShape;
        updateScatter();
    }
}

void Curve::setMarkerSize(int markerSize)
{
    if (markerSize > 0 && m_markerSize != markerSize) {
        m_markerSize = markerSize;
        updateScatter();
    }
}

void Curve::addLabel(PointLabel *label)
{DD;
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{DD;
    if (labels.contains(label)) {
        labels.removeOne(label);
        label->detachFrom(m_plot);
        delete label;
    }
}

void Curve::removeLabels()
{DD;
    for (PointLabel *label: labels) {
        label->detachFrom(m_plot);
    }
    labels.clear();
}

PointLabel *Curve::findLabel(const QPoint &pos)
{DD;
    for (PointLabel *l: labels)
        if (l->underMouse(pos, nullptr, nullptr, nullptr))
            return l;

    return nullptr;
}

PointLabel *Curve::findLabel(SelectedPoint point)
{DD;
    for (PointLabel *l: labels)
        if (l->point() == point)
            return l;

    return nullptr;
}

void Curve::moveToPos(QPoint pos, QPoint startPos)
{DD;
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
{DD;
    return channel->data()->yMin(-1);
}

double Curve::yMax() const
{DD;
    return channel->data()->yMax(-1);
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

void Curve::updateLabels()
{
    for (auto label: labels) label->updateLabel();
}

void Curve::setVisible(bool visible)
{DD;
    for (PointLabel *label: labels) {
        label->setVisible(visible);
    }
    if (m_pointMarker) m_pointMarker->setVisible(visible);
}

void Curve::evaluateScale(int &from, int &to, double startX, double endX) const
{DD;
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
{DD;
    fixed = !fixed;
}

LegendData Curve::commonLegendData() const
{DD;
    LegendData data;

    data.color = pen().color();
    data.text = title();
    if (duplicate && fileNumber > 0)
        data.fileNumber = fileNumber;
    data.selected = selected();
    data.fixed = fixed;
    //data.checked = isVisible();

    return data;
}



bool Curve::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DD;
    if (!isVisible()) return false;
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
{DD;
    if (selectedPoint.x >= count) {
        selectedPoint.x -= count;
        updateSelection(selectedPoint);
    }
    m_plot->replot();
}

void Curve::moveRight(int count)
{DD;
    if (selectedPoint.x >=0 && selectedPoint.x < samplesCount()-count) {
        selectedPoint.x += count;
        updateSelection(selectedPoint);
    }
    m_plot->replot();
}

void Curve::moveUp(int count)
{DD;
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
{DD;
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
{DD;
    if (selectedPoint.x >= 0 && selectedPoint.x < samplesCount()) {
        if (m_pointMarker) m_pointMarker->setVisible(false);

        PointLabel *label = findLabel(selectedPoint);
        if (!label) {
            label = new PointLabel(m_plot, this);
            label->setPoint(selectedPoint);
            label->setVisible(true);
            addLabel(label);
            m_plot->replot();
        }
    }
}

void Curve::remove()
{DD;
    m_plot->deleteSelectedCurve(this);
}

bool Curve::draggable() const
{
    return false;
}

void Curve::updateSelection(SelectedPoint point)
{DD;
    updatePen();

    selectedPoint = point;
    if (m_pointMarker) {
        m_pointMarker->setPoint(selectedPoint);
        m_pointMarker->setVisible(selected());
    }
    m_plot->replot();
}
