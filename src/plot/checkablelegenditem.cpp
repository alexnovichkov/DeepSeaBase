#include "checkablelegenditem.h"

#include "checkablelegend.h"
#include "graph2d.h"

//QCPCheckableLegendItem::QCPCheckableLegendItem(QCPCheckableLegend *parent, Graph2D *plottable)
//    : QCPPlottableLegendItem(parent, plottable), mGraph(plottable)
//{

//}

//void QCPCheckableLegendItem::draw(QCPPainter *painter)
//{
//    if (!mGraph) return;
//    painter->setFont(getFont());
//    painter->setPen(QPen(getTextColor()));

//    QStyleOptionButton option;
//    option.state |= QStyle::State_Enabled;
//    option.rect = mGraph->parentPlot()->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option);
//    option.rect.moveTo(mRect.topLeft());
//    if (mGraph->visible())
//        option.state |= QStyle::State_On;
//    else
//        option.state |= QStyle::State_Off;

//    QSize iconSize = mParentLegend->iconSize();
//    QRect iconRect(option.rect.topRight(), iconSize);
//    iconRect.moveLeft(mParentLegend->iconTextPadding()+iconRect.left());

//    QRect textRect = painter->fontMetrics().boundingRect(0, 0, 0, iconSize.height(), Qt::TextDontClip, mPlottable->name());
//    int textHeight = qMax(textRect.height(), iconSize.height());  // if text has smaller height than icon, center text vertically in icon height, else align tops
//    painter->drawText(mRect.x()+option.rect.width()+iconSize.width()+2*mParentLegend->iconTextPadding(),
//                      mRect.y(), textRect.width(), textHeight, Qt::TextDontClip, mGraph->name());
//    // draw icon:
//    painter->save();
//    painter->setClipRect(iconRect, Qt::IntersectClip);
//    mGraph->drawLegendIcon(painter, iconRect);
//    painter->restore();



//    mGraph->parentPlot()->style()->drawControl(QStyle::CE_CheckBox, &option, painter);

//    // draw icon border:
//    if (getIconBorderPen().style() != Qt::NoPen)
//    {
//        painter->setPen(getIconBorderPen());
//        painter->setBrush(Qt::NoBrush);
//        int halfPen = qCeil(painter->pen().widthF()*0.5)+1;
//        painter->setClipRect(mOuterRect.adjusted(-halfPen, -halfPen, halfPen, halfPen)); // extend default clip rect so thicker pens (especially during selection) are not clipped
//        painter->drawRect(iconRect);
//    }
//}

//QSize QCPCheckableLegendItem::minimumOuterSizeHint() const
//{
//    if (!mGraph) return {};
//    QSize result(0, 0);
//    QRect textRect;
//    QFontMetrics fontMetrics(getFont());
//    QSize iconSize = mParentLegend->iconSize();
//    textRect = fontMetrics.boundingRect(0, 0, 0, iconSize.height(), Qt::TextDontClip, mGraph->name());
//    result.setWidth(iconSize.width() + mParentLegend->iconTextPadding() + textRect.width());
//    result.setHeight(qMax(textRect.height(), iconSize.height()));

//    QStyleOptionButton checkBoxStyleOption;
//    QRect checkBoxRect = mGraph->parentPlot()->style()->subElementRect(QStyle::SE_CheckBoxIndicator,
//                                                 &checkBoxStyleOption);

//    result.rwidth() += checkBoxRect.width();
//    result.rwidth() += 16;


//    result.rwidth() += mMargins.left()+mMargins.right();
//    result.rheight() += mMargins.top()+mMargins.bottom();
//    return result;
//}
