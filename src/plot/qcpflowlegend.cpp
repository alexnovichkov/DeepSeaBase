#include "qcpflowlegend.h"
#include "logging.h"

QCPFlowLegend::QCPFlowLegend() :
  mIconTextPadding{}
{
  setRowSpacing(3);
  setColumnSpacing(8);
  setMargins(QMargins(7, 5, 7, 4));
  setAntialiased(false);
  setIconSize(32, 18);

  setIconTextPadding(7);

  setSelectableParts(spLegendBox | spItems);
  setSelectedParts(spNone);

  setBorderPen(QPen(Qt::black, 0));
  setSelectedBorderPen(QPen(Qt::blue, 2));
  setIconBorderPen(Qt::NoPen);
  setSelectedIconBorderPen(QPen(Qt::blue, 2));
  setBrush(Qt::white);
  setSelectedBrush(Qt::white);
  setTextColor(Qt::black);
  setSelectedTextColor(Qt::blue);
}

QCPFlowLegend::~QCPFlowLegend()
{
  clearItems();
//  if (qobject_cast<QCustomPlot*>(mParentPlot)) // make sure this isn't called from QObject dtor when QCustomPlot is already destructed (happens when the legend is not in any layout and thus QObject-child of QCustomPlot)
//    mParentPlot->legendRemoved(this);
}

/* no doc for getter, see setSelectedParts */
QCPFlowLegend::SelectableParts QCPFlowLegend::selectedParts() const
{
  // check whether any legend elements selected, if yes, add spItems to return value
  bool hasSelectedItems = false;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i) && item(i)->selected())
    {
      hasSelectedItems = true;
      break;
    }
  }
  if (hasSelectedItems)
    return mSelectedParts | spItems;
  else
    return mSelectedParts & ~spItems;
}

/*!
  Sets the pen, the border of the entire legend is drawn with.
*/
void QCPFlowLegend::setBorderPen(const QPen &pen)
{
  mBorderPen = pen;
}

/*!
  Sets the brush of the legend background.
*/
void QCPFlowLegend::setBrush(const QBrush &brush)
{
  mBrush = brush;
}

/*!
  Sets the default font of legend text. Legend items that draw text (e.g. the name of a graph) will
  use this font by default. However, a different font can be specified on a per-item-basis by
  accessing the specific legend item.

  This function will also set \a font on all already existing legend items.

  \see QCPAbstractLegendItem::setFont
*/
void QCPFlowLegend::setFont(const QFont &font)
{
  mFont = font;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setFont(mFont);
  }
}

/*!
  Sets the default color of legend text. Legend items that draw text (e.g. the name of a graph)
  will use this color by default. However, a different colors can be specified on a per-item-basis
  by accessing the specific legend item.

  This function will also set \a color on all already existing legend items.

  \see QCPAbstractLegendItem::setTextColor
*/
void QCPFlowLegend::setTextColor(const QColor &color)
{
  mTextColor = color;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setTextColor(color);
  }
}

/*!
  Sets the size of legend icons. Legend items that draw an icon (e.g. a visual
  representation of the graph) will use this size by default.
*/
void QCPFlowLegend::setIconSize(const QSize &size)
{
  mIconSize = size;
}

/*! \overload
*/
void QCPFlowLegend::setIconSize(int width, int height)
{
  mIconSize.setWidth(width);
  mIconSize.setHeight(height);
}

/*!
  Sets the horizontal space in pixels between the legend icon and the text next to it.
  Legend items that draw an icon (e.g. a visual representation of the graph) and text (e.g. the
  name of the graph) will use this space by default.
*/
void QCPFlowLegend::setIconTextPadding(int padding)
{
  mIconTextPadding = padding;
}

/*!
  Sets the pen used to draw a border around each legend icon. Legend items that draw an
  icon (e.g. a visual representation of the graph) will use this pen by default.

  If no border is wanted, set this to \a Qt::NoPen.
*/
void QCPFlowLegend::setIconBorderPen(const QPen &pen)
{
  mIconBorderPen = pen;
}

