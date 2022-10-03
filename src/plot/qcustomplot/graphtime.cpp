#include "graphtime.h"

#include "data2d.h"
#include "enums.h"
#include "logging.h"

GraphTime::GraphTime(const QString &title, Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis)
    : Graph2D(title, channel, keyAxis, valueAxis)
{

}

GraphTime::~GraphTime()
{

}

void GraphTime::getOptimizedLineData(QVector<QCPGraphData> *lineData, const int begin, const int end) const
{DD0;
    if (!lineData) return;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
    if (begin == end) return;

    int dataCount = int(end-begin);
    int maxCount = (std::numeric_limits<int>::max)();

    {
        double keyPixelSpan = qAbs(keyAxis->coordToPixel(m_data->mainKey(begin)) - keyAxis->coordToPixel(m_data->mainKey(end-1)));
        if (2*keyPixelSpan+2 < static_cast<double>((std::numeric_limits<int>::max)()))
            maxCount = int(2*keyPixelSpan+2);
    }

    if (dataCount >= maxCount) // use adaptive sampling only if there are at least two points per pixel on average
    {
        QVector<QCPGraphData> data;

        auto it = begin;
        double minValue = m_data->mainValue(it);
        double maxValue = m_data->mainValue(it);
        auto currentIntervalFirstPoint = it;
        int reversedFactor = keyAxis->pixelOrientation(); // is used to calculate keyEpsilon pixel into the correct direction
        int reversedRound = reversedFactor==-1 ? 1 : 0; // is used to switch between floor (normal) and ceil (reversed) rounding of currentIntervalStartKey
        double currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(m_data->mainKey(begin))+reversedRound));
        double lastIntervalEndKey = currentIntervalStartKey;
        double keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor)); // interval of one pixel on screen when mapped to plot key coordinates
        bool keyEpsilonVariable = keyAxis->scaleType() == QCPAxis::stLogarithmic; // indicates whether keyEpsilon needs to be updated after every interval (for log axes)
        int intervalDataCount = 1;
        ++it; // advance iterator to second data point because adaptive sampling works in 1 point retrospect
        while (it != end)
        {
            if (m_data->mainKey(it) < currentIntervalStartKey+keyEpsilon) // data point is still within same pixel, so skip it and expand value span of this cluster if necessary
            {
                if (auto val = m_data->mainValue(it); val < minValue)
                    minValue = val;
                else if (val > maxValue)
                    maxValue = val;
                ++intervalDataCount;
            } else // new pixel interval started
            {
                data.append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.5, minValue));
                data.append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.5, maxValue));

                lastIntervalEndKey = m_data->mainKey(it-1);
                minValue = m_data->mainValue(it);
                maxValue = m_data->mainValue(it);
                currentIntervalFirstPoint = it;
                currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(m_data->mainKey(it))+reversedRound));
                if (keyEpsilonVariable)
                    keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
                intervalDataCount = 1;
            }
            ++it;
        }
        // handle last interval:
        data.append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.5, minValue));
        data.append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.5, maxValue));

        // rearrange lineData
        lineData->resize(data.size());
        for (int i = 0; i < data.size()/2; ++i) {
            (*lineData)[i].key = data[2*i].key;
            (*lineData)[i].value = data[2*i].value;

            (*lineData)[data.size() - 1 - i].key = data[2*i+1].key;
            (*lineData)[data.size() - 1 - i].value = data[2*i+1].value;
        }
    }
    else // don't use adaptive sampling algorithm, transfer points one-to-one from the data container into the output
    {
        *lineData = m_data->toLineData();
    }
}

void GraphTime::draw(QCPPainter *painter)
{DD0;
//    auto start = std::chrono::high_resolution_clock::now();

    if (!mKeyAxis || !mValueAxis) { qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return; }
    if (mKeyAxis.data()->range().size() <= 0 || m_data->isEmpty()) return;
    if (mLineStyle == lsNone && mScatterStyle.isNone()) return;

    // calculate the pixel span
    QCPAxis *keyAxis = mKeyAxis.data();
    int begin = 0;
    int end = dataCount();
    int maxCount = (std::numeric_limits<int>::max)();

    {
      double keyPixelSpan = qAbs(keyAxis->coordToPixel(m_data->mainKey(begin)) - keyAxis->coordToPixel(m_data->mainKey(end-1)));
      if (2*keyPixelSpan+2 < static_cast<double>((std::numeric_limits<int>::max)()))
        maxCount = int(2*keyPixelSpan+2);
    }

    //no need to draw in an optimized way
    if (maxCount > dataCount()) {
        Graph2D::draw(painter);
        return;
    }


    QVector<QPointF> lines; // line pixel coordinates will be stored here while iterating over segments

    // loop over and draw segments of unselected/selected data:
    QList<QCPDataRange> selectedSegments, unselectedSegments, allSegments;
    getDataSegments(selectedSegments, unselectedSegments);
    allSegments << unselectedSegments << selectedSegments;


    for (int i=0; i<allSegments.size(); ++i)
    {

      bool isSelectedSegment = i >= unselectedSegments.size();
      // get line pixel points appropriate to line style:
      QCPDataRange lineDataRange = isSelectedSegment ? allSegments.at(i) : allSegments.at(i).adjusted(-1, 1); // unselected segments extend lines to bordering selected data point (safe to exceed total data bounds in first/last segment, getLines takes care)
      getLines(&lines, lineDataRange);

//      auto diff = std::chrono::high_resolution_clock::now() - start;
//      qDebug()<< std::chrono::duration<double, std::milli>(diff).count() << "ms";

      if (isSelectedSegment && mSelectionDecorator)
        mSelectionDecorator->applyPen(painter);
      else {
          mPen.setWidth(0);
        painter->setPen(mPen);
      }

      // draw fill of graph:
      QColor c = mPen.color();
      c.setAlpha(200);
      QBrush b(c);

      if (isSelectedSegment && mSelectionDecorator)
        mSelectionDecorator->applyBrush(painter);
      else
        painter->setBrush(b);

      applyFillAntialiasingHint(painter);
      painter->drawPolygon(lines);
    }

    // draw other selection decoration that isn't just line/scatter pens and brushes:
    if (mSelectionDecorator)
      mSelectionDecorator->drawDecoration(painter, selection());
}
