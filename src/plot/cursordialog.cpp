#include "cursordialog.h"

#include "cursor.h"
#include <QtWidgets>
#include "settings.h"
#include "logging.h"

CursorDialog::CursorDialog(Cursor *cursor, QWidget *parent): QDialog(parent), cursor{cursor}
{DD;
    setWindowFlag(Qt::Tool);

    showValues = new QCheckBox("Показывать уровни дискрет", this);
    showValues->setChecked(cursor->showValues());

    snapToValues = new QCheckBox("Привязать к данным", this);
    snapToValues->setChecked(cursor->snapToValues());

    showPeaksInfo = new QCheckBox("Показывать информацию о пиках", this);
    showPeaksInfo->setChecked(cursor->showPeaksInfo());

    format = new QComboBox(this);
    format->addItems({"Фиксированный","Научный"});
    format->setCurrentIndex(static_cast<int>(cursor->format()));

    digits = new QSpinBox(this);
    digits->setRange(0,10);
    digits->setValue(cursor->digits());

    harmonics = new QSpinBox(this);
    harmonics->setRange(0,30);
    harmonics->setValue(cursor->harmonics());
    harmonics->setEnabled(cursor->type() == Cursor::Type::Harmonic);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto l = new QFormLayout;
    l->addRow(showValues);
    l->addRow(snapToValues);
    l->addRow(showPeaksInfo);
    l->addRow("Формат числа", format);
    l->addRow("Знаков после запятой", digits);
    l->addRow("Количество гармоник", harmonics);
    l->addRow(buttonBox);

    setLayout(l);
}


void CursorDialog::accept()
{DD;
    cursor->setDigits(digits->value());
    cursor->setHarmonics(harmonics->value());
    cursor->setShowValues(showValues->isChecked());
    cursor->setSnapToValues(snapToValues->isChecked());
    cursor->setFormat(format->currentIndex()==0?Cursor::Format::Fixed:Cursor::Format::Scientific);
    cursor->setShowPeaksInfo(showPeaksInfo->isChecked());

    se->setSetting("cursorSnapToValues", snapToValues->isChecked());
    se->setSetting("cursorShowYValues", showValues->isChecked());
    se->setSetting("cursorShowPeaksInfo", showPeaksInfo->isChecked());
    se->setSetting("cursorDigits", cursor->digits());
    se->setSetting("cursorHarmonics", cursor->harmonics());
    se->setSetting("cursorFormat", cursor->format()==Cursor::Format::Fixed?"fixed":"scientific");
    QDialog::accept();
}
