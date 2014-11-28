#include "editdescriptionsdialog.h"

#include <QtWidgets>

class StackWidget : public QWidget
{
public:
    StackWidget(const DescriptionList &descriptions, QWidget *parent = 0) : QWidget(parent)
    {
        edit = new QPlainTextEdit(this);
        QStringList data;
        foreach (const DescriptionEntry &entry, descriptions) {
            QString s = entry.second;
            if (!entry.first.isEmpty()) s = entry.first+"="+s;
            data << s;
        }
        edit->setPlainText(data.join("\n"));

        QVBoxLayout *l = new QVBoxLayout;
        l->addWidget(edit);
        setLayout(l);
    }
private:
    QPlainTextEdit *edit;
};

EditDescriptionsDialog::EditDescriptionsDialog(QList<FileDescriptor *> &records, QWidget *parent) :
    QDialog(parent), records(records)
{
    this->setWindowTitle("Описатели файлов");

    QListWidget *recordsList = new QListWidget(this);
    QStackedWidget *stack = new QStackedWidget(this);
    foreach (FileDescriptor *record, records) {
        new QListWidgetItem(QFileInfo(record->fileName()).fileName(), recordsList);
        stack->addWidget(new StackWidget(record->dataDescriptor()));
    }
    connect(recordsList, &QListWidget::currentRowChanged, [=](int index){
        stack->setCurrentIndex(index);
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(recordsList, 0,0,2,1);
    grid->addWidget(stack, 1,1);
    grid->addWidget(buttonBox, 2,0,1,2,Qt::AlignRight);

    setLayout(grid);

    resize(900,500);
}

QHash<FileDescriptor *, DescriptionList> EditDescriptionsDialog::descriptions()
{

}
