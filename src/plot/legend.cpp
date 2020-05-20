#include "legend.h"
#include <QtDebug>
#include "qwt_plot_item.h"
#include <QtWidgets>
#include "logging.h"

static void qwtRenderBackground( QPainter *painter, const QRectF &rect, const QWidget *widget )
{
    if ( widget->testAttribute( Qt::WA_StyledBackground ) )  {
        QStyleOption opt;
        opt.initFrom( widget );
        opt.rect = rect.toAlignedRect();

        widget->style()->drawPrimitive(QStyle::PE_Widget, &opt, painter, widget);
    }
    else  {
        const QBrush brush = widget->palette().brush( widget->backgroundRole() );
        painter->fillRect( rect, brush );
    }
}

CheckableLegend::CheckableLegend(QWidget *parent) : QwtAbstractLegend(parent)
{
    d_treeView = new LegendTreeView(this);
    d_model = new LegendModel(this);
    d_treeView->setModel(d_model);
    d_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    d_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    d_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( d_treeView );

    connect(d_treeView, SIGNAL(pressed(QModelIndex)), this, SLOT(handleClick(QModelIndex)));
}

CheckableLegend::~CheckableLegend()
{

}

void CheckableLegend::renderLegend(QPainter *painter, const QRectF &rect, bool fillBackground) const
{
    if (fillBackground ) {
        if (autoFillBackground() || testAttribute(Qt::WA_StyledBackground)) {
            qwtRenderBackground(painter, rect, d_treeView);
        }
    }

    QStyleOptionViewItem styleOption;
    styleOption.initFrom( this );
    styleOption.decorationAlignment = Qt::AlignCenter;

    const QAbstractItemDelegate *delegate = d_treeView->itemDelegate();

    painter->save();
    painter->translate( rect.topLeft() );

    for ( int row = 0; row < d_model->rowCount(); row++ ) {
        for (int col = 0; col < d_model->columnCount(); col++) {
            styleOption.rect = d_treeView->visualRect( d_model->index(row, col) );
            if (!styleOption.rect.isEmpty() )
                delegate->paint( painter, styleOption, d_model->index(row, col) );
        }
    }
    painter->restore();
}

bool CheckableLegend::isEmpty() const
{
    return d_model->rowCount() == 0;
}

int CheckableLegend::scrollExtent(Qt::Orientation) const
{
    return style()->pixelMetric( QStyle::PM_ScrollBarExtent );
}

void CheckableLegend::updateLegend(const QVariant &itemInfo, const QList<QwtLegendData> &data)
{
    QwtPlotItem *plotItem = qvariant_cast<QwtPlotItem *>(itemInfo);

    if (plotItem->rtti() == QwtPlotItem::Rtti_PlotItem)
        d_model->removeItem(plotItem);

    if (!data.isEmpty())  {
        if (!d_model->contains(plotItem))
            d_model->addItem(plotItem);

        updateItem(plotItem, data.constFirst());
    }

    d_treeView->updateGeometry();
}

void CheckableLegend::handleClick(const QModelIndex &i)
{DD;
    if (QApplication::mouseButtons() & Qt::RightButton) {
        emit markedForDelete(d_model->item(i.row()));
        return;
    }

    if (QApplication::mouseButtons() & Qt::MiddleButton) {
        emit markedToMove(d_model->item(i.row()));
        return;
    }

    if (i.column() == 1) {
        emit clicked(d_model->item(i.row()));
    }
    else if (i.column() == 2) {
        emit fixedChanged(d_model->item(i.row()));
//        d_model->setData(i, )
    }
}

