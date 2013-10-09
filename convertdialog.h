#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>

#include "dfdfiledescriptor.h"

class ConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConvertDialog(QWidget *parent = 0);
    void setDatabase(QList<DfdFileDescriptor *> *dataBase);
    QStringList getNewFiles() const {return newFiles;}
    QStringList getSpfFile(const QVector<int> &indexes, const QString &dir);
signals:
    
public slots:
private:
    QStringList getMethodSettings();
    QList<DfdFileDescriptor *> *dataBase;

    QStringList newFiles;

    int method;

    QVector<int> channels;
    int activeChannel;
    int baseChannel;
};

#endif // CONVERTDIALOG_H