/*!
  Sets whether the user can (de-)select the parts in \a selectable by clicking on the QCustomPlot surface.
  (When \ref QCustomPlot::setInteractions contains \ref QCP::iSelectLegend.)

  However, even when \a selectable is set to a value not allowing the selection of a specific part,
  it is still possible to set the selection of this part manually, by calling \ref setSelectedParts
  directly.

  \see SelectablePart, setSelectedParts
*/
void QCPFlowLegend::setSelectableParts(const SelectableParts &selectable)
{
  if (mSelectableParts != selectable)
  {
    mSelectableParts = selectable;
    emit selectableChanged(mSelectableParts);
  }
}

/*!
  Sets the selected state of the respective legend parts described by \ref SelectablePart. When a part
  is selected, it uses a different pen/font and brush. If some legend items are selected and \a selected
  doesn't contain \ref spItems, those items become deselected.

  The entire selection mechanism is handled automatically when \ref QCustomPlot::setInteractions
  contains iSelectLegend. You only need to call this function when you wish to change the selection
  state manually.

  This function can change the selection state of a part even when \ref setSelectableParts was set to a
  value that actually excludes the part.

  emits the \ref selectionChanged signal when \a selected is different from the previous selection state.

  Note that it doesn't make sense to set the selected state \ref spItems here when it wasn't set
  before, because there's no way to specify which exact items to newly select. Do this by calling
  \ref QCPAbstractLegendItem::setSelected directly on the legend item you wish to select.

  \see SelectablePart, setSelectableParts, selectTest, setSelectedBorderPen, setSelectedIconBorderPen, setSelectedBrush,
  setSelectedFont
*/
void QCPFlowLegend::setSelectedParts(const SelectableParts &selected)
{
  SelectableParts newSelected = selected;
  mSelectedParts = this->selectedParts(); // update mSelectedParts in case item selection changed

  if (mSelectedParts != newSelected)
  {
    if (!mSelectedParts.testFlag(spItems) && newSelected.testFlag(spItems)) // attempt to set spItems flag (can't do that)
    {
      LOG(DEBUG) << Q_FUNC_INFO << "spItems flag can not be set, it can only be unset with this function";
      newSelected &= ~spItems;
    }
    if (mSelectedParts.testFlag(spItems) && !newSelected.testFlag(spItems)) // spItems flag was unset, so clear item selection
    {
      for (int i=0; i<itemCount(); ++i)
      {
        if (item(i))
          item(i)->setSelected(false);
      }
    }
    mSelectedParts = newSelected;
    emit selectionChanged(mSelectedParts);
  }
}

/*!
  When the legend box is selected, this pen is used to draw the border instead of the normal pen
  set via \ref setBorderPen.

  \see setSelectedParts, setSelectableParts, setSelectedBrush
*/
void QCPFlowLegend::setSelectedBorderPen(const QPen &pen)
{
  mSelectedBorderPen = pen;
}

/*!
  Sets the pen legend items will use to draw their icon borders, when they are selected.

  \see setSelectedParts, setSelectableParts, setSelectedFont
*/
void QCPFlowLegend::setSelectedIconBorderPen(const QPen &pen)
{
  mSelectedIconBorderPen = pen;
}

/*!
  When the legend box is selected, this brush is used to draw the legend background instead of the normal brush
  set via \ref setBrush.

  \see setSelectedParts, setSelectableParts, setSelectedBorderPen
*/
void QCPFlowLegend::setSelectedBrush(const QBrush &brush)
{
  mSelectedBrush = brush;
}

/*!
  Sets the default font that is used by legend items when they are selected.

  This function will also set \a font on all already existing legend items.

  \see setFont, QCPAbstractLegendItem::setSelectedFont
*/
void QCPFlowLegend::setSelectedFont(const QFont &font)
{
  mSelectedFont = font;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setSelectedFont(font);
  }
}

/*!
  Sets the default text color that is used by legend items when they are selected.

  This function will also set \a color on all already existing legend items.

  \see setTextColor, QCPAbstractLegendItem::setSelectedTextColor
*/
void QCPFlowLegend::setSelectedTextColor(const QColor &color)
{
  mSelectedTextColor = color;
  for (int i=0; i<itemCount(); ++i)
  {
    if (item(i))
      item(i)->setSelectedTextColor(color);
  }
}

