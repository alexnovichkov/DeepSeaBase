#include "descriptorpropertiesdialog.h"

#include <QtWidgets>
#include "fileformats/filedescriptor.h"
#include "logging.h"

class FilesItemDelegate : public QStyledItemDelegate
{


    // QAbstractItemDelegate interface
public:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

static struct Properties
{
    QLineEdit* labels[11];
} propertiesLabels;

void FilesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{DD;
    if (!painter) return;

    painter->save();

    if (index.isValid()) {
        if (!index.parent().isValid()) {
            QFileInfo fi(index.data(Qt::DisplayRole).toString());
            QString fileName = fi.fileName();
            QString dirName = fi.canonicalPath();

            if (fileName.isEmpty()) {
                QStyledItemDelegate::paint(painter,option,index);
                painter->restore();
                return;
            }


            QStyleOptionViewItem optionV4 = option;
            initStyleOption(&optionV4, index);

            QStyle *style = optionV4.widget? optionV4.widget->style() : QApplication::style();

            QTextDocument doc;
            QString html = "<b>"+fileName+"</b><br>"+dirName;
            doc.setHtml(html);

            /// Painting item without text
            optionV4.text = QString();
            style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

            QAbstractTextDocumentLayout::PaintContext ctx;

            // Highlighting text if item is selected
            //if (optionV4.state & QStyle::State_Selected)
            //    ctx.palette.setColor(QPalette::Text, optionV4.palette.color(QPalette::Active, QPalette::HighlightedText));

            QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
            painter->save();
            painter->translate(textRect.topLeft());
            painter->setClipRect(textRect.translated(-textRect.topLeft()));
            doc.documentLayout()->draw(painter, ctx);

            painter->restore();
        }
        else QStyledItemDelegate::paint(painter,option,index);
    }
    else QStyledItemDelegate::paint(painter,option,index);

    painter->restore();
}

QSize FilesItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{DD;
    if (!index.isValid()) return option.rect.size();
    //QString info = index.data(Qt::DisplayRole).toString();
    return QSize(option.rect.width(), option.fontMetrics.height() * 2);
}

DescriptorPropertiesDialog::DescriptorPropertiesDialog(const QList<FileDescriptor *> &records, QWidget *parent)
    : QDialog(parent), records(records)
{DD;
    setWindowTitle("Свойства записей");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
//    prevButton = buttonBox->addButton("Предыдущий", QDialogButtonBox::ActionRole);
//    connect(prevButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::prev);
//    nextButton = buttonBox->addButton("Следующий", QDialogButtonBox::ActionRole);
//    connect(nextButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::next);
//    applyToCurrButton = buttonBox->addButton("Применить к текущему", QDialogButtonBox::ActionRole);
//    connect(applyToCurrButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToCurrent);
//    applyToAllButton = buttonBox->addButton("Применить ко всем", QDialogButtonBox::ActionRole);
//    connect(applyToAllButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToAll);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->addWidget(buttonBox);

    files = new QTreeWidget(this);
    files->setAlternatingRowColors(true);
    files->setRootIsDecorated(false);
    files->setColumnCount(1);
    files->setHeaderLabel("Записи");
    files->setSelectionMode(QAbstractItemView::ExtendedSelection);
    //files->setColumnWidth(0,200);
    files->setItemDelegateForColumn(0, new FilesItemDelegate());
    connect(files, &QTreeWidget::currentItemChanged, this, &DescriptorPropertiesDialog::currentFileChanged);
    fillFiles();
    splitter->addWidget(files);

    QTabWidget *tab = new QTabWidget(this);
    QWidget *properties = new QWidget(this);
    QFormLayout *propertiesFL = new QFormLayout;
    static QStringList propertiesNames {"Файл", "Размер, байт", "Количество каналов",
                                        "Дата и время записи", "Дата и время создания файла",
                                        "GUID", "Создан в", "Файл-источник", "GUID файла-источника",
                                        "Дата и время записи файла-источника", "Каналы источника"};
    for (int i=0; i<propertiesNames.size(); ++i) {
        propertiesLabels.labels[i] = new QLineEdit(this);
        propertiesLabels.labels[i]->setReadOnly(true);
        propertiesLabels.labels[i]->setFrame(false);
        propertiesFL->addRow(propertiesNames[i], propertiesLabels.labels[i]);
    }
    properties->setLayout(propertiesFL);

    QScrollArea *scroll1 = new QScrollArea(this);
    scroll1->setWidget(properties);
    scroll1->setWidgetResizable(true);
    scroll1->setFrameShape(QFrame::NoFrame);
    tab->addTab(scroll1, "Свойства");

    QWidget *descriptions = new QWidget(this);
    tab->addTab(descriptions, "Описатели");
    QToolBar *toolBar = new QToolBar(this);

    QAction *addAct = toolBar->addAction("Добавить", this, &DescriptorPropertiesDialog::addProperty);
    addAct->setIcon(QIcon(":/icons/list-add.png"));
    QAction *removeAct = toolBar->addAction("Удалить", this, &DescriptorPropertiesDialog::removeProperty);
    removeAct->setEnabled(false);
    removeAct->setIcon(QIcon(":/icons/list-remove.png"));

    descriptionsTable = new QTableWidget(this);
    descriptionsTable->setColumnCount(1);
    descriptionsTable->setHorizontalHeaderLabels({"Параметр", "Значение"});
    descriptionsTable->horizontalHeader()->setStretchLastSection(true);
    connect(descriptionsTable, &QTableWidget::cellChanged, this, &DescriptorPropertiesDialog::cellChanged);
    connect(descriptionsTable, &QTableWidget::itemSelectionChanged, [=](){
        removeAct->setDisabled(descriptionsTable->selectedItems().isEmpty());
    });

    QVBoxLayout *descriptionsL = new QVBoxLayout;
    descriptionsL->setContentsMargins(0,0,0,0);
    descriptionsL->addWidget(toolBar);
    descriptionsL->addWidget(descriptionsTable);
    descriptions->setLayout(descriptionsL);

    splitter->addWidget(tab);

    setLayout(l);
    updateState();
    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);
}

