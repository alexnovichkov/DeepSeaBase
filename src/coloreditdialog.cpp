#include "coloreditdialog.h"

#include "app.h"
#include "colorselector.h"
#include <QtWidgets>
#include "logging.h"
#include "settings.h"

ColorEditDialog::ColorEditDialog(QWidget *parent) :
    QDialog(parent)
{DD;
    setWindowTitle("Редактор цветов графиков");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    QVariantList list = se->getSetting("colors").toList();
    selector = new ColorSelector(list);

    QTableWidget *table = new QTableWidget(selector->colorsCount(),1, this);
    table->setHorizontalHeaderLabels(QStringList("Цвет"));

    for (int i =0; i<selector->colorsCount(); ++i) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setBackground(selector->color(i));
        table->setItem(i,0,item);
    }
    connect(table,&QTableWidget::cellClicked, [=](int row, int col) {
        QColor color = table->item(row, col)->background().color();
        color = QColorDialog::getColor(color, this);

        if (color.isValid()) {
            table->item(row, col)->setBackground(color);
            selector->setColor(color, row);
        }
    }
    );

    QGridLayout *l = new QGridLayout;
    int i = 0;
    l->addWidget(table,i,0);
    i++;
    l->addWidget(buttonBox,i,0);
    setLayout(l);
}

void ColorEditDialog::accept()
{
    auto list = selector->getColors();
    se->setSetting("colors", list);
}