/*!
  Returns the item with index \a i. If non-legend items were added to the legend, and the element
  at the specified cell index is not a QCPAbstractLegendItem, returns \c nullptr.

  Note that the linear index depends on the current fill order (\ref setFillOrder).

  \see itemCount, addItem, itemWithPlottable
*/
QCPAbstractLegendItem *QCPFlowLegend::item(int index) const
{
  return qobject_cast<QCPAbstractLegendItem*>(elementAt(index));
}

/*!
  Returns the QCPPlottableLegendItem which is associated with \a plottable (e.g. a \ref QCPGraph*).
  If such an item isn't in the legend, returns \c nullptr.

  \see hasItemWithPlottable
*/
QCPPlottableLegendItem *QCPFlowLegend::itemWithPlottable(const QCPAbstractPlottable *plottable) const
{
  for (int i=0; i<itemCount(); ++i)
  {
    if (QCPPlottableLegendItem *pli = qobject_cast<QCPPlottableLegendItem*>(item(i)))
    {
      if (pli->plottable() == plottable)
        return pli;
    }
  }
  return nullptr;
}

/*!
  Returns the number of items currently in the legend. It is identical to the base class
  QCPLayoutGrid::elementCount(), and unlike the other "item" interface methods of QCPFlowLegend,
  doesn't only address elements which can be cast to QCPAbstractLegendItem.

  Note that if empty cells are in the legend (e.g. by calling methods of the \ref QCPLayoutGrid
  base class which allows creating empty cells), they are included in the returned count.

  \see item
*/
int QCPFlowLegend::itemCount() const
{
  return elementCount();
}

/*!
  Returns whether the legend contains \a item.

  \see hasItemWithPlottable
*/
bool QCPFlowLegend::hasItem(QCPAbstractLegendItem *item) const
{
  for (int i=0; i<itemCount(); ++i)
  {
    if (item == this->item(i))
        return true;
  }
  return false;
}

/*!
  Returns whether the legend contains a QCPPlottableLegendItem which is associated with \a plottable (e.g. a \ref QCPGraph*).
  If such an item isn't in the legend, returns false.

  \see itemWithPlottable
*/
bool QCPFlowLegend::hasItemWithPlottable(const QCPAbstractPlottable *plottable) const
{
  return itemWithPlottable(plottable);
}

/*!
  Adds \a item to the legend, if it's not present already. The element is arranged according to the
  current fill order (\ref setFillOrder) and wrapping (\ref setWrap).

  Returns true on sucess, i.e. if the item wasn't in the list already and has been successfuly added.

  The legend takes ownership of the item.

  \see removeItem, item, hasItem
*/
bool QCPFlowLegend::addItem(QCPAbstractLegendItem *item)
{
  return addElement(item);
}

/*! \overload

  Removes the item with the specified \a index from the legend and deletes it.

  After successful removal, the legend is reordered according to the current fill order (\ref
  setFillOrder) and wrapping (\ref setWrap), so no empty cell remains where the removed \a item
  was. If you don't want this, rather use the raw element interface of \ref QCPLayoutGrid.

  Returns true, if successful. Unlike \ref QCPLayoutGrid::removeAt, this method only removes
  elements derived from \ref QCPAbstractLegendItem.

  \see itemCount, clearItems
*/
bool QCPFlowLegend::removeItem(int index)
{
    if (QCPAbstractLegendItem *ali = item(index))
    {
        bool success = remove(ali);
        if (success)
            updateLayout();
        return success;
    }
    return false;
}

/*! \overload

  Removes \a item from the legend and deletes it.

  After successful removal, the legend is reordered according to the current fill order (\ref
  setFillOrder) and wrapping (\ref setWrap), so no empty cell remains where the removed \a item
  was. If you don't want this, rather use the raw element interface of \ref QCPLayoutGrid.

  Returns true, if successful.

  \see clearItems
*/
bool QCPFlowLegend::removeItem(QCPAbstractLegendItem *item)
{
  bool success = remove(item);
  if (success)
    updateLayout(); // gets rid of empty cell by reordering
  return success;
}

/*!
  Removes all items from the legend.
*/
void QCPFlowLegend::clearItems()
{
  for (int i=elementCount()-1; i>=0; --i)
  {
    if (item(i))
      removeAt(i); // don't use removeItem() because it would unnecessarily reorder the whole legend for each item
  }
  updateLayout(); // get rid of empty cells by reordering once after all items are removed
}