//void DescriptorPropertiesDialog::accept()
//{

//}

void DescriptorPropertiesDialog::fillFiles()
{DD;
    for(const auto f: qAsConst(records)) {
        new QTreeWidgetItem(files, {f->fileName()});
    }
}

void DescriptorPropertiesDialog::prev()
{DD;
    if (current > 0) {
        current--;
        updateState();
    }
}

void DescriptorPropertiesDialog::next()
{DD;
    if (current < records.size()-1) {
        current++;
        updateState();
    }
}

void DescriptorPropertiesDialog::applyToCurrent()
{DD;

}

void DescriptorPropertiesDialog::applyToAll()
{DD;

}

void DescriptorPropertiesDialog::updateState()
{DD;
//    prevButton->setEnabled(current > 0);
//    nextButton->setEnabled(current<records.size()-1);
    //    file->setText(records.at(current)->fileName());
}

void DescriptorPropertiesDialog::currentFileChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous)
{DD;
    Q_UNUSED(previous);
//    static QStringList propertiesNames {"Файл", "Размер, байт", "Количество каналов",
//                                        "Дата и время записи", "Дата и время создания файла",
//                                        "GUID", "Создан в", "Файл-источник", "GUID файла-источника",
//                                        "Дата и время записи файла-источника", "Каналы источника"};
    if (cur) {
        current = files->indexOfTopLevelItem(cur);
        QString s;
        FileDescriptor *d = records.at(current);
        propertiesLabels.labels[0]->setText(d->fileName());
        propertiesLabels.labels[1]->setText(QString::number(d->fileSize()));
        propertiesLabels.labels[2]->setText(QString::number(d->channelsCount()));
        QDateTime dt = d->dataDescription().dateTime("dateTime");
        if (dt.isValid()) s = dt.toString("d MMMM yyyy, hh:mm:ss");
        else s.clear();
        propertiesLabels.labels[3]->setText(s);
        dt = d->dataDescription().dateTime("fileCreationTime");
        if (dt.isValid()) s = dt.toString("d MMMM yyyy, hh:mm:ss");
        else s.clear();
        propertiesLabels.labels[4]->setText(s);
        propertiesLabels.labels[5]->setText(d->dataDescription().get("guid").toString());
        propertiesLabels.labels[6]->setText(d->dataDescription().get("createdBy").toString());
        propertiesLabels.labels[7]->setText(d->dataDescription().get("source.file").toString());
        propertiesLabels.labels[8]->setText(d->dataDescription().get("source.guid").toString());
        dt = d->dataDescription().dateTime("source.dateTime");
        if (dt.isValid()) s = dt.toString("d MMMM yyyy, hh:mm:ss");
        else s.clear();
        propertiesLabels.labels[9]->setText(s);
        s = d->dataDescription().get("source.channels").toString();
        if (s.isEmpty()) s = "все";
        propertiesLabels.labels[10]->setText(s);

        auto data = d->dataDescription().filter("description");
        data.insert("legend",d->legend());
        descriptionsTable->clearContents();
        descriptionsTable->setRowCount(data.size());
        int i=0;
        for (const auto [key, val]: asKeyValueRange(data)) {
            if (auto item = descriptionsTable->verticalHeaderItem(i))
                item->setText(key);
            else
                descriptionsTable->setVerticalHeaderItem(i, new QTableWidgetItem(key));
            if (auto item = descriptionsTable->item(i,0))
                item->setText(val.toString());
            else
                descriptionsTable->setItem(i,0, new QTableWidgetItem(val.toString()));
            i++;
        }
    }
    else {
        current = -1;
        for (auto l: qAsConst(propertiesLabels.labels)) l->clear();

        descriptionsTable->clearContents();
    }
}

