#ifndef CORRECTIONDIALOG_H
#define CORRECTIONDIALOG_H

#include <QDialog>

class Plot;
class QTableWidget;
class CheckableHeaderView;
class QPushButton;

class CorrectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CorrectionDialog(Plot *plot, QWidget *parent = 0);
    

private:
    Plot *plot;
    QTableWidget *table;
    CheckableHeaderView *tableHeader;
    QPushButton *correctButton;
};

#endif // CORRECTIONDIALOG_H
