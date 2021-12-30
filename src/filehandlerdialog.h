#ifndef FILEHANDLERDIALOG_H
#define FILEHANDLERDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>

class FileHandler;
class QTreeView;

class HandlerModel: public QAbstractTableModel
{
public:
    HandlerModel(FileHandler *fileHandler, QObject *parent=0);

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
    FileHandler *fileHandler;
};

class FileHandlerDialog : public QDialog
{
    Q_OBJECT
public:
    FileHandlerDialog(FileHandler *fileHandler, QWidget *parent=nullptr);
private:
    FileHandler *fileHandler;
    HandlerModel *model;
    QTreeView *tree;
};

#endif // FILEHANDLERDIALOG_H