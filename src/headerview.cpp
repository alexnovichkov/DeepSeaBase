#include "headerview.h"

#include <QtWidgets>
#include "logging.h"

HeaderView::HeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)

{DD;
    setSectionsClickable(true);
}

void HeaderView::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
{DD;
    painter->save();
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();

    if (m_isCheckable.contains(logicalIndex)) {
        QStyleOptionButton option;
        if (isEnabled())
            option.state |= QStyle::State_Enabled;
        option.rect = checkBoxRect(rect);

        Qt::CheckState state = Qt::CheckState(model()->headerData(logicalIndex,Qt::Horizontal,Qt::CheckStateRole).toInt());
        if (state==Qt::Checked)
            option.state |= QStyle::State_On;
        else if (state==Qt::Unchecked)
            option.state |= QStyle::State_Off;
        else
            option.state |= QStyle::State_NoChange;
        style()->drawControl(QStyle::CE_CheckBox, &option, painter);
    }
}

void HeaderView::mousePressEvent(QMouseEvent *event)
{DD;
    int logicalIndex = logicalIndexAt(event->pos());
    QRect r(0,0,sectionSize(logicalIndex), this->height());
    QRect rr = checkBoxRect(r);

    if (isEnabled()
        && m_isCheckable.contains(logicalIndex)
        && rr.contains(event->pos())) {

        Qt::CheckState state = Qt::CheckState(model()->headerData(logicalIndex,Qt::Horizontal,
                                                                  Qt::CheckStateRole).toInt());
        if (state==Qt::Checked)
            state = Qt::Unchecked;
        else
            state = Qt::Checked;
        model()->setHeaderData(logicalIndex, Qt::Horizontal, state, Qt::CheckStateRole);
    }
    else QHeaderView::mousePressEvent(event);
}

QRect HeaderView::checkBoxRect(const QRect &sourceRect) const
{DD;
    QStyleOptionButton checkBoxStyleOption;
    QRect checkBoxRect = style()->subElementRect(QStyle::SE_CheckBoxIndicator,
                                                 &checkBoxStyleOption);
    QPoint checkBoxPoint(sourceRect.x() + 3,
                         sourceRect.y() +
                         sourceRect.height() / 2 -
                         checkBoxRect.height() / 2);
    return QRect(checkBoxPoint, checkBoxRect.size());
}

void HeaderView::setCheckable(int section, bool checkable)
{DD;
    if (checkable) m_isCheckable.insert(section);
    else m_isCheckable.remove(section);
}