void CheckableLegend::updateItem(QwtPlotItem *item, const QwtLegendData &data)
{
    //QwtLegendData::UserRole+1 - дополнительный идентификатор канала
    //QwtLegendData::UserRole+2 - индикатор того, что кривая выбрана
    //QwtLegendData::UserRole+3 - цвет кривой
    //QwtLegendData::UserRole+4 - индикатор того, что кривая фиксирована

    const QVariant titleValue = data.value( QwtLegendData::TitleRole );

    QString title;
    if (titleValue.canConvert<QString>() )
        title = titleValue.value<QString>();

    const QVariant iconValue = data.value(QwtLegendData::UserRole+3);

    QColor color;
    if ( iconValue.canConvert<QColor>() )
        color = iconValue.value<QColor>();

    if (data.hasRole(QwtLegendData::UserRole+1)) {
        title.append(QString(" [%1]").arg(data.value(QwtLegendData::UserRole+1).toInt()));
    }

    bool selected = data.value(QwtLegendData::UserRole+2).toBool();

    bool fixed = data.value(QwtLegendData::UserRole+4).toBool();

    d_model->update(item, title, color, item->isVisible(), selected, fixed);
}



LegendModel::LegendModel(QObject *parent) : QAbstractTableModel(parent)
{

}

LegendModel::~LegendModel()
{

}

bool LegendModel::contains(QwtPlotItem *item)
{
    for (const LegendItem &i: qAsConst(items)) {
        if (i.item == item) return true;
    }
    return false;
}

QwtPlotItem *LegendModel::item(int row)
{
    if (row < 0 || row >= items.size()) return 0;

    return items[row].item;
}

void LegendModel::addItem(QwtPlotItem *it)
{
    beginInsertRows(QModelIndex(), items.size(), items.size());
    LegendItem item;
    item.checked = it->isVisible();
    item.item = it;
    items.append(item);
    endInsertRows();
}

void LegendModel::removeItem(QwtPlotItem *plotItem)
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

void LegendModel::update(QwtPlotItem *it, const QString &text, const QColor &color,
                         bool checked, bool selected, bool fixed)
{
    int ind = -1;
    for (int index = 0; index < items.size(); ++index) {
        if (items[index].item == it) {
            ind = index;
            break;
        }
    }
    if (ind < 0 || ind >= items.size()) return;
    items[ind].text = text;
    items[ind].color = color;
    items[ind].checked = checked;
    items[ind].selected = selected;
    items[ind].fixed = fixed;
    emit dataChanged(index(ind,0), index(ind, 2));
}

int LegendModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return items.size();
}

int LegendModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant LegendModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=items.size()) return QVariant();

    LegendItem d = items.at(row);

    if (role == Qt::DisplayRole) {
        switch (column) {
            case 1: return d.text;
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
                return d.color;
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

bool LegendModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole) return false;
    if (!index.isValid()) return false;

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=items.size()) return false;
    if (column == 0) {
        items[row].checked = value.toInt() == int(Qt::Checked);
        items[row].item->setVisible(items[row].checked);
        emit dataChanged(index, index, QVector<int>()<<Qt::CheckStateRole);
        return true;
    }
    else if (column == 1) {
        if (items[row].fixed != value.toBool()) {
            items[row].fixed = value.toBool();
            emit dataChanged(index,index, QVector<int>()<<Qt::DecorationRole);
            return true;
        }
    }
    return false;
}

Qt::ItemFlags LegendModel::flags(const QModelIndex &index) const
{
    const int col = index.column();
    Qt::ItemFlags fl = QAbstractTableModel::flags(index);
    if (col==0) return fl | Qt::ItemIsUserCheckable;

    return fl;
}

LegendTreeView::LegendTreeView(QWidget *parent) : QTreeView(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle( NoFrame );
    viewport()->setBackgroundRole(QPalette::Window);
    viewport()->setAutoFillBackground( false );

    setRootIsDecorated( false );
    setHeaderHidden( true );
    setContentsMargins(0,0,0,0);
    setUniformRowHeights(true);
    header()->setStretchLastSection(false);
}

QSize LegendTreeView::minimumSizeHint() const
{
    return QSize( -1, -1 );
}

QSize LegendTreeView::sizeHint() const
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


