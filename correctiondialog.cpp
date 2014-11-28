#include "correctiondialog.h"

#include <QtWidgets>

#include "checkableheaderview.h"
#include "plot.h"
#include "curve.h"
#include "dfdfiledescriptor.h"

CorrectionDialog::CorrectionDialog(Plot *plot, QWidget *parent) :
    QDialog(parent), plot(plot)
{
    setWindowTitle("Добавление поправки к графику");

    table = new QTableWidget(0,3,this);

    tableHeader = new CheckableHeaderView(Qt::Horizontal, table);

    table->setHorizontalHeader(tableHeader);
    tableHeader->setCheckable(1,true);
    tableHeader->setCheckable(2,true);

    connect(tableHeader, &CheckableHeaderView::toggled, [=](int column, Qt::CheckState state)
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
//        qDebug()<<"Checked"<<checked<<"of"<<table->rowCount();

        if (checked==0)
            tableHeader->setCheckState(col, Qt::Unchecked);
        else if (checked == table->rowCount())
            tableHeader->setCheckState(col, Qt::Checked);
        else
            tableHeader->setCheckState(col, Qt::PartiallyChecked);

        if (col == 2) {
            if (item->checkState()==Qt::Checked)
                item->setText("Сохранить поправку в файл RAW");
            else
                item->setText("Изменить только график");
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
        QTableWidgetItem *item = new QTableWidgetItem(curve->legend);
        item->setCheckState(Qt::Unchecked);
        table->setItem(i,1,item);

        item = new QTableWidgetItem();
        item->setCheckState(Qt::Unchecked);
        table->setItem(i,2,item);

        item = new QTableWidgetItem();
        item->setBackgroundColor(curve->pen().color());
        table->setItem(i++,0,item);
    }

    QLineEdit *edit = new QLineEdit(this);
    edit->setText("0.0");

    correctButton = new QPushButton("Скорректировать", this);
    connect(correctButton, &QPushButton::clicked, [=](){
        bool ok;
        double correctionValue = edit->text().toDouble(&ok);
        if (ok) {
            if (correctionValue == 0.0) return;

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
                        for (uint j = 0; j < ch->samplesCount(); ++j)
                            ch->yValues()[j] += correctionValue;

                        curve->legend += QString(correctionValue>=0?"+":"")+QString::number(correctionValue);
                        ch->setName(ch->name() + QString(correctionValue>=0?"+":"")
                                           +QString::number(correctionValue));
                        if (table->item(i,2)->checkState()==Qt::Checked) {
                            curve->descriptor->setChanged(true);
                            curve->descriptor->setDataChanged(true);
                            curve->descriptor->write();
                            curve->descriptor->writeRawFile();
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
            QMessageBox::critical(this, "Поправка","Введенное значение поправки не является числом.\n"
                                  "Вам дается последняя попытка, после чего компьютер взорвется");

        }
    });


    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Отметьте каналы, введите величину поправки\nи нажмите \"Скорректировать\"", this),
                 0,0,1,2);
    l->addWidget(table,1,0,1,2);
    l->addWidget(new QLabel("Величина поправки",this),2,0);
    l->addWidget(edit,2,1);
    l->addWidget(correctButton,3,1);
    l->addWidget(buttonBox,4,0,1,2);
    setLayout(l);

    resize(500,500);
}
