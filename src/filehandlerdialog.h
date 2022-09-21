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
    void untrack(int index);
private:
    FileHandler *fileHandler;
};

class FileHandlerDialog : public QDialog
{
    Q_OBJECT
public:
    FileHandlerDialog(FileHandler *fileHandler, QWidget *parent=nullptr);
    QStringList getTrackedPaths() const {return trackedPaths;}
private:
    void removeTrackedFile();
    FileHandler *fileHandler;
    HandlerModel *model;
    QTreeView *tree;
    QAction *removeFileAction;
    QAction *addFileAction;
    QStringList trackedPaths;
};

#endif // FILEHANDLERDIALOG_H
