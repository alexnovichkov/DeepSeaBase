#include "checkablelegend.h"
#include "enums.h"
#include "curve.h"

QCPCheckableLegend::QCPCheckableLegend(QWidget *parent) : QObject(parent)
{
    m_treeView = new QCPLegendTreeView(parent);
    m_model = new QCPLegendModel(parent);
    m_treeView->setModel(m_model);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    connect(m_model, SIGNAL(visibilityChanged(Curve*, bool)), this, SIGNAL(visibilityChanged(Curve*, bool)));

    connect(m_treeView, &QTreeView::pressed, this, &QCPCheckableLegend::handleClick);
}

void QCPCheckableLegend::addItem(Curve *item, const LegendData &data)
{
    if (!m_model->contains(item)) m_model->addItem(item, data);
}

void QCPCheckableLegend::removeItem(Curve *item)
{
    m_model->removeItem(item);
}

void QCPCheckableLegend::updateItem(Curve *item, const LegendData &data)
{
    m_model->update(item, data);
}

void QCPCheckableLegend::handleClick(const QModelIndex &index)
{
    auto item = m_model->item(index.row());
    if (QApplication::mouseButtons() & Qt::RightButton) {
        QMenu menu;
        menu.addAction("Удалить", this, [this, item](){
            emit markedForDelete(item);
        });
        menu.addAction("Переместить на правую ось", this, [item, this](){
            emit markedToMove(item);
        });
        menu.exec(QCursor::pos());
    }
    else if (QApplication::mouseButtons() & Qt::MiddleButton)
        emit markedToMove(item);
    else if (index.column() == 1)
        emit clicked(item);
    else if (index.column() == 2)
        emit fixedChanged(item);
}

QCPLegendTreeView::QCPLegendTreeView(QWidget *parent) : QTreeView(parent)
{
//    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle( NoFrame );
    setBackgroundRole(QPalette::Base);
//    viewport()->setBackgroundRole(QPalette::Window);
//    viewport()->setAutoFillBackground( false );

    setRootIsDecorated( false );
    setHeaderHidden( true );
    setContentsMargins(0,0,0,0);
    setUniformRowHeights(true);
    header()->setStretchLastSection(false);
}

QSize QCPLegendTreeView::sizeHint() const
{
    int w = 0;
    int h = 0;

    for (int i=0; i<model()->columnCount(); ++i)
        w += sizeHintForColumn(i);
    if (model()->rowCount()>0) {
        int h1 = sizeHintForRow(0);
        h = h1 * model()->rowCount();
    }

    w += 10;
    return QSize( w, h );
}

QSize QCPLegendTreeView::minimumSizeHint() const
{
    int w = 0;
    int h = 0;

    for (int i=0; i<model()->columnCount(); ++i)
        w += sizeHintForColumn(i);
    if (model()->rowCount()>0) {
        int h1 = sizeHintForRow(0);
        h = h1 * model()->rowCount();
    }

    w += 10;
    return QSize( w, h );
}

QCPLegendModel::QCPLegendModel(QObject *parent) : QAbstractTableModel(parent)
{

}

bool QCPLegendModel::contains(Curve *item)
{
    for (const auto &i: qAsConst(items)) {
        if (i.item == item) return true;
    }
    return false;
}

Curve *QCPLegendModel::item(int row)
{
    if (row < 0 || row >= items.size()) return nullptr;

    return items.at(row).item;
}

void QCPLegendModel::addItem(Curve *it, const LegendData &data)
{
    beginInsertRows(QModelIndex(), items.size(), items.size());
    QCPLegendItem item;
    item.data = data;
    item.item = it;
    items.append(item);
    endInsertRows();
}

void QCPLegendModel::removeItem(Curve *plotItem)
{
    int ind = -1;
    for (int index = 0; index < items.size(); ++index) {
        if (items[index].item == plotItem) {
            ind = index;
            break;
        }
    }
    if (ind < 0 || ind >= items.size()) return;

    beginRemoveRows(QModelIndex(), ind, ind);
    items.removeAt(ind);
    endRemoveRows();
}

void QCPLegendModel::update(Curve *it, const LegendData &data)
{
    int ind = -1;
    for (int index = 0; index < items.size(); ++index) {
        if (items[index].item == it) {
            ind = index;
            break;
        }
    }
    if (ind < 0 || ind >= items.size()) return;
    items[ind].data = data;
    emit dataChanged(index(ind,0), index(ind, 2));
}

int QCPLegendModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return items.size();
}

int QCPLegendModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant QCPLegendModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=items.size()) return QVariant();

    auto d = items.at(row).data;

    if (role == Qt::DisplayRole) {
        switch (column) {
            case 1: {
                auto s = d.text;
                if (d.fileNumber > 0) s.append(QString(" [%1]").arg(d.fileNumber));
                return s;
            }
            default: return QVariant();
        }
    }
    else if (role == Qt::FontRole) {
        switch (column) {
            case 1: {
                QFont f = qApp->font();
                if (d.selected) f.setUnderline(true);
                return f;
            }
            default: return QVariant();
        }
    }
    else if (role == Qt::DecorationRole) {
        switch (column) {
            case 0: {
//                return d.color;
                return items.at(row).item->thumbnail();
            }
            case 2: {
                return d.fixed?QIcon(":/icons/pinned.png"):QIcon(":/icons/pin.png");
            }
            default: return QVariant();

        }
    }
    else if (role == Qt::CheckStateRole) {
        switch (column) {
            case 0: return d.checked ? Qt::Checked : Qt::Unchecked;
            default: return QVariant();
        }
    }
    return QVariant();
}

bool QCPLegendModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole) return false;
    if (!index.isValid()) return false;

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=items.size()) return false;
    if (column == 0) {
        //скрываем все остальные спектрограммы
        if (items[row].item->type == Curve::Type::Spectrogram && value.toInt() == int(Qt::Checked)) {
            for (int i=0; i<rowCount(); ++i) {
                if (i == row) continue;
                setData(QAbstractTableModel::index(i, column), Qt::Unchecked, Qt::CheckStateRole);
            }
        }

        items[row].data.checked = value.toInt() == int(Qt::Checked);
        emit visibilityChanged(items[row].item, items[row].data.checked);
        emit dataChanged(index, index, QVector<int>()<<Qt::CheckStateRole);

        return true;
    }
    if (column == 1) {
        if (items[row].data.fixed != value.toBool()) {
            items[row].data.fixed = value.toBool();
            emit dataChanged(index,index, QVector<int>()<<Qt::DecorationRole);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags QCPLegendModel::flags(const QModelIndex &index) const
{
    const int col = index.column();
    Qt::ItemFlags fl = QAbstractTableModel::flags(index);
    if (col==0) return fl | Qt::ItemIsUserCheckable;

    return fl;
}
