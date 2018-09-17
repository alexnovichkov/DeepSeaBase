#include "correctiondialog.h"

#include <QtWidgets>

#include "checkableheaderview.h"
#include "plot.h"
#include "curve.h"
#include "dfdfiledescriptor.h"
#include "logging.h"

CorrectionDialog::CorrectionDialog(Plot *plot, QList<FileDescriptor *> &files, QWidget *parent) :
    QDialog(parent), plot(plot), files(files)
{DD;
    setWindowTitle("Добавление поправки к графику");

    table = new QTableWidget(0,3,this);

    tableHeader = new CheckableHeaderView(Qt::Horizontal, table);

    table->setHorizontalHeader(tableHeader);
    tableHeader->setCheckable(1,true);
    tableHeader->setCheckable(2,true);

    if (files.size()>1)
        allFilesCheckBox = new QCheckBox("Применить поправку ко всем\nвыделенным файлам", this);
    else
        allFilesCheckBox = 0;


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

        if (checked==0)
            tableHeader->setCheckState(col, Qt::Unchecked);
        else if (checked == table->rowCount())
            tableHeader->setCheckState(col, Qt::Checked);
        else
            tableHeader->setCheckState(col, Qt::PartiallyChecked);

        if (col == 2) {
//            if (item->checkState()==Qt::Checked)
                item->setText("Сохранить поправку в файл RAW");
//            else
//                item->setText("Изменить только график");
        }
    });
    tableHeader->setCheckState(0,Qt::Unchecked);
    tableHeader->setCheckState(2,Qt::Unchecked);

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setHorizontalHeaderLabels(QStringList()<<""<<"Канал"<<"Сохранить поправку в файл RAW");

    QList<Curve *> curves = plot->curves();
    table->setRowCount(curves.size());
    int i=0;
    foreach (Curve *curve, curves) {
        QTableWidgetItem *item = new QTableWidgetItem(curve->channel->legendName());
        item->setCheckState(Qt::Unchecked);
        table->setItem(i,1,item);

        item = new QTableWidgetItem();
        item->setCheckState(Qt::Unchecked);
        table->setItem(i,2,item);

        item = new QTableWidgetItem();
        item->setBackgroundColor(curve->pen().color());
        table->setItem(i++,0,item);
    }

    edit = new QLineEdit(this);
    edit->setText("0.0");

    correctButton = new QPushButton("Скорректировать", this);
    connect(correctButton, SIGNAL(clicked()),SLOT(correct()));


    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Отметьте каналы, введите величину поправки и нажмите \"Скорректировать\"\n\n"
                            "Если вы хотите записать эту поправку на диск, поставьте галочки во втором столбце.\n"
                            "Если величина поправки не устроила, введите другую величину поправки.\n"
                            "Разные поправки не будут накапливаться, если не стоит галочка во втором столбце.", this),
                 0,0,1,2);
    l->addWidget(table,1,0,1,2);
    l->addWidget(new QLabel("Величина поправки",this),2,0);
    l->addWidget(edit,2,1);

    l->addWidget(correctButton,3,1);
    if (allFilesCheckBox)
        l->addWidget(allFilesCheckBox,3,0);
    l->addWidget(buttonBox,4,0,1,2);
    setLayout(l);

    resize(500,500);
}

void CorrectionDialog::correct()
{
    DD;
    bool ok;
    double correctionValue = edit->text().toDouble(&ok);
    if (ok) {
        int selected = 0;
        for (int i=0; i<table->rowCount(); ++i) {
            if (table->item(i,1)->checkState()==Qt::Checked) selected++;
        }

        if (selected>0) {
            QList<Curve *> curves = plot->curves();
            for (int i=0; i<table->rowCount(); ++i) {
                if (table->item(i,1)->checkState()==Qt::Checked) {
                    Curve *curve = curves.at(i);
                    Channel *ch = curve->channel;

                    if (table->item(i,2)->checkState()==Qt::Checked) {
                        ch->addCorrection(correctionValue, true);
                        curve->descriptor->setChanged(true);
                        curve->descriptor->setDataChanged(true);
                        curve->descriptor->write();
                        curve->descriptor->writeRawFile();

                        // обновление данных кривой, если она упрощена
                        if (curve->isSimplified()) {
                            curve->setRawSamples();
                        }

                        // добавление поправки к выделенным файлам
                        if (allFilesCheckBox && allFilesCheckBox->isChecked()) {
                            foreach(FileDescriptor *d, files) {
                                if (d == curve->descriptor) continue;
                                ch = d->channel(curve->channelIndex);
                                if (ch) {
                                    ch->addCorrection(correctionValue, true);
                                    d->setChanged(true);
                                    d->setDataChanged(true);
                                    d->write();
                                    d->writeRawFile();
                                }
                            }
                        }
                    }
                    else {
                        ch->addCorrection(correctionValue, false);

                        // обновление данных кривой, если она упрощена
                        if (curve->isSimplified()) {
                            curve->setRawSamples();
                        }
                    }
                }
            }
            plot->updateAxes();
            plot->updateLegends();
            plot->replot();
        }
        else {
            QMessageBox::critical(this, "Поправка","Ни одного канала не выделено.\n"
                                  "Куда мне вносить поправку?");
        }
    }
    else {
        QMessageBox::critical(this, "Поправка","Введенное значение поправки не является числом.");

    }
}
