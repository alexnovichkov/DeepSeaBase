#include "editdescriptionsdialog.h"

#include <QtWidgets>

class StackWidget : public QWidget
{
public:
    StackWidget(FileDescriptor *record, QWidget *parent = 0) : QWidget(parent),
        record(record)
    {
        edit = new QPlainTextEdit(this);
        QStringList data;
        DataDescription descriptions = record->dataDescription();
        for (auto it = descriptions.data.begin(); it != descriptions.data.end(); ++it) {
            data << it.key()+"="+it.value().toString();
        }
        edit->setPlainText(data.join("\n"));

        QVBoxLayout *l = new QVBoxLayout;
        l->addWidget(edit);
        setLayout(l);
    }
    DataDescription description()
    {
        DataDescription result;
        QStringList list = edit->toPlainText().split("\n");
        for(const QString &s: list) {
            if (s.isEmpty()) continue;
            if (s.contains("=")) {
                result.put(s.section("=",0,0),s.section("=",1));
            }
            else {
                result.put("", s);
            }
        }
        return result;
    }
    FileDescriptor *record;

private:
    QPlainTextEdit *edit;

};

EditDescriptionsDialog::EditDescriptionsDialog(QList<FileDescriptor *> &records, QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Описатели файлов");

    QListWidget *recordsList = new QListWidget(this);
    stack = new QStackedWidget(this);
    for (FileDescriptor *record: records) {
        new QListWidgetItem(QFileInfo(record->fileName()).fileName(), recordsList);
        stack->addWidget(new StackWidget(record));
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

QHash<FileDescriptor *, DataDescription> EditDescriptionsDialog::descriptions()
{
    QHash<FileDescriptor *, DataDescription> result;

    for (int i=0; i<stack->count(); ++i) {
        StackWidget *sw = dynamic_cast<StackWidget*>(stack->widget(i));
        if (sw) result.insert(sw->record, sw->description());
    }
    return result;
}
