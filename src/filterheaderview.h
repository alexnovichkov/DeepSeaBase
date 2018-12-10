#ifndef FILTERHEADERVIEW_H
#define FILTERHEADERVIEW_H

#include <QHeaderView>

class QLineEdit;

class FilterHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    FilterHeaderView(Qt::Orientation orientation, QWidget *parent = Q_NULLPTR);
    void setFilterBoxes(int count);
signals:
    void filterActivated();
    void filterChanged(const QString &text, int column);
private slots:
    void adjustPositions();
private:
    int _padding;
    QList<QLineEdit*> _editors;

    // QWidget interface
public:
    virtual QSize sizeHint() const override;

    // QAbstractItemView interface
protected slots:
    virtual void updateGeometries() override;
};

#endif // FILTERHEADERVIEW_H
