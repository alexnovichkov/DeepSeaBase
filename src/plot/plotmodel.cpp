#include "plotmodel.h"

#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include <QFont>

PlotModel::PlotModel(QObject *parent) : QAbstractTableModel(parent)
{

}

bool PlotModel::isEmpty() const
{
    return m_curves.isEmpty();
}

int PlotModel::size(int type) const
{
    if (type==-1) return m_curves.size();
    return std::count_if(m_curves.cbegin(), m_curves.cend(),
                         [type](Curve *c){return int(c->channel->type()) == type;});
}

Curve *PlotModel::firstOf(CurvePredicate predicate)
{
    const auto result = std::find_if(m_curves.cbegin(), m_curves.cend(), predicate);
    if (result != m_curves.cend()) return *result;

    return nullptr;
}

QList<Curve *> PlotModel::curves(CurvePredicate predicate) const
{
    QList<Curve *> result;
    for (const auto &c: m_curves) {
        if (predicate(c)) result << c;
    }
    return result;
}

void PlotModel::clear(bool forceDeleteFixed)
{
    beginResetModel();
    for (int i=m_curves.size()-1; i>=0; --i) {
        Curve *c = m_curves[i];
        if (!c->fixed || forceDeleteFixed) {
            deleteCurve(c, nullptr);
        }
    }
    endResetModel();
}

void PlotModel::updatePlottedIndexes()
{
    m_plotted.clear();
    if (!m_curves.isEmpty()) {
        auto d = m_curves.first()->channel->descriptor();
        for (const auto c : m_leftCurves) {
            if (c->channel->descriptor()==d) m_plotted.append({c->channel->index(), true});
            else {
                m_plotted.clear();
                break;
            }
        }
        for (const auto c : m_rightCurves) {
            if (c->channel->descriptor()==d) m_plotted.append({c->channel->index(), false});
            else {
                m_plotted.clear();
                break;
            }
        }
    }
    std::sort(m_plotted.begin(), m_plotted.end(), [](const PlottedIndex &f, const PlottedIndex &s){
        return f.index < s.index;
    });
}

void PlotModel::updateCycled()
{
    m_cycled.clear();
    for (auto curve: m_curves) {
        if (!curve->fixed)
            m_cycled.append({curve->channel, m_leftCurves.contains(curve), curve->fileNumber});
    }
}

void PlotModel::cycleChannels(bool up)
{
    for (Cycled &c: m_cycled) {
        auto d = c.ch->descriptor();
        const int index = c.ch->index();
        int newIndex = index;
        if (up) {
            if (index == 0) newIndex = d->channelsCount()-1;
            else newIndex = index-1;
        }
        else {
            if (index == d->channelsCount()-1) newIndex = 0;
            else newIndex = index+1;
        }
        c.ch = d->channel(newIndex);
    }
}

void PlotModel::setYValuesPresentation(bool leftCurves, int presentation)
{
    auto &list = leftCurves?this->m_leftCurves:m_rightCurves;
    for (auto &curve: list) curve->channel->data()->setYValuesPresentation(presentation);
}

void PlotModel::removeLabels()
{
    for (Curve *c: qAsConst(m_curves))
        c->removeLabels();
}

void PlotModel::updateTitles()
{
    for (Curve *curve: qAsConst(m_curves))
        curve->setTitle(curve->channel->legendName());
}

void PlotModel::resetHighlighting()
{
    for(Curve *c: qAsConst(m_curves)) {
        c->resetHighlighting();
    }
}

void PlotModel::setTemporaryCorrection(Channel *ch, double correctionValue, int correctionType)
{
    ch->data()->setTemporaryCorrection(correctionValue, correctionType);

    for (int i=0; i<m_curves.size(); ++i) {
        if (m_curves.at(i)->channel == ch) {
            emit dataChanged(index(i, PlotCorrectionColumn), index(i, PlotCorrectionColumn));
        }
    }
}

Curve * PlotModel::plotted(Channel *channel) const
{DD;
    for (Curve *curve: qAsConst(m_curves)) {
        if (curve->channel == channel) return curve;
    }
    return 0;
}

void PlotModel::addCurve(Curve *curve, bool onLeft)
{
    beginInsertRows(QModelIndex(), m_curves.size(), m_curves.size());
    m_curves.append(curve);
    if (onLeft) m_leftCurves.append(curve);
    else m_rightCurves.append(curve);
    endInsertRows();
    checkDuplicates(curve->title());
}