void DescriptorPropertiesDialog::cellChanged(int row, int column)
{DD;
    auto item = descriptionsTable->item(row, column);
    if (!item || current < 0) return;

    FileDescriptor *d = records.at(current);
    QString key = descriptionsTable->verticalHeaderItem(row)->text();
    QString val = descriptionsTable->item(row, 0)->text();

    if (key != "legend") key.prepend("description.");
    QVariant old = d->dataDescription().get(key);

    if (old.toString() == item->text()) return;

    d->dataDescription().put(key, item->text());
    d->setChanged(true);
}

void DescriptorPropertiesDialog::addProperty()
{DD;
    int row = descriptionsTable->rowCount();

    QString name = QInputDialog::getText(this, "Новое свойство", "Введите название свойства");
    if (!name.isEmpty()) {
        descriptionsTable->setRowCount(row+1);
        descriptionsTable->setVerticalHeaderItem(row, new QTableWidgetItem());
        descriptionsTable->setItem(row, 0, new QTableWidgetItem());
        descriptionsTable->verticalHeaderItem(row)->setText(name);
    }
}

void DescriptorPropertiesDialog::removeProperty()
{DD;
    const auto selected = descriptionsTable->selectedItems();
    if (!selected.isEmpty()) {
        QSet<int> rows;
        for (auto s: selected) rows << s->row();

        if (QMessageBox::question(this, "Удаление свойств", "Удалить эти свойства?")==QMessageBox::Yes) {
            QList<int> r = rows.toList();
            std::sort(r.begin(), r.end());
            FileDescriptor *d = records.at(current);
            for (int i=r.size()-1; i>=0; --i) {
                QString key = descriptionsTable->verticalHeaderItem(r.at(i))->text();
                QString val = descriptionsTable->item(r.at(i), 0)->text();
                if (key != "legend") key.prepend("description.");
                descriptionsTable->removeRow(r.at(i));
                d->dataDescription().data.remove(key);
            }
            d->setChanged(true);
        }
    }

}
