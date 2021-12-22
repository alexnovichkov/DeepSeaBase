#ifndef CHANNELSTABLE_H
#define CHANNELSTABLE_H

#include <QTableView>

class ChannelsTable : public QTableView
{
    Q_OBJECT
public:
    ChannelsTable(QWidget *parent=0);
    void addAction(const QString &name, QAction *action);

    // QAbstractItemView interface
protected:
    virtual void startDrag(Qt::DropActions supportedActions) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
private slots:
    void editYName();
signals:
    void yNameChanged(const QString &newYName);
private:
    bool enableDragging = false;
    QHash<QString, QAction*> parentActions;
    QAction *editYNameAct;
};

#endif // CHANNELSTABLE_H
