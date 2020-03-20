#include "filterheaderview.h"

#include <QtWidgets>

#include "sortfiltermodel.h"

//FilterHeaderView::FilterHeaderView(Qt::Orientation orientation, QWidget *parent)
//    : QHeaderView(orientation, parent), _padding(4)
//{
//    setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//    setSortIndicatorShown(false);
//    connect(this, SIGNAL(sectionResized(int,int,int)), this, SLOT(adjustPositions()));

//    if (QTreeView *tree = qobject_cast<QTreeView *>(parent))
//        connect(tree->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustPositions()));
//}

//void FilterHeaderView::setFilterBoxes(int count)
//{
//    foreach (QLineEdit *e, _editors) {
//        e->deleteLater();
//    }
//    _editors.clear();

//    for (int i=0; i<count; ++i) {
//        QLineEdit *editor = new QLineEdit(this);
//        editor->setFrame(false);
//        editor->setClearButtonEnabled(false);
//        editor->setPlaceholderText(model()->headerData(i, Qt::Horizontal).toString());
//        connect(editor, &QLineEdit::textChanged, [this, i](const QString &text){
//            emit filterChanged(text, i);
//        });
//        _editors << editor;
//    }
//    adjustPositions();
//}

//void FilterHeaderView::clear()
//{
//    foreach (QLineEdit *edit, _editors) edit->clear();
//}

//void FilterHeaderView::adjustPositions()
//{
//    for (int index = 0; index < _editors.size(); ++index) {
//        QLineEdit *editor = _editors[index];
//        int height = editor->sizeHint().height();
//        editor->move(sectionPosition(index) - offset() + 2,
//                    height + (_padding / 2));
//        editor->resize(sectionSize(index), height);
//    }
//}

//QSize FilterHeaderView::sizeHint() const
//{
//    QSize size = QHeaderView::sizeHint();
//    if (!_editors.isEmpty()) {
//        int height = _editors[0]->sizeHint().height();
//        size.setHeight(size.height() + height + _padding);
//    }
//    return size;
//}


//void FilterHeaderView::updateGeometries()
//{
//    if (!_editors.isEmpty()) {
//        int height = _editors[0]->sizeHint().height();
//        setViewportMargins(0, 0, 0, height + _padding);
//    }
//    else
//        setViewportMargins(0, 0, 0, 0);
//    QHeaderView::updateGeometries();
//    adjustPositions();
//}

//******************************************************************************



EditMenuAction::EditMenuAction(int section, const QString &filter, const QString &sectionText, QObject *parent)
    : QWidgetAction(parent), section(section), filter(filter), sectionText(sectionText)
{

}

void EditMenuAction::onTextChanged(const QString &text)
{
    emit filterChanged(text,section);
}

QWidget *EditMenuAction::createWidget(QWidget *parent)
{
    QLineEdit *edit = new QLineEdit(parent);
    edit->setText(filter);
    edit->setClearButtonEnabled(true);
    edit->setPlaceholderText(sectionText);
    edit->setFrame(false);
    connect(edit,SIGNAL(textChanged(QString)),SLOT(onTextChanged(QString)));
    return edit;
}

FilteredHeaderView::FilteredHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
    setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setSortIndicatorShown(false);
}

void FilteredHeaderView::clear()
{
    for (int i=0; i<model()->columnCount(); ++i)
        emit filterChanged("",i);
}




void FilteredHeaderView::paintEvent(QPaintEvent *event)
{
    QHeaderView::paintEvent(event);
    QRect rect = event->rect();

    int padding = 2;
    int w = rect.height()+12;

    //определяем, какую секцию перерисовываем
    //если end=-1, это значит, что перерисовываем за пределами секций
    int end = -1;
    if (orientation() == Qt::Horizontal)
        end = visualIndexAt(rect.right());
    else
        end = visualIndexAt(rect.bottom());
    if (end != -1) return;

    //рисуем кнопку
    QPainter painter(this->viewport());
    painter.save();

    QStyleOptionButton option;
//    if (isEnabled())
//        option.state |= QStyle::State_Enabled;
//    option.state |= QStyle::State_Sunken;
    option.rect = QRect(rect.width()-w-padding, rect.y(), w, rect.height());
    option.features = QStyleOptionButton::Flat | QStyleOptionButton::HasMenu;
    option.iconSize = QSize(16,16);
    option.icon = QIcon(":/icons/funnel.png");

    style()->drawControl(QStyle::CE_PushButton, &option, &painter);
    painter.restore();
}


void FilteredHeaderView::mousePressEvent(QMouseEvent *event)
{
    QRect geom = geometry();
    int w = geom.height()+12;
    int padding = 2;

    //курсор должен быть внутри кнопки
    QRect rect(geom.width()-w-padding, geom.y(), w, geom.height());
    if (event->button() == Qt::LeftButton && rect.contains(event->pos(),true)) {
        if (SortFilterModel *m=dynamic_cast<SortFilterModel*>(model())) {
            QMenu menu;
            for (int i=0; i<m->columnCount(); ++i) {
                EditMenuAction *a = new EditMenuAction(i, m->filter(i),
                                                       m->headerData(i, Qt::Horizontal).toString(), &menu);
                connect(a,SIGNAL(filterChanged(QString,int)),this, SIGNAL(filterChanged(QString,int)));
                menu.addAction(a);
            }

            menu.exec(QCursor::pos());
        }

    }
    else QHeaderView::mousePressEvent(event);
}
