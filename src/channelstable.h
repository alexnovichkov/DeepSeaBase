#ifndef CHANNELSTABLE_H
#define CHANNELSTABLE_H

#include <QTableView>

class ChannelsTable : public QTableView
{
    Q_OBJECT
public:
    ChannelsTable(QWidget *parent=0);

    // QAbstractItemView interface
protected:
    virtual void startDrag(Qt::DropActions supportedActions) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
private:
    bool enableDragging = false;
};

#endif // CHANNELSTABLE_H
