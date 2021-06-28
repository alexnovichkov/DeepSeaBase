#include "descriptorpropertiesdialog.h"

#include <QtWidgets>
#include "fileformats/filedescriptor.h"

class FilesItemDelegate : public QStyledItemDelegate
{


    // QAbstractItemDelegate interface
public:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

static struct Properties
{
    QLabel* labels[11];
} propertiesLabels;

void FilesItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
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
{
    if (!index.isValid()) return option.rect.size();
    //QString info = index.data(Qt::DisplayRole).toString();
    return QSize(option.rect.width(), option.fontMetrics.height() * 2);
}

DescriptorPropertiesDialog::DescriptorPropertiesDialog(const QList<FileDescriptor *> &records, QWidget *parent)
    : QDialog(parent), records(records)
{
    setWindowTitle("Свойства записей");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
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
    tab->addTab(properties, "Свойства");
    propertiesFL = new QFormLayout;
    static QStringList propertiesNames {"Файл", "Размер, байт", "Количество каналов",
                                        "Дата и время записи", "Дата и время создания файла",
                                        "GUID", "Создан в", "Файл-источник", "GUID файла-источника",
                                        "Дата и время записи файла-источника", "Каналы источника"};
    for (int i=0; i<propertiesNames.size(); ++i) {
        propertiesLabels.labels[i] = new QLabel(this);
        propertiesFL->addRow(propertiesNames[i], propertiesLabels.labels[i]);
    }
    properties->setLayout(propertiesFL);

    QWidget *descriptions = new QWidget(this);
    tab->addTab(descriptions, "Описатели");

    splitter->addWidget(tab);

//    file = new QLabel(this);

    //QGridLayout *grid = new QGridLayout;
//    grid->addWidget(new QLabel("Файл", this), 0, 0, 1, 1, Qt::AlignRight);
//    grid->addWidget(file, 0, 1, 1, 2);
//    grid->addWidget(new QLabel("Свойство", this), 1,1);
//    grid->addWidget(new QLabel("Значение", this), 1,2);
//    for (int i=0; i<6; ++i) {
//        DescriptorProperty p;
//        p.edit = new QLineEdit(this);
//        p.checked = new QCheckBox(this);
//        p.property = new QComboBox(this);
//        properties << p;
//        grid->addWidget(p.checked, i+2, 0);
//        grid->addWidget(p.property, i+2, 1);
//        grid->addWidget(p.edit, i+2, 2);
//    }
    //grid->addWidget(buttonBox, 8, 0, 1, 3, Qt::AlignRight);

//    file->setText(records.at(current)->fileName());

    setLayout(l);
    updateState();
    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);
}

void DescriptorPropertiesDialog::fillFiles()
{
    for(auto f: records) {
        new QTreeWidgetItem(files, {f->fileName()});
    }
}

void DescriptorPropertiesDialog::prev()
{
    if (current > 0) {
        current--;
        updateState();
    }
}

void DescriptorPropertiesDialog::next()
{
    if (current < records.size()-1) {
        current++;
        updateState();
    }
}

void DescriptorPropertiesDialog::applyToCurrent()
{

}

void DescriptorPropertiesDialog::applyToAll()
{

}

void DescriptorPropertiesDialog::updateState()
{
//    prevButton->setEnabled(current > 0);
//    nextButton->setEnabled(current<records.size()-1);
    //    file->setText(records.at(current)->fileName());
}

void DescriptorPropertiesDialog::currentFileChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
//    static QStringList propertiesNames {"Файл", "Размер, байт", "Количество каналов",
//                                        "Дата и время записи", "Дата и время создания файла",
//                                        "GUID", "Создан в", "Файл-источник", "GUID файла-источника",
//                                        "Дата и время записи файла-источника", "Каналы источника"};
    if (current) {
        FileDescriptor *d = records.at(files->indexOfTopLevelItem(current));
        propertiesLabels.labels[0]->setText(d->fileName());
        propertiesLabels.labels[1]->setText(QString::number(QFileInfo(d->fileName()).size()));
        propertiesLabels.labels[2]->setText(QString::number(d->channelsCount()));
        propertiesLabels.labels[3]->setText(d->dataDescription().get("dateTime").toString());
        propertiesLabels.labels[3]->setText(d->dataDescription().get("fileCreationTime").toString());

    }
}
