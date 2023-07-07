#ifndef COLOREDITDIALOG_H
#define COLOREDITDIALOG_H

#include <QDialog>

class ColorSelector;

class ColorEditDialog : public QDialog
{
    Q_OBJECT
    ColorSelector *selector = nullptr;
public:
    explicit ColorEditDialog(QWidget *parent = 0);
    
signals:
    
public slots:
    void accept() override;
};

#endif // COLOREDITDIALOG_H
