#include "coloreditdialog.h"

#include "colorselector.h"

#include <QtWidgets>

ColorEditDialog::ColorEditDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Редактор цветов графиков");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    ColorSelector *selector = ColorSelector::instance();

    QTableWidget *table = new QTableWidget(selector->colorsCount(),1, this);
    table->setHorizontalHeaderLabels(QStringList("Цвет"));

    QVector<QColor> colors = selector->getColors();

    for (int i =0; i<colors.size(); ++i) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setBackground(colors.at(i));
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
