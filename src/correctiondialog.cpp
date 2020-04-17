#include "correctiondialog.h"

#include <QtWidgets>

#include "checkableheaderview.h"
#include "plot/plot.h"
#include "plot/curve.h"
#include "fileformats/dfdfiledescriptor.h"
#include "logging.h"

CorrectionDialog::CorrectionDialog(Plot *plot, QList<FileDescriptor *> &files, QWidget *parent) :
    QDialog(parent), plot(plot), files(files)
{DD;
    setWindowTitle("Добавление поправки к графику");

    // удаляем из списка файлов файлы, графики которых построены на экране
    // чтобы эти файлы не мешались
    foreach (Curve *curve, plot->curves)
        this->files.removeAll(curve->channel->descriptor());

    table = new QTableWidget(0,3,this);

    tableHeader = new CheckableHeaderView(Qt::Horizontal, table);

    table->setHorizontalHeader(tableHeader);
    tableHeader->setCheckable(1,true);


    allFilesCheckBox = new QCheckBox("Применить поправку ко всем выделенным файлам", this);
    if (files.size()<1) allFilesCheckBox->setEnabled(false);


    connect(tableHeader, &CheckableHeaderView::toggled, [this](int column, Qt::CheckState state)
    {
        if (column<0 || column >= table->columnCount()) return;

        if (state == Qt::PartiallyChecked) return;
        for (int i=0; i<table->rowCount(); ++i)
            table->item(i,column)->setCheckState(state);
    });

    connect(table, &QTableWidget::itemChanged, [=](QTableWidgetItem *item)
    {
        const int col = item->column();

        int checked = 0;
        for (int i=0; i<table->rowCount(); ++i) {
            if (table->item(i, col) && table->item(i, col)->checkState()==Qt::Checked) checked++;
        }

        Qt::CheckState state = Qt::PartiallyChecked;
        if (checked==0)
            state = Qt::Unchecked;
        else if (checked == table->rowCount())
            state = Qt::Checked;
        else
            state = Qt::PartiallyChecked;

        tableHeader->setCheckState(col, state);
    });

    tableHeader->setCheckState(0,Qt::Unchecked);

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels(QStringList()<<""<<"Канал"<<"Поправка");

    table->setRowCount(plot->curves.size());
    int i=0;
    foreach (Curve *curve, plot->curves) {
        QTableWidgetItem *item = new QTableWidgetItem(curve->channel->legendName());
        item->setCheckState(Qt::Unchecked);
        table->setItem(i,1,item);

        item = new QTableWidgetItem();
        table->setItem(i,2,item);

        item = new QTableWidgetItem();
        item->setBackgroundColor(curve->pen().color());
        table->setItem(i++,0,item);
    }

    edit = new QLineEdit(this);
    edit->setText("0.0");

    correctButton = new QToolButton(this);
    correctButton->setText("Скорректировать");
    connect(correctButton, SIGNAL(clicked(bool)), SLOT(correct()));
    correctButton->setSizePolicy(correctButton->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

    correctionType = new QComboBox(this);
    correctionType->addItems(QStringList()<<"Слагаемое"<<"Множитель");
    correctionType->setCurrentIndex(0);


    QDialogButtonBox *buttonBox = new QDialogButtonBox(/*Qt::Vertical, */this);
    auto b = buttonBox->addButton("Закрыть без сохранения", QDialogButtonBox::RejectRole);
    b->setDefault(true);
    b->setAutoDefault(true);
    b = buttonBox->addButton("Закрыть и сохранить поправку в файл данных", QDialogButtonBox::AcceptRole);
    b->setDefault(false);
    b->setAutoDefault(false);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Отметьте каналы, введите величину поправки и нажмите \"Скорректировать\"\n\n"
                            "Если величина поправки не устроила, введите другую величину поправки.\n"
                            "Поправки не накапливаются.", this),
                 0,0,1,3);
    l->addWidget(table,1,0,1,3);
    l->addWidget(new QLabel("Величина поправки",this),2,0);
    l->addWidget(edit,2,1);

    l->addWidget(new QLabel("Тип поправки",this),3,0);
    l->addWidget(correctionType,3,1);

    l->addWidget(correctButton,2,2,2,1);

    l->addWidget(allFilesCheckBox,4,0,1,3);

    l->addWidget(buttonBox,5,0,1,3);
    l->setRowStretch(5,l->rowStretch(5)+30);
    setLayout(l);

    //resize(500,500);
}

