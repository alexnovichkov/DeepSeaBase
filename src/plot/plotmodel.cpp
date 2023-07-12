#include "plotmodel.h"

#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include <QFont>

PlotModel::PlotModel(QObject *parent) : QAbstractTableModel(parent)
{DD;

}

bool PlotModel::isEmpty() const
{DD;
    return m_curves.isEmpty();
}

int PlotModel::size(int type) const
{DD;
    if (type==-1) return m_curves.size();
    return std::count_if(m_curves.cbegin(), m_curves.cend(),
                         [type](Curve *c){return int(c->channel->type()) == type;});
}

Curve *PlotModel::firstOf(CurvePredicate predicate)
{DD;
    const auto result = std::find_if(m_curves.cbegin(), m_curves.cend(), predicate);
    if (result != m_curves.cend()) return *result;

    return nullptr;
}

QList<Curve *> PlotModel::curves(CurvePredicate predicate) const
{DD;
    QList<Curve *> result;
    for (const auto &c: m_curves) {
        if (predicate(c)) result << c;
    }
    return result;
}

Curve *PlotModel::selectedCurve() const
{DD;
    for (const auto &c: m_curves) {
        if (c->selected()) return c;
    }
    return nullptr;
}

void PlotModel::clear(bool forceDeleteFixed)
{DD;
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
{DD;
    m_plotted.clear();
    for (auto curve: m_curves) {
        if (!curve->fixed)
            m_plotted.append({curve->channel->index(), m_leftCurves.contains(curve),
                                 curve->channel, curve->fileNumber});
    }
}

void PlotModel::updatePlottedIndexes(FileDescriptor *d, int fileIndex)
{DD;
    for (auto &c: m_plotted) {
        c.fileIndex = fileIndex;
        c.ch = d->channel(c.channelIndex);
    }
}

void PlotModel::cycleChannels(bool up)
{DD;
    for (auto &c: m_plotted) {
        if (!c.ch) continue;
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
        c.channelIndex = newIndex;
        c.ch = d->channel(newIndex); //can be nullptr
    }
}

void PlotModel::setYValuesPresentation(bool leftCurves, int presentation)
{DD;
    auto &list = leftCurves?this->m_leftCurves:m_rightCurves;
    for (auto &curve: list) curve->channel->data()->setYValuesPresentation(presentation);
}

void PlotModel::removeLabels()
{DD;
    for (Curve *c: qAsConst(m_curves))
        c->removeLabels();
}

void PlotModel::updateTitles()
{DD;
    for (Curve *curve: qAsConst(m_curves))
        curve->setTitle(curve->channel->legendName());
}

void PlotModel::resetHighlighting()
{DD;
    for(Curve *c: qAsConst(m_curves)) {
        c->setSelected(false, SelectedPoint());
    }
}

void PlotModel::setTemporaryCorrection(Channel *ch, double correctionValue, int correctionType)
{DD;
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
    return nullptr;
}

bool PlotModel::allCurvesFromSameDescriptor() const
{DD;
    if (m_curves.isEmpty()) return false;
    auto d = m_curves.first()->channel->descriptor();
    for (int i=1; i<m_curves.size(); ++i) {
        if (m_curves[i]->channel->descriptor() != d) return false;
    }
    return true;
}

void PlotModel::addCurve(Curve *curve, bool onLeft)
{DD;
    beginInsertRows(QModelIndex(), m_curves.size(), m_curves.size());
    m_curves.append(curve);
    if (onLeft) m_leftCurves.append(curve);
    else m_rightCurves.append(curve);
    endInsertRows();
    checkDuplicates(curve->title());
}

bool PlotModel::deleteCurve(Curve *curve, bool *removedFromLeft)
{DD;
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
{DD;
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
{DD;
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

//QMap<FileDescriptor *, QVector<int> > PlotModel::plottedDescriptors() const
//{DD;
//    QMap<FileDescriptor *, QVector<int> > result;

//    for (auto c: m_curves) {
//        result[c->channel->descriptor()].append(c->channel->index());
//    }

//    return result;
//}

QVector<FileDescriptor *> PlotModel::plottedDescriptors() const
{
    QVector<FileDescriptor *> result;
    for (auto c: m_curves) {
        if (!result.contains(c->channel->descriptor()))
            result << c->channel->descriptor();
    }
    return result;
}

QVector<int> PlotModel::plottedIndexesForDescriptor(FileDescriptor *d) const
{
    QVector<int> result;
    for (auto c: m_curves) {
        if (c->channel->descriptor() == d && !result.contains(c->channel->index()))
            result << c->channel->index();
    }
    std::sort(result.begin(), result.end());
    return result;
}

Descriptor::DataType PlotModel::curvesDataType() const
{DD;
    if (m_curves.isEmpty()) return Descriptor::Unknown;
    auto type = m_curves.constFirst()->channel->type();
    if (std::all_of(m_curves.cbegin(), m_curves.cend(), [type](Curve *c){return c->channel->type()==type;}))
        return type;
    return Descriptor::Unknown;
}

Curve *PlotModel::curve(int index, bool left) const
{DD;
    auto &list = left ? m_leftCurves : m_rightCurves;
    if (index > -1 && index < list.size()) return list.at(index);
    return nullptr;
}

Curve *PlotModel::curve(int index) const
{DD;
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
{DD;
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
                case PlotAxisColumn: return c->yAxis() == Enums::AxisType::atLeft?"Левая":"Правая";
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
                    if (c->selected()) {
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
{DD;
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return false;
}


QVariant PlotModel::headerData(int section, Qt::Orientation orientation, int role) const
{DD;
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
{DD;
    const int col = index.column();
    auto result = Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsEnabled;
    if (col == PlotCorrectionColumn) result |= Qt::ItemIsEditable;
    if (col == PlotTitleColumn) result |= Qt::ItemIsUserCheckable;
    return result;
}
