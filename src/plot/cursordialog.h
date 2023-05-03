#ifndef CURSORDIALOG_H
#define CURSORDIALOG_H

#include <QDialog>

class Cursor;
class QCheckBox;
class QComboBox;
class QSpinBox;

class CursorDialog : public QDialog
{
    Q_OBJECT
public:
    CursorDialog(Cursor *cursor, QWidget *parent = nullptr);
private:
    Cursor *cursor;
    QCheckBox *showValues;
    QCheckBox *snapToValues;
    QCheckBox *showPeaksInfo;
    QComboBox *format;
    QSpinBox *digits;
    QSpinBox *harmonics;

    // QDialog interface
public slots:
    virtual void accept() override;
};

#endif // CURSORDIALOG_H