void CorrectionDialog::correct()
{
    DD;
    bool ok;
    QString s = edit->text();
    double correctionValue = s.toDouble(&ok);
    if (!ok) {
        s.replace(',','.');
        correctionValue = s.toDouble(&ok);
    }
    if (!ok) {
        QMessageBox::critical(this, "Поправка","Введенное значение поправки не является числом.");
        return;
    }

    int selected = 0;
    for (int i=0; i<table->rowCount(); ++i) {
        if (table->item(i,1)->checkState()==Qt::Checked) selected++;
    }

    if (selected==0) {
        QMessageBox::critical(this, "Поправка","Ни одного канала не выделено.\n"
                                               "Куда мне вносить поправку?");
        return;
    }

    if (correctionValue < 0.0 && correctionType->currentIndex()==1) {
        QMessageBox::critical(this, "Поправка","Отрицательный множитель коррекции!\n"
                                               "Лучше так не делать");
        return;
    }



    for (int i=0; i<table->rowCount(); ++i) {
        if (table->item(i,1)->checkState()==Qt::Checked) {
            Curve *curve = plot->curves.at(i);
            Channel *ch = curve->channel;
            int channelNumber = ch->index();

            ch->data()->setTemporaryCorrection(correctionValue, correctionType->currentIndex());
            if (ch->data()->hasCorrection())
                table->item(i,2)->setText(ch->data()->correctionString());
            else
                table->item(i,2)->setText("");

            if (allFilesCheckBox->isChecked())
            foreach (FileDescriptor *file, files) {
                if (Channel *ch1 = file->channel(channelNumber)) {
                    if (!ch1->populated()) ch1->populate();
                    ch1->data()->setTemporaryCorrection(correctionValue, correctionType->currentIndex());
                }
            }
        }
    }

    plot->recalculateScale(true);
    plot->recalculateScale(false);
    plot->update();
}

void CorrectionDialog::makeCorrectionConstant(Channel *channel)
{
    if (!channel) return;

    channel->data()->makeCorrectionConstant();
    //Обработка предыдущей коррекции.
    QString previousCorrection = channel->correction();

    if (previousCorrection.isEmpty()) {
        // ранее коррекция не проводилась
        channel->setCorrection(channel->data()->correctionString());
    }
    else {
        // ранее коррекция проводилась, мы должны учесть старое и новое значения
        if (previousCorrection.startsWith("[")) previousCorrection.remove(0,1);
        if (previousCorrection.endsWith("]")) previousCorrection.chop(1);
        int previousType = previousCorrection.startsWith("*")?1:0;
        if (previousType == 1) previousCorrection.remove(0,1);
        bool ok;
        double previousCorrectionValue = previousCorrection.toDouble(&ok);
        if (!ok) previousCorrectionValue = previousType == 0 ? 0.0 : 1.0;

        if (previousType == correctionType->currentIndex()) {
            // типы коррекций совпадают, можно работать
            double newValue = previousType == 0?previousCorrectionValue + channel->data()->correction():
                                                previousCorrectionValue * channel->data()->correction();
            channel->setCorrection(DataHolder::correctionString(newValue, previousType));
        }
        else
            channel->setCorrection(channel->data()->correctionString());
    }
    //подчищаем за собой
    channel->data()->removeCorrection();
}

void CorrectionDialog::accept()
{
    QList<FileDescriptor*> list;
    // сперва графики
    for (int i=0; i<table->rowCount(); ++i) {
        if (table->item(i,2)->text().isEmpty()) continue;

        Channel *ch = plot->curves.at(i)->channel;
        makeCorrectionConstant(ch);
        if (!list.contains(ch->descriptor())) list << ch->descriptor();

        if (allFilesCheckBox->isChecked()) {
            foreach (FileDescriptor *file, files) {
                if (Channel *ch1 = file->channel(ch->index())) {
                    makeCorrectionConstant(ch1);
                    if (!list.contains(ch1->descriptor())) list << ch1->descriptor();
                }
            }
        }
    }
    foreach (FileDescriptor *f, list) {
        f->setChanged(true);
        f->setDataChanged(true);
        f->write();
        f->writeRawFile();
    }

    plot->recalculateScale(true);
    plot->recalculateScale(false);
    plot->update();
    QDialog::accept();
}

void CorrectionDialog::reject()
{
    for (int i=0; i<table->rowCount(); ++i) {
        plot->curves.at(i)->channel->data()->removeCorrection();

        if (allFilesCheckBox->isChecked())
        foreach (FileDescriptor *file, files) {
            if (Channel *ch1 = file->channel(plot->curves.at(i)->channel->index())) {
                ch1->data()->removeCorrection();
            }
        }
    }

//    plot->updateAxes();
//    plot->updateLegends();
//    plot->replot();
    plot->recalculateScale(true);
    plot->recalculateScale(false);
    plot->update();
    QDialog::reject();
}
