#ifndef HEADERVIEW_H
#define HEADERVIEW_H

#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>

class HeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit HeaderView(Qt::Orientation orientation, QWidget *parent = 0);

    void setCheckable(int section, bool checkable);
Q_SIGNALS:
    void toggled(int section, Qt::CheckState checkState);
protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const;
    void mousePressEvent(QMouseEvent *event);
private:
    QRect checkBoxRect(const QRect &sourceRect) const;

    QSet<int> m_isCheckable;
    QRect _rect;
};

#endif // HEADERVIEW_H
