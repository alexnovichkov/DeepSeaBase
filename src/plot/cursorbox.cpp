#include "cursorbox.h"

#include "plot.h"
#include "plotmodel.h"
#include "cursors.h"
#include "cursor.h"
#include "curve.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include <QwtText>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>

CursorBox::CursorBox(Cursors *cursors, Plot *parent) : QTreeWidget(parent),
    cursors{cursors}, plot{parent}
{DDD;
    setWindowFlag(Qt::Tool);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(cursors, SIGNAL(cursorsChanged()), this, SLOT(updateLayout()));
    connect(cursors,SIGNAL(cursorPositionChanged()), this, SLOT(updateContents()));
    setContentsMargins(0,0,0,0);
    setFrameShape(QFrame::NoFrame);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectColumns);
    setAlternatingRowColors(true);

    auto f = font();
    f.setPointSize(f.pointSize()-1);
    setFont(f);
    header()->setFont(f);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setSectionsClickable(true);
    header()->setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));

    copyAct = new QAction("Копировать", this);
    connect(copyAct, &QAction::triggered, this, &CursorBox::copy);
    addAction(copyAct);
}

void CursorBox::update()
{DDD;
    updateContents();
    updateLayout();
}

void CursorBox::updateContents()
{DDD;
    setHeaderLabels(cursors->dataHeader());
    auto size = plot->model()->curves().size();
    for (int i=0; i<size; ++i) {
        auto data = cursors->data(i);
        for (int j=0; j<data.size();++j) {
            topLevelItem(i)->setText(j, data.at(j));
        }
    }
}

void CursorBox::updateLayout()
{DDD;
    clear();

    auto size = plot->model()->curves().size();

    setColumnCount(cursors->dataCount()+1);
    setHeaderLabels(cursors->dataHeader());

    QList<QTreeWidgetItem*> list;
    for (int i=0; i<size; ++i) {
        auto item = new QTreeWidgetItem(cursors->data(i));
        item->setForeground(0, plot->model()->curve(i)->pen().color());
        list << item;
    }

    addTopLevelItems(list);
    int w = 0;
    for (int i=0; i<columnCount(); ++i) w += columnWidth(i);
    int h=200;
    resize(w+20,h);
}

void CursorBox::copy()
{DDD;
    QStringList list;
    QStringList values;
    for (int i=0; i<columnCount(); ++i) values << headerItem()->text(i);
    list << values.join('\t');

    for(int i=0; i<topLevelItemCount(); ++i) {
        values.clear();
        for (int j=0; j<columnCount(); ++j) {
            values.append(topLevelItem(i)->text(j));
        }
        for (int j=1; j<values.size(); ++j) {
            values[j].replace(".",",");
        }
        list << values.join('\t');

    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(list.join("\n"));
}



void CursorBox::closeEvent(QCloseEvent *event)
{DDD;
    emit closeRequested();
    QTreeWidget::closeEvent(event);
}

