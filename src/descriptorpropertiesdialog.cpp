#include "descriptorpropertiesdialog.h"

#include <QtWidgets>
#include "fileformats/filedescriptor.h"

DescriptorPropertiesDialog::DescriptorPropertiesDialog(QList<FileDescriptor *> &records, QWidget *parent)
    : QDialog(parent), records(records)
{
    setWindowTitle("Описатели файлов");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    prevButton = buttonBox->addButton("Предыдущий", QDialogButtonBox::ActionRole);
    connect(prevButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::prev);
    nextButton = buttonBox->addButton("Следующий", QDialogButtonBox::ActionRole);
    connect(nextButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::next);
    applyToCurrButton = buttonBox->addButton("Применить к текущему", QDialogButtonBox::ActionRole);
    connect(applyToCurrButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToCurrent);
    applyToAllButton = buttonBox->addButton("Применить ко всем", QDialogButtonBox::ActionRole);
    connect(applyToAllButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToAll);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));




    file = new QLabel(this);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Файл", this), 0, 0, 1, 1, Qt::AlignRight);
    grid->addWidget(file, 0, 1, 1, 2);
    grid->addWidget(new QLabel("Свойство", this), 1,1);
    grid->addWidget(new QLabel("Значение", this), 1,2);
    for (int i=0; i<6; ++i) {
        DescriptorProperty p;
        p.edit = new QLineEdit(this);
        p.checked = new QCheckBox(this);
        p.property = new QComboBox(this);
        properties << p;
        grid->addWidget(p.checked, i+2, 0);
        grid->addWidget(p.property, i+2, 1);
        grid->addWidget(p.edit, i+2, 2);
    }
    grid->addWidget(buttonBox, 8, 0, 1, 3, Qt::AlignRight);

    file->setText(records.at(current)->fileName());

    setLayout(grid);
    updateState();
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
    prevButton->setEnabled(current > 0);
    nextButton->setEnabled(current<records.size()-1);
    file->setText(records.at(current)->fileName());
}
