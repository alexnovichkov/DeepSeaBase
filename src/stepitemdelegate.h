#ifndef STEPITEMDELEGATE_H
#define STEPITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QDoubleSpinBox>

class StepItemDelegate : public QStyledItemDelegate
{
public:
    StepItemDelegate(QObject *parent = Q_NULLPTR) : QStyledItemDelegate(parent)
    {}

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QWidget *ed = QStyledItemDelegate::createEditor(parent, option, index);
        if (QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(ed)) {
            spin->setDecimals(10);
        }

        return ed;
    }
};

#endif // STEPITEMDELEGATE_H
