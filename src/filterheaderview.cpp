#include "filterheaderview.h"

#include <QtWidgets>

FilterHeaderView::FilterHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent), _padding(4)
{
    setStretchLastSection(true);

    setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setSortIndicatorShown(false);
    connect(this, SIGNAL(sectionResized(int,int,int)), this, SLOT(adjustPositions()));

    if (QTreeView *tree = qobject_cast<QTreeView *>(parent))
        connect(tree->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustPositions()));
}

void FilterHeaderView::setFilterBoxes(int count)
{
    foreach (QLineEdit *e, _editors) {
        e->deleteLater();
    }
    _editors.clear();

    for (int i=0; i<count; ++i) {
        QLineEdit *editor = new QLineEdit(this);
        editor->setFrame(false);
        editor->setClearButtonEnabled(true);
        editor->setPlaceholderText(model()->headerData(i, Qt::Horizontal).toString());
        connect(editor, &QLineEdit::textChanged, [this, i](const QString &text){
            emit filterChanged(text, i);
        });
        _editors << editor;
    }
    adjustPositions();
}

void FilterHeaderView::clear()
{
    foreach (QLineEdit *edit, _editors) edit->clear();
}

void FilterHeaderView::adjustPositions()
{
    for (int index = 0; index < _editors.size(); ++index) {
        QLineEdit *editor = _editors[index];
        int height = editor->sizeHint().height();
        editor->move(sectionPosition(index) - offset() + 2,
                    height + (_padding / 2));
        editor->resize(sectionSize(index), height);
    }
}

QSize FilterHeaderView::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();
    if (!_editors.isEmpty()) {
        int height = _editors[0]->sizeHint().height();
        size.setHeight(size.height() + height + _padding);
    }
    return size;
}


void FilterHeaderView::updateGeometries()
{
    if (!_editors.isEmpty()) {
        int height = _editors[0]->sizeHint().height();
        setViewportMargins(0, 0, 0, height + _padding);
    }
    else
        setViewportMargins(0, 0, 0, 0);
    QHeaderView::updateGeometries();
    adjustPositions();
}