/*!
  Returns the legend items that are currently selected. If no items are selected,
  the list is empty.

  \see QCPAbstractLegendItem::setSelected, setSelectable
*/
QList<QCPAbstractLegendItem *> QCPFlowLegend::selectedItems() const
{
  QList<QCPAbstractLegendItem*> result;
  for (int i=0; i<itemCount(); ++i)
  {
    if (QCPAbstractLegendItem *ali = item(i))
    {
      if (ali->selected())
        result.append(ali);
    }
  }
  return result;
}

/*! \internal

  A convenience function to easily set the QPainter::Antialiased hint on the provided \a painter
  before drawing main legend elements.

  This is the antialiasing state the painter passed to the \ref draw method is in by default.

  This function takes into account the local setting of the antialiasing flag as well as the
  overrides set with \ref QCustomPlot::setAntialiasedElements and \ref
  QCustomPlot::setNotAntialiasedElements.

  \seebaseclassmethod

  \see setAntialiased
*/
void QCPFlowLegend::applyDefaultAntialiasingHint(QCPPainter *painter) const
{
  applyAntialiasingHint(painter, mAntialiased, QCP::aeLegend);
}

/*! \internal

  Returns the pen used to paint the border of the legend, taking into account the selection state
  of the legend box.
*/
QPen QCPFlowLegend::getBorderPen() const
{
  return mSelectedParts.testFlag(spLegendBox) ? mSelectedBorderPen : mBorderPen;
}

/*! \internal

  Returns the brush used to paint the background of the legend, taking into account the selection
  state of the legend box.
*/
QBrush QCPFlowLegend::getBrush() const
{
  return mSelectedParts.testFlag(spLegendBox) ? mSelectedBrush : mBrush;
}

/*! \internal

  Draws the legend box with the provided \a painter. The individual legend items are layerables
  themselves, thus are drawn independently.
*/
void QCPFlowLegend::draw(QCPPainter *painter)
{
  // draw background rect:
  painter->setBrush(getBrush());
  painter->setPen(getBorderPen());
  painter->drawRect(mOuterRect);
}

/* inherits documentation from base class */
double QCPFlowLegend::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  if (!mParentPlot) return -1;
  if (onlySelectable && !mSelectableParts.testFlag(spLegendBox))
    return -1;

  if (mOuterRect.contains(pos.toPoint()))
  {
    if (details) details->setValue(spLegendBox);
    return mParentPlot->selectionTolerance()*0.99;
  }
  return -1;
}

/* inherits documentation from base class */
void QCPFlowLegend::selectEvent(QMouseEvent *event, bool additive, const QVariant &details, bool *selectionStateChanged)
{
  Q_UNUSED(event)
  mSelectedParts = selectedParts(); // in case item selection has changed
  if (details.value<SelectablePart>() == spLegendBox && mSelectableParts.testFlag(spLegendBox))
  {
    SelectableParts selBefore = mSelectedParts;
    setSelectedParts(additive ? mSelectedParts^spLegendBox : mSelectedParts|spLegendBox); // no need to unset spItems in !additive case, because they will be deselected by QCustomPlot (they're normal QCPLayerables with own deselectEvent)
    if (selectionStateChanged)
      *selectionStateChanged = mSelectedParts != selBefore;
  }
}

/* inherits documentation from base class */
void QCPFlowLegend::deselectEvent(bool *selectionStateChanged)
{
  mSelectedParts = selectedParts(); // in case item selection has changed
  if (mSelectableParts.testFlag(spLegendBox))
  {
    SelectableParts selBefore = mSelectedParts;
    setSelectedParts(selectedParts() & ~spLegendBox);
    if (selectionStateChanged)
      *selectionStateChanged = mSelectedParts != selBefore;
  }
}

/* inherits documentation from base class */
QCP::Interaction QCPFlowLegend::selectionCategory() const
{
  return QCP::iSelectLegend;
}

/* inherits documentation from base class */
void QCPFlowLegend::parentPlotInitialized(QCustomPlot *parentPlot)
{
    Q_UNUSED(parentPlot);
//  if (parentPlot && !parentPlot->legend)
//    parentPlot->legend = this;
}