bool PlotModel::deleteCurve(Curve *curve, bool *removedFromLeft)
{
    curve->channel->setPlotted(false);

    const auto index = m_curves.indexOf(curve);
    beginRemoveRows(QModelIndex(), index, index);
    int removed = m_leftCurves.removeAll(curve);
    if (removed > 0 && removedFromLeft) {
        *removedFromLeft = true;
    }

    removed = m_rightCurves.removeAll(curve);
    if (removed > 0 && removedFromLeft) {
        *removedFromLeft = false;
    }

    removed = m_curves.removeAll(curve);

    QString title = curve->title();
    checkDuplicates(title);
    endRemoveRows();
    return removed > 0;
}

bool PlotModel::moveToOtherAxis(Curve *curve)
{
    if (m_leftCurves.contains(curve)) {
        m_leftCurves.removeAll(curve);
        m_rightCurves.append(curve);
        return true;
    }
    else if (m_rightCurves.contains(curve)) {
        m_rightCurves.removeAll(curve);
        m_leftCurves.append(curve);
        return true;
    }
    return false;
}

void PlotModel::checkDuplicates(const QString name)
{
    Curve *c1 = nullptr;
    int found = 0;
    for (auto c: qAsConst(m_curves)) {
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

QVector<Channel *> PlotModel::plottedChannels() const
{DD;
    QVector<Channel*> result;
    result.reserve(m_curves.size());
    for (auto c: m_curves) result << c->channel;
    return result;
}

QVector<FileDescriptor *> PlotModel::plottedDescriptors() const
{
    QVector<FileDescriptor*> result;

    for (auto c: m_curves) {
        if (auto d = c->channel->descriptor(); !result.contains(d))
            result.append(d);
    }

    return result;
}

Descriptor::DataType PlotModel::curvesDataType() const
{
    if (m_curves.isEmpty()) return Descriptor::Unknown;
    auto type = m_curves.constFirst()->channel->type();
    if (std::all_of(m_curves.cbegin(), m_curves.cend(), [type](Curve *c){return c->channel->type()==type;}))
        return type;
    return Descriptor::Unknown;
}

Curve *PlotModel::curve(int index, bool left) const
{
    auto &list = left ? m_leftCurves : m_rightCurves;
    if (index > -1 && index < list.size()) return list.at(index);
    return nullptr;
}

Curve *PlotModel::curve(int index) const
{
    if (index > -1 && index < m_curves.size()) return m_curves.at(index);
    return nullptr;
}


int PlotModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_curves.size();
}

int PlotModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return PlotColumnsCount;
}

QVariant PlotModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const auto row = index.row();
    const auto col = index.column();
    const auto &c = m_curves.at(row);

    switch (role) {
        case Qt::DisplayRole: {
            switch (col) {
                case PlotTitleColumn: {
                    QString s = c->duplicate ? QString("%1 [%2]").arg(c->title()).arg(c->fileNumber) :c->title();
//                    if (QString corr = c->channel->correction(); !corr.isEmpty())
//                        s.append(corr);
                    return s;
                }
                case PlotAxisColumn: return c->yAxis()==QwtAxis::YLeft?"Левая":"Правая";
                case PlotCorrectionColumn:
                    return c->channel->data()->hasCorrection()
                            ? c->channel->data()->correctionString()
                            : "";
                default: break;
            }
            break;
        }
        case Qt::BackgroundRole: {
            switch (col) {
                case PlotColorColumn: return c->pen().color();
                default: break;
            }
            break;
        }
        case Qt::FontRole: {
            switch (col) {
                case PlotTitleColumn: {
                    if (c->highlighted) {
                        QFont font;
                        font.setBold(true);
                        return font;
                    }
                }
                default: break;
            }

            break;
        }
        case Qt::CheckStateRole: {

            break;
        }
    }
    return QVariant();
}

bool PlotModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}


QVariant PlotModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case PlotColorColumn: return "";
                case PlotTitleColumn: return "Канал";
                case PlotCorrectionColumn: return "Поправка";
                case PlotAxisColumn: return "Ось Y";
                default: break;
            }
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags PlotModel::flags(const QModelIndex &index) const
{
    const int col = index.column();
    auto result = Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsEnabled;
    if (col == PlotCorrectionColumn) result |= Qt::ItemIsEditable;
    if (col == PlotTitleColumn) result |= Qt::ItemIsUserCheckable;
    return result;
}
