#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QList>
#include <QHash>
#include <QVariant>

class QAction;
class QLabel;
class TabWidget;
class Tab;
class QMenu;
class QTreeWidget;

class QStatusBar;

class QFileSystemModel;
class QTreeView;
class QSplitter;

class DfdFileDescriptor;

//struct Act {
//    const char *key;
//    const char *text;
//    const char *tooltip;
//    const char *slot;
//    const char *shortcut;
//    const QKeySequence::StandardKey standardShortcut;
//    const char *icon;
//    const char *tabSlot;
//};

//struct Menu {
//    const char *key;
//    const char *text;
//    const char *actions;
//};


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void addFolder();
private:
    QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
    void setSetting(const QString &key, const QVariant &value);

    void addFiles(const QStringList &files, const QString &folder);
    QTreeWidget *tree;

    // key is folder, values - dfd files of this folder
    QList<DfdFileDescriptor *> records;

    QAction *addFolderAct;
};

#endif // MAINWINDOW_H
