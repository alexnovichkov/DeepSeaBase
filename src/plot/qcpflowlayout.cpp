#include "qcpflowlayout.h"
#include "logging.h"

QCPFlowLayout::QCPFlowLayout()
{

}

void QCPFlowLayout::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation) return;

    m_orientation = orientation;
    updateLayout();
}


int QCPFlowLayout::elementCount() const
{
    return m_elements.size();
}

QCPLayoutElement *QCPFlowLayout::elementAt(int index) const
{
    return m_elements.value(index, nullptr);
}

QCPLayoutElement *QCPFlowLayout::takeAt(int index)
{
    if (QCPLayoutElement *el = elementAt(index))
    {
      releaseElement(el);
      m_elements.removeAt(index);
      return el;
    } else
    {
      LOG(ERROR) << Q_FUNC_INFO << "Attempt to take invalid index:" << index;
      return nullptr;
    }
}

bool QCPFlowLayout::take(QCPLayoutElement *element)
{
    if (element)
    {
      for (int i=0; i<elementCount(); ++i)
      {
        if (elementAt(i) == element)
        {
          takeAt(i);
          return true;
        }
      }
      LOG(ERROR) << Q_FUNC_INFO << "Element not in this layout, couldn't take";
    } else
      LOG(ERROR) << Q_FUNC_INFO << "Can't take nullptr element";
    return false;
}

int QCPFlowLayout::rowSpacing() const
{
    return m_rowSpacing;
}

void QCPFlowLayout::setRowSpacing(int rowSpacing)
{
    if (m_rowSpacing == rowSpacing) return;

    m_rowSpacing = rowSpacing;
    updateLayout();
}

bool QCPFlowLayout::hasElement(QCPLayoutElement *element) const
{
    return m_elements.contains(element);
}

bool QCPFlowLayout::appendElement(QCPLayoutElement *element)
{
    if (!element) {
        LOG(ERROR) << "Element is null";
        return false;
    }
    if (hasElement(element)) {
        LOG(ERROR) << "Element"<<element<<"already in layout";
        return false;
    }

    if (element && element->layout()) // remove from old layout first
      element->layout()->take(element);

    m_elements.append(element);
    adoptElement(element);
    return true;
}

bool QCPFlowLayout::addElement(QCPLayoutElement *element)
{
    return appendElement(element);
}

bool QCPFlowLayout::prependElement(QCPLayoutElement *element)
{
    if (!element) {
        LOG(DEBUG) << "Element is null";
        return false;
    }
    if (hasElement(element)) {
        LOG(DEBUG) << "Element "<<element<<" already in layout";
        return false;
    }

    if (element && element->layout()) // remove from old layout first
      element->layout()->take(element);

    m_elements.prepend(element);
    adoptElement(element);
    return true;
}

bool QCPFlowLayout::insertElement(QCPLayoutElement *element, int index)
{
    if (!element) {
        LOG(DEBUG) << "Element is null";
        return false;
    }
    if (hasElement(element)) {
        LOG(DEBUG) << "Element "<<element<<" already in layout";
        return false;
    }
    if (index < 0) index = 0;
    if (index > m_elements.size()) index = m_elements.size();

    if (element && element->layout()) // remove from old layout first
      element->layout()->take(element);

    m_elements.insert(index, element);
    adoptElement(element);
    return true;
}

int QCPFlowLayout::columnSpacing() const
{
    return m_columnSpacing;
}

void QCPFlowLayout::setColumnSpacing(int columnSpacing)
{
    if (m_columnSpacing == columnSpacing) return;

    m_columnSpacing = columnSpacing;
    updateLayout();
}

void QCPFlowLayout::updateLayout()
{
    //maximum rect we cannot exceed
    auto innerRect = mRect;

    int lineHeight = 0;
    int x = innerRect.x();
    int y = innerRect.y();

    int columns = 0;

    for (auto el: m_elements) {
        auto minSize = getFinalMinimumOuterSize(el);

        if (m_orientation == Qt::Horizontal) {
            int nextX = x + minSize.width() + m_columnSpacing;
            if ((nextX - m_columnSpacing > innerRect.right()+2 && lineHeight > 0)
                || columns == m_maximumColumnCount) {
                x = innerRect.x();
                y = y + lineHeight + m_rowSpacing;
                nextX = x + minSize.width() + m_columnSpacing;
                lineHeight = 0;
                columns = 0;
            }
            el->setOuterRect(QRect(QPoint(x, y), minSize));

            x = nextX;
            lineHeight = qMax(lineHeight, minSize.height());
            columns++;
        }
    }
}

QSize QCPFlowLayout::minimumOuterSizeHint() const
{
    //maximum rect we cannot exceed
    auto maximumSize = layout()->rect().size();

    int totalWidth = 0;
    int totalHeight = 0;

    int lineHeight = 0;
    int x = 0;
    int y = 0;

    int columns = 0;

    for (auto el: m_elements) {
        auto minSize = getFinalMinimumOuterSize(el);

        if (m_orientation == Qt::Horizontal) {
            int nextX = x + minSize.width() + m_columnSpacing;

            if ((nextX - m_columnSpacing > maximumSize.width() && lineHeight > 0)
                || columns == m_maximumColumnCount) {
                x = 0;
                y = y + lineHeight + m_rowSpacing;
                nextX = x + minSize.width() + m_columnSpacing;
                lineHeight = 0;
                columns = 0;
            }
            totalWidth = qMax(totalWidth, x+minSize.width()/*-m_columnSpacing*/);

            x = nextX;
            lineHeight = qMax(lineHeight, minSize.height());
            columns++;
        }
    }
    totalHeight = y + lineHeight;
    return QSize(totalWidth + mMargins.left() + mMargins.right(), totalHeight + mMargins.top() + mMargins.bottom());
}

QSize QCPFlowLayout::maximumOuterSizeHint() const
{
    return minimumOuterSizeHint();
}

int QCPFlowLayout::maximumColumnCount() const
{
    return m_maximumColumnCount;
}

void QCPFlowLayout::setMaximumColumnCount(int maximumColumnCount)
{
    m_maximumColumnCount = maximumColumnCount;
    updateLayout();
}
