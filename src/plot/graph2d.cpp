#include "graph2d.h"

#include "data2d.h"
#include "fileformats/filedescriptor.h"
#include "plot/plot.h"
#include "checkablelegend.h"
#include "qcpplot.h"
#include "logging.h"
#include "settings.h"
#include "qcppointmarker.h"

Graph2D::LineStyle lineStyleByType(Channel *c)
{DD;
    if (c->octaveType() > 0 && Settings::getSetting("plotOctaveAsHistogram", false).toBool())
        return Graph2D::lsStep;
    return Graph2D::lsLine;
}

Graph2D::Graph2D(Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis) :
    QCPAbstractPlottable(keyAxis, valueAxis), Curve(channel)
{DD;
    setData(new Data2D(channel->data()));
    setName(channel->legendName());

    setLineStyle(lineStyleByType(channel));
    if (channel->octaveType() > 0 && Settings::getSetting("plotOctaveAsHistogram", false).toBool())
        setAntialiased(false);
}

Graph2D::~Graph2D()
{DD;
    delete m_data;
}

void Graph2D::setData(Data2D *data)
{DD;
    if (m_data == data) return;
    delete m_data;
    m_data = data;
    // update();
}

void Graph2D::setLineStyle(Graph2D::LineStyle ls)
{DD;
    mLineStyle = ls;
}

void Graph2D::setScatterStyle(const QCPScatterStyle &style)
{DD;
    mScatterStyle = style;
}

void Graph2D::setScatterSkip(int skip)
{DD;
    mScatterSkip = qMax(0, skip);
}

void Graph2D::setChannelFillGraph(Graph2D *targetGraph)
{DD;
    // prevent setting channel target to this graph itself:
    if (targetGraph == this)
    {
      LOG(WARNING) << Q_FUNC_INFO << "targetGraph is this graph itself";
      mChannelFillGraph = nullptr;
      return;
    }
    // prevent setting channel target to a graph not in the plot:
    if (targetGraph && targetGraph->mParentPlot != mParentPlot)
    {
      LOG(WARNING) << Q_FUNC_INFO << "targetGraph not in same plot";
      mChannelFillGraph = nullptr;
      return;
    }

    mChannelFillGraph = targetGraph;
}

void Graph2D::setAdaptiveSampling(bool enabled)
{DD;
    mAdaptiveSampling = enabled;
}

QCPRange Graph2D::getValueRange(bool &foundRange, QCP::SignDomain inSignDomain, const QCPRange &inKeyRange) const
{DD;
    return m_data->valueRange(foundRange, inSignDomain, inKeyRange);
}

double Graph2D::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{DD;
    if ((onlySelectable && mSelectable == QCP::stNone) || m_data->isEmpty())
      return -1;
    if (!mKeyAxis || !mValueAxis)
      return -1;

    if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()) || mParentPlot->interactions().testFlag(QCP::iSelectPlottablesBeyondAxisRect))
    {
      int closestDataPoint = m_data->size();
      double result = pointDistance(pos, closestDataPoint);
      if (details)
        details->setValue(QCPDataSelection(QCPDataRange(closestDataPoint, closestDataPoint+1)));
      return result;
    }
    return -1;
}

QCPRange Graph2D::getKeyRange(bool &foundRange, QCP::SignDomain inSignDomain) const
{DD;
    return m_data->keyRange(foundRange, inSignDomain);
}

void Graph2D::getDataSegments(QList<QCPDataRange> &selectedSegments, QList<QCPDataRange> &unselectedSegments) const
{DD;
    selectedSegments.clear();
    unselectedSegments.clear();
    if (mSelectable == QCP::stWhole) // stWhole selection type draws the entire plottable with selected style if mSelection isn't empty
    {
      if (QCPAbstractPlottable::selected())
        selectedSegments << QCPDataRange(0, dataCount());
      else
        unselectedSegments << QCPDataRange(0, dataCount());
    } else
    {
      QCPDataSelection sel(selection());
      sel.simplify();
      selectedSegments = sel.dataRanges();
      unselectedSegments = sel.inverse(QCPDataRange(0, dataCount())).dataRanges();
    }
}

void Graph2D::drawPolyline(QCPPainter *painter, const QVector<QPointF> &lineData) const
{DD;
    // if drawing lines in plot (instead of PDF), reduce 1px lines to cosmetic, because at least in
    // Qt6 drawing of "1px" width lines is much slower even though it has same appearance apart from
    // High-DPI. In High-DPI cases people must set a pen width slightly larger than 1.0 to get
    // correct DPI scaling of width, but of course with performance penalty.
    if (!painter->modes().testFlag(QCPPainter::pmVectorized) &&
        qFuzzyCompare(painter->pen().widthF(), 1.0))
    {
      QPen newPen = painter->pen();
      newPen.setWidth(0);
      painter->setPen(newPen);
    }

    // if drawing solid line and not in PDF, use much faster line drawing instead of polyline:
    if (mParentPlot->plottingHints().testFlag(QCP::phFastPolylines) &&
        painter->pen().style() == Qt::SolidLine &&
        !painter->modes().testFlag(QCPPainter::pmVectorized) &&
        !painter->modes().testFlag(QCPPainter::pmNoCaching))
    {
      int i = 0;
      bool lastIsNan = false;
      const int lineDataSize = lineData.size();
      while (i < lineDataSize && (qIsNaN(lineData.at(i).y()) || qIsNaN(lineData.at(i).x()))) // make sure first point is not NaN
        ++i;
      ++i; // because drawing works in 1 point retrospect
      while (i < lineDataSize)
      {
        if (!qIsNaN(lineData.at(i).y()) && !qIsNaN(lineData.at(i).x())) // NaNs create a gap in the line
        {
          if (!lastIsNan)
            painter->drawLine(lineData.at(i-1), lineData.at(i));
          else
            lastIsNan = false;
        } else
          lastIsNan = true;
        ++i;
      }
    } else
    {
      int segmentStart = 0;
      int i = 0;
      const int lineDataSize = lineData.size();
      while (i < lineDataSize)
      {
        if (qIsNaN(lineData.at(i).y()) || qIsNaN(lineData.at(i).x()) || qIsInf(lineData.at(i).y())) // NaNs create a gap in the line. Also filter Infs which make drawPolyline block
        {
          painter->drawPolyline(lineData.constData()+segmentStart, i-segmentStart); // i, because we don't want to include the current NaN point
          segmentStart = i+1;
        }
        ++i;
      }
      // draw last segment:
      painter->drawPolyline(lineData.constData()+segmentStart, lineDataSize-segmentStart);
    }
}

void Graph2D::drawFill(QCPPainter *painter, QVector<QPointF> *lines) const
{DD;
    if (painter->brush().style() == Qt::NoBrush || painter->brush().color().alpha() == 0) return;

    applyFillAntialiasingHint(painter);
    const QVector<QCPDataRange> segments = getNonNanSegments(lines, keyAxis()->orientation());
    if (!mChannelFillGraph)
    {
      // draw base fill under graph, fill goes all the way to the zero-value-line:
      for (QCPDataRange segment: segments)
        painter->drawPolygon(getFillPolygon(lines, segment));
    } else
    {
      // draw fill between this graph and mChannelFillGraph:
      QVector<QPointF> otherLines;
      mChannelFillGraph->getLines(&otherLines, QCPDataRange(0, mChannelFillGraph->dataCount()));
      if (!otherLines.isEmpty())
      {
        QVector<QCPDataRange> otherSegments = getNonNanSegments(&otherLines, mChannelFillGraph->keyAxis()->orientation());
        QVector<QPair<QCPDataRange, QCPDataRange> > segmentPairs = getOverlappingSegments(segments, lines, otherSegments, &otherLines);
        for (int i=0; i<segmentPairs.size(); ++i)
          painter->drawPolygon(getChannelFillPolygon(lines, segmentPairs.at(i).first, &otherLines, segmentPairs.at(i).second));
      }
    }
}

void Graph2D::drawScatterPlot(QCPPainter *painter, const QVector<QPointF> &scatters, const QCPScatterStyle &style) const
{DD;
    applyScattersAntialiasingHint(painter);
    style.applyTo(painter, mPen);
    foreach (const QPointF &scatter, scatters)
        style.drawShape(painter, scatter.x(), scatter.y());
}

void Graph2D::drawLinePlot(QCPPainter *painter, const QVector<QPointF> &lines) const
{DD;
    if (painter->pen().style() != Qt::NoPen && painter->pen().color().alpha() != 0)
    {
      applyDefaultAntialiasingHint(painter);
      drawPolyline(painter, lines);
    }
}

void Graph2D::drawImpulsePlot(QCPPainter *painter, const QVector<QPointF> &lines) const
{DD;
    if (painter->pen().style() != Qt::NoPen && painter->pen().color().alpha() != 0)
    {
      applyDefaultAntialiasingHint(painter);
      QPen oldPen = painter->pen();
      QPen newPen = painter->pen();
      newPen.setCapStyle(Qt::FlatCap); // so impulse line doesn't reach beyond zero-line
      painter->setPen(newPen);
      painter->drawLines(lines);
      painter->setPen(oldPen);
    }
}

void Graph2D::getOptimizedLineData(QVector<QCPGraphData> *lineData, const int begin, const int end) const
{DD;
//    auto start = std::chrono::high_resolution_clock::now();
    if (!lineData) return;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return; }
    if (begin == end) return;

    int extraPoints = 0;

    int dataCount = int(end-begin);
    int maxCount = (std::numeric_limits<int>::max)();

    {
        double keyPixelSpan = qAbs(keyAxis->coordToPixel(channel->data()->xValue(begin))
                                   - keyAxis->coordToPixel(channel->data()->xValue(end-1)));

        if (2*keyPixelSpan+2 < static_cast<double>((std::numeric_limits<int>::max)()))
            maxCount = int(2*keyPixelSpan+2);
    }


    if (dataCount >= maxCount) {// use adaptive sampling only if there are at least two points per pixel on average
        auto it = begin;
        double minYValue = channel->data()->yValue(it);
        double maxYValue = minYValue;
        double xValue = channel->data()->xValue(it);

        double currentIntervalFirstPointY = minYValue;
        double currentIntervalFirstPointX = xValue;

        int reversedFactor = keyAxis->pixelOrientation(); // is used to calculate keyEpsilon pixel into the correct direction
        double currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(xValue)));
        double lastIntervalEndKey = currentIntervalStartKey;
        double keyEpsilon = qAbs(currentIntervalStartKey - keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor)); // interval of one pixel on screen when mapped to plot key coordinates
        bool keyEpsilonVariable = keyAxis->scaleType() == QCPAxis::stLogarithmic; // indicates whether keyEpsilon needs to be updated after every interval (for log axes)
        int intervalDataCount = 1;
        ++it; // advance iterator to second data point because adaptive sampling works in 1 point retrospect
        while (it != end) {
            xValue = channel->data()->xValue(it);
            double yValue = channel->data()->yValue(it, 0);
            if (xValue < currentIntervalStartKey + keyEpsilon) {// data point is still within same pixel, so skip it and expand value span of this cluster if necessary
                if (yValue < minYValue)
                    minYValue = yValue;
                else if (yValue > maxYValue)
                    maxYValue = yValue;
                ++intervalDataCount;
            }
            else {// new pixel interval started
                if (intervalDataCount >= 2) {// last pixel had multiple data points, consolidate them to a cluster
                    if (lastIntervalEndKey < currentIntervalStartKey-keyEpsilon) {// last point is further away, so first point of this cluster must be at a real data point
                        lineData->append(QCPGraphData(currentIntervalStartKey + keyEpsilon*0.2, currentIntervalFirstPointY));
                        extraPoints++;
                    }
                    lineData->append(QCPGraphData(currentIntervalStartKey + keyEpsilon*0.25, minYValue));
                    lineData->append(QCPGraphData(currentIntervalStartKey + keyEpsilon*0.75, maxYValue));
                    if (xValue > currentIntervalStartKey+keyEpsilon*2) {// new pixel started further away from previous cluster, so make sure the last point of the cluster is at a real data point
                        extraPoints++;
                        lineData->append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.8, channel->data()->yValue(it-1)));
                    }
                }
                else
                    lineData->append(QCPGraphData(currentIntervalFirstPointX, currentIntervalFirstPointY));
                lastIntervalEndKey = channel->data()->xValue(it-1);
                minYValue = yValue;
                maxYValue = minYValue;

                currentIntervalFirstPointY = yValue;
                currentIntervalFirstPointX = xValue;

                currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(xValue)));
                if (keyEpsilonVariable)
                    keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
                intervalDataCount = 1;
            }
            ++it;
        }
        // handle last interval:
        if (intervalDataCount >= 2) {// last pixel had multiple data points, consolidate them to a cluster
            if (lastIntervalEndKey < currentIntervalStartKey-keyEpsilon) {// last point wasn't a cluster, so first point of this cluster must be at a real data point
                extraPoints++;
                lineData->append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.2, currentIntervalFirstPointY));
            }
            lineData->append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.25, minYValue));
            lineData->append(QCPGraphData(currentIntervalStartKey+keyEpsilon*0.75, maxYValue));
        }
        else
            lineData->append(QCPGraphData(currentIntervalFirstPointX, currentIntervalFirstPointY));

    }  // don't use adaptive sampling algorithm, transfer points one-to-one from the data container into the output
    else {
        *lineData = m_data->toLineData(begin, end);
    }
}

void Graph2D::getOptimizedScatterData(QVector<QCPGraphData> *scatterData, int begin, int end) const
{DD;
    if (!scatterData) return;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return; }

    const int scatterModulo = mScatterSkip+1;
    const bool doScatterSkip = mScatterSkip > 0;
    int beginIndex = begin;
    int endIndex = end;
    while (doScatterSkip && begin != end && beginIndex % scatterModulo != 0) // advance begin iterator to first non-skipped scatter
    {
      ++beginIndex;
      ++begin;
    }
    if (begin == end) return;
    int dataCount = int(end-begin);
    int maxCount = (std::numeric_limits<int>::max)();

    {
      int keyPixelSpan = int(qAbs(keyAxis->coordToPixel(m_data->mainKey(begin)) - keyAxis->coordToPixel(m_data->mainKey(end-1))));
      maxCount = 2*keyPixelSpan+2;
    }

    if (dataCount >= maxCount) // use adaptive sampling only if there are at least two points per pixel on average
    {
      double valueMaxRange = valueAxis->range().upper;
      double valueMinRange = valueAxis->range().lower;
      auto it = begin;
      int itIndex = int(beginIndex);
      double minValue = m_data->mainValue(it);
      double maxValue = m_data->mainValue(it);
      auto minValueIt = it;
      auto maxValueIt = it;
      auto currentIntervalStart = it;
      int reversedFactor = keyAxis->pixelOrientation(); // is used to calculate keyEpsilon pixel into the correct direction
      int reversedRound = reversedFactor==-1 ? 1 : 0; // is used to switch between floor (normal) and ceil (reversed) rounding of currentIntervalStartKey
      double currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(m_data->mainKey(begin))+reversedRound));
      double keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor)); // interval of one pixel on screen when mapped to plot key coordinates
      bool keyEpsilonVariable = keyAxis->scaleType() == QCPAxis::stLogarithmic; // indicates whether keyEpsilon needs to be updated after every interval (for log axes)
      int intervalDataCount = 1;
      // advance iterator to second (non-skipped) data point because adaptive sampling works in 1 point retrospect:
      if (!doScatterSkip)
        ++it;
      else
      {
        itIndex += scatterModulo;
        if (itIndex < endIndex) // make sure we didn't jump over end
          it += scatterModulo;
        else
        {
          it = end;
          itIndex = endIndex;
        }
      }
      // main loop over data points:
      while (it != end)
      {
        if (m_data->mainKey(it) < currentIntervalStartKey+keyEpsilon) // data point is still within same pixel, so skip it and expand value span of this pixel if necessary
        {
          if (auto val = m_data->mainValue(it); val < minValue && val > valueMinRange && val < valueMaxRange)
          {
            minValue = val;
            minValueIt = it;
          } else if (val > maxValue && val > valueMinRange && val < valueMaxRange)
          {
            maxValue = val;
            maxValueIt = it;
          }
          ++intervalDataCount;
        } else // new pixel started
        {
          if (intervalDataCount >= 2) // last pixel had multiple data points, consolidate them
          {
            // determine value pixel span and add as many points in interval to maintain certain vertical data density (this is specific to scatter plot):
            double valuePixelSpan = qAbs(valueAxis->coordToPixel(minValue)-valueAxis->coordToPixel(maxValue));
            int dataModulo = qMax(1, qRound(intervalDataCount/(valuePixelSpan/4.0))); // approximately every 4 value pixels one data point on average
            auto intervalIt = currentIntervalStart;
            int c = 0;
            while (intervalIt != it)
            {
              if ((c % dataModulo == 0 || intervalIt == minValueIt || intervalIt == maxValueIt) && m_data->mainValue(intervalIt) > valueMinRange && m_data->mainValue(intervalIt) < valueMaxRange)
                scatterData->append({m_data->mainKey(intervalIt), m_data->mainValue(intervalIt)});
              ++c;
              if (!doScatterSkip)
                ++intervalIt;
              else
                intervalIt += scatterModulo; // since we know indices of "currentIntervalStart", "intervalIt" and "it" are multiples of scatterModulo, we can't accidentally jump over "it" here
            }
          } else if (auto val = m_data->mainValue(currentIntervalStart); val > valueMinRange && val < valueMaxRange)
            scatterData->append({m_data->mainKey(currentIntervalStart), val});
          minValue = m_data->mainValue(it);
          maxValue = m_data->mainValue(it);
          currentIntervalStart = it;
          currentIntervalStartKey = keyAxis->pixelToCoord(int(keyAxis->coordToPixel(m_data->mainKey(it))+reversedRound));
          if (keyEpsilonVariable)
            keyEpsilon = qAbs(currentIntervalStartKey-keyAxis->pixelToCoord(keyAxis->coordToPixel(currentIntervalStartKey)+1.0*reversedFactor));
          intervalDataCount = 1;
        }
        // advance to next data point:
        if (!doScatterSkip)
          ++it;
        else
        {
          itIndex += scatterModulo;
          if (itIndex < endIndex) // make sure we didn't jump over end
            it += scatterModulo;
          else
          {
            it = end;
            itIndex = endIndex;
          }
        }
      }
      // handle last interval:
      if (intervalDataCount >= 2) // last pixel had multiple data points, consolidate them
      {
        // determine value pixel span and add as many points in interval to maintain certain vertical data density (this is specific to scatter plot):
        double valuePixelSpan = qAbs(valueAxis->coordToPixel(minValue)-valueAxis->coordToPixel(maxValue));
        int dataModulo = qMax(1, qRound(intervalDataCount/(valuePixelSpan/4.0))); // approximately every 4 value pixels one data point on average
        auto intervalIt = currentIntervalStart;
        int intervalItIndex = intervalIt;
        int c = 0;
        while (intervalIt != it)
        {
          if (auto val = m_data->mainValue(intervalIt); (c % dataModulo == 0 || intervalIt == minValueIt || intervalIt == maxValueIt) && val > valueMinRange && val < valueMaxRange)
            scatterData->append({m_data->mainKey(intervalIt), val});
          ++c;
          if (!doScatterSkip)
            ++intervalIt;
          else // here we can't guarantee that adding scatterModulo doesn't exceed "it" (because "it" is equal to "end" here, and "end" isn't scatterModulo-aligned), so check via index comparison:
          {
            intervalItIndex += scatterModulo;
            if (intervalItIndex < itIndex)
              intervalIt += scatterModulo;
            else
            {
              intervalIt = it;
              intervalItIndex = itIndex;
            }
          }
        }
      } else if (auto val = m_data->mainValue(currentIntervalStart); val > valueMinRange && val < valueMaxRange)
        scatterData->append({m_data->mainKey(currentIntervalStart), val});

    } else // don't use adaptive sampling algorithm, transfer points one-to-one from the data container into the output
    {
      auto it = begin;
      int itIndex = beginIndex;
      scatterData->reserve(dataCount);
      while (it != end)
      {
        scatterData->append({m_data->mainKey(it), m_data->mainValue(it)});
        // advance to next data point:
        if (!doScatterSkip)
          ++it;
        else
        {
          itIndex += scatterModulo;
          if (itIndex < endIndex)
            it += scatterModulo;
          else
          {
            it = end;
            itIndex = endIndex;
          }
        }
      }
    }
}

void Graph2D::getVisibleDataBounds(int &begin, int &end, const QCPDataRange &rangeRestriction) const
{DD;
    if (rangeRestriction.isEmpty())
    {
      end = m_data->size();
      begin = end;
    } else
    {
      QCPAxis *keyAxis = mKeyAxis.data();
      QCPAxis *valueAxis = mValueAxis.data();
      if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return; }
      // get visible data range:
      begin = m_data->findBegin(keyAxis->range().lower);
      end = m_data->findEnd(keyAxis->range().upper);
      // limit lower/upperEnd to rangeRestriction:
      m_data->limitIteratorsToDataRange(begin, end, rangeRestriction); // this also ensures rangeRestriction outside data bounds doesn't break anything
    }
}

void Graph2D::getLines(QVector<QPointF> *lines, const QCPDataRange &dataRange) const
{DD;
    if (!lines) return;
    if (mLineStyle == lsNone) {
        lines->clear();
        return;
    }
    int begin, end;
    getVisibleDataBounds(begin, end, dataRange);
    if (begin == end)
    {
      lines->clear();
      return;
    }

    QVector<QCPGraphData> lineData;
    if (mLineStyle != lsNone)
      getOptimizedLineData(&lineData, begin, end);

    if (mKeyAxis->rangeReversed() != (mKeyAxis->orientation() == Qt::Vertical)) // make sure key pixels are sorted ascending in lineData (significantly simplifies following processing)
      std::reverse(lineData.begin(), lineData.end());

    switch (mLineStyle)
    {
      case lsLine: *lines = dataToLines(lineData); break;
      case lsStep: *lines = dataToStepLines(lineData); break;
        default: break;
    }
}

void Graph2D::getScatters(QVector<QPointF> *scatters, const QCPDataRange &dataRange) const
{DD;
    if (!scatters) return;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; scatters->clear(); return; }

    int begin, end;
    getVisibleDataBounds(begin, end, dataRange);
    if (begin == end)
    {
      scatters->clear();
      return;
    }

    QVector<QCPGraphData> data;
    getOptimizedScatterData(&data, begin, end);

    if (mKeyAxis->rangeReversed() != (mKeyAxis->orientation() == Qt::Vertical)) // make sure key pixels are sorted ascending in data (significantly simplifies following processing)
      std::reverse(data.begin(), data.end());

    scatters->resize(data.size());
    if (keyAxis->orientation() == Qt::Vertical)
    {
      for (int i=0; i<data.size(); ++i)
      {
        if (!qIsNaN(data.at(i).value))
        {
          (*scatters)[i].setX(valueAxis->coordToPixel(data.at(i).value));
          (*scatters)[i].setY(keyAxis->coordToPixel(data.at(i).key));
        }
      }
    } else
    {
      for (int i=0; i<data.size(); ++i)
      {
        if (!qIsNaN(data.at(i).value))
        {
          (*scatters)[i].setX(keyAxis->coordToPixel(data.at(i).key));
          (*scatters)[i].setY(valueAxis->coordToPixel(data.at(i).value));
        }
      }
    }
}

QVector<QPointF> Graph2D::dataToLines(const QVector<QCPGraphData> &data) const
{DD;
    QVector<QPointF> result;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return result; }

    result.resize(data.size());

    // transform data points to pixels:
    if (keyAxis->orientation() == Qt::Vertical)
    {
      for (int i=0; i<data.size(); ++i)
      {
        result[i].setX(valueAxis->coordToPixel(data.at(i).value));
        result[i].setY(keyAxis->coordToPixel(data.at(i).key));
      }
    } else // key axis is horizontal
    {
      for (int i=0; i<data.size(); ++i)
      {
        result[i].setX(keyAxis->coordToPixel(data.at(i).key));
        result[i].setY(valueAxis->coordToPixel(data.at(i).value));
      }
    }
    return result;
}

QVector<QPointF> Graph2D::dataToStepLines(const QVector<QCPGraphData> &data) const
{DD;
    QVector<QPointF> result;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return result; }

    result.resize(data.size()*2);

    double factor = 2.0;

    switch (channel->octaveType()) {
        case 1: factor = pow(10.0, 0.15); break;
        case 2: factor = pow(10.0, 0.075); break;
        case 3: factor = pow(10.0, 0.05); break;
        case 6: factor = pow(10.0, 0.025); break;
        case 12: factor = pow(10.0, 0.0125); break;
        case 24: factor = pow(10.0, 0.00625); break;
        default: break;
    }

    double key = keyAxis->coordToPixel(data.first().key);
    double value = valueAxis->coordToPixel(data.first().value);

    int index = 0;
    result[index].setX(key / factor);
    result[index].setY(value);
    for (int i=0; i<data.size()-1; ++i) {
        index++;
        result[index].setX(keyAxis->coordToPixel(qSqrt(data.at(i).key * data.at(i+1).key)));
        result[index].setY(valueAxis->coordToPixel(data.at(i).value));
        index++;
        result[index].setX(keyAxis->coordToPixel(qSqrt(data.at(i).key * data.at(i+1).key)));
        result[index].setY(valueAxis->coordToPixel(data.at(i+1).value));
    }
    //last point
    index++;
    result[index].setX(keyAxis->coordToPixel(data.last().key * factor));
    result[index].setY(valueAxis->coordToPixel(data.last().value));


    return result;
}

QVector<QCPDataRange> Graph2D::getNonNanSegments(const QVector<QPointF> *lineData, Qt::Orientation keyOrientation) const
{DD;
    QVector<QCPDataRange> result;
    const int n = lineData->size();

    QCPDataRange currentSegment(-1, -1);
    int i = 0;

    if (keyOrientation == Qt::Horizontal)
    {
      while (i < n)
      {
        while (i < n && qIsNaN(lineData->at(i).y())) // seek next non-NaN data point
          ++i;
        if (i == n)
          break;
        currentSegment.setBegin(i++);
        while (i < n && !qIsNaN(lineData->at(i).y())) // seek next NaN data point or end of data
          ++i;
        currentSegment.setEnd(i++);
        result.append(currentSegment);
      }
    } else // keyOrientation == Qt::Vertical
    {
      while (i < n)
      {
        while (i < n && qIsNaN(lineData->at(i).x())) // seek next non-NaN data point
          ++i;
        if (i == n)
          break;
        currentSegment.setBegin(i++);
        while (i < n && !qIsNaN(lineData->at(i).x())) // seek next NaN data point or end of data
          ++i;
        currentSegment.setEnd(i++);
        result.append(currentSegment);
      }
    }
    return result;
}

QVector<QPair<QCPDataRange, QCPDataRange> > Graph2D::getOverlappingSegments(QVector<QCPDataRange> thisSegments, const QVector<QPointF> *thisData, QVector<QCPDataRange> otherSegments, const QVector<QPointF> *otherData) const
{DD;
    QVector<QPair<QCPDataRange, QCPDataRange> > result;
    if (thisData->isEmpty() || otherData->isEmpty() || thisSegments.isEmpty() || otherSegments.isEmpty())
      return result;

    int thisIndex = 0;
    int otherIndex = 0;
    const bool verticalKey = mKeyAxis->orientation() == Qt::Vertical;
    while (thisIndex < thisSegments.size() && otherIndex < otherSegments.size())
    {
      if (thisSegments.at(thisIndex).size() < 2) // segments with fewer than two points won't have a fill anyhow
      {
        ++thisIndex;
        continue;
      }
      if (otherSegments.at(otherIndex).size() < 2) // segments with fewer than two points won't have a fill anyhow
      {
        ++otherIndex;
        continue;
      }
      double thisLower, thisUpper, otherLower, otherUpper;
      if (!verticalKey)
      {
        thisLower = thisData->at(thisSegments.at(thisIndex).begin()).x();
        thisUpper = thisData->at(thisSegments.at(thisIndex).end()-1).x();
        otherLower = otherData->at(otherSegments.at(otherIndex).begin()).x();
        otherUpper = otherData->at(otherSegments.at(otherIndex).end()-1).x();
      } else
      {
        thisLower = thisData->at(thisSegments.at(thisIndex).begin()).y();
        thisUpper = thisData->at(thisSegments.at(thisIndex).end()-1).y();
        otherLower = otherData->at(otherSegments.at(otherIndex).begin()).y();
        otherUpper = otherData->at(otherSegments.at(otherIndex).end()-1).y();
      }

      int bPrecedence;
      if (segmentsIntersect(thisLower, thisUpper, otherLower, otherUpper, bPrecedence))
        result.append(QPair<QCPDataRange, QCPDataRange>(thisSegments.at(thisIndex), otherSegments.at(otherIndex)));

      if (bPrecedence <= 0) // otherSegment doesn't reach as far as thisSegment, so continue with next otherSegment, keeping current thisSegment
        ++otherIndex;
      else // otherSegment reaches further than thisSegment, so continue with next thisSegment, keeping current otherSegment
        ++thisIndex;
    }

    return result;
}

bool Graph2D::segmentsIntersect(double aLower, double aUpper, double bLower, double bUpper, int &bPrecedence) const
{DD;
    bPrecedence = 0;
    if (aLower > bUpper)
    {
      bPrecedence = -1;
      return false;
    } else if (bLower > aUpper)
    {
      bPrecedence = 1;
      return false;
    } else
    {
      if (aUpper > bUpper)
        bPrecedence = -1;
      else if (aUpper < bUpper)
        bPrecedence = 1;

      return true;
    }
}

QPointF Graph2D::getFillBasePoint(QPointF matchingDataPoint) const
{DD;
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return {}; }

    QPointF result;
    if (valueAxis->scaleType() == QCPAxis::stLinear)
    {
      if (keyAxis->orientation() == Qt::Horizontal)
      {
        result.setX(matchingDataPoint.x());
        result.setY(valueAxis->coordToPixel(0));
      } else // keyAxis->orientation() == Qt::Vertical
      {
        result.setX(valueAxis->coordToPixel(0));
        result.setY(matchingDataPoint.y());
      }
    } else // valueAxis->mScaleType == QCPAxis::stLogarithmic
    {
      // In logarithmic scaling we can't just draw to value 0 so we just fill all the way
      // to the axis which is in the direction towards 0
      if (keyAxis->orientation() == Qt::Vertical)
      {
        if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
            (valueAxis->range().upper > 0 && valueAxis->rangeReversed())) // if range is negative, zero is on opposite side of key axis
          result.setX(keyAxis->axisRect()->right());
        else
          result.setX(keyAxis->axisRect()->left());
        result.setY(matchingDataPoint.y());
      } else if (keyAxis->axisType() == QCPAxis::atTop || keyAxis->axisType() == QCPAxis::atBottom)
      {
        result.setX(matchingDataPoint.x());
        if ((valueAxis->range().upper < 0 && !valueAxis->rangeReversed()) ||
            (valueAxis->range().upper > 0 && valueAxis->rangeReversed())) // if range is negative, zero is on opposite side of key axis
          result.setY(keyAxis->axisRect()->top());
        else
          result.setY(keyAxis->axisRect()->bottom());
      }
    }
    return result;
}

const QPolygonF Graph2D::getFillPolygon(const QVector<QPointF> *lineData, QCPDataRange segment) const
{DD;
    if (segment.size() < 2)
      return QPolygonF();
    QPolygonF result(segment.size()+2);

    result[0] = getFillBasePoint(lineData->at(segment.begin()));
    std::copy(lineData->constBegin()+segment.begin(), lineData->constBegin()+segment.end(), result.begin()+1);
    result[result.size()-1] = getFillBasePoint(lineData->at(segment.end()-1));

    return result;
}

const QPolygonF Graph2D::getChannelFillPolygon(const QVector<QPointF> *thisData, QCPDataRange thisSegment, const QVector<QPointF> *otherData, QCPDataRange otherSegment) const
{DD;
    if (!mChannelFillGraph)
      return QPolygonF();

    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return QPolygonF(); }
    if (!mChannelFillGraph.data()->mKeyAxis) { LOG(ERROR) << Q_FUNC_INFO << "channel fill target key axis invalid"; return QPolygonF(); }

    if (mChannelFillGraph.data()->mKeyAxis.data()->orientation() != keyAxis->orientation())
      return QPolygonF(); // don't have same axis orientation, can't fill that (Note: if keyAxis fits, valueAxis will fit too, because it's always orthogonal to keyAxis)

    if (thisData->isEmpty()) return QPolygonF();
    QVector<QPointF> thisSegmentData(thisSegment.size());
    QVector<QPointF> otherSegmentData(otherSegment.size());
    std::copy(thisData->constBegin()+thisSegment.begin(), thisData->constBegin()+thisSegment.end(), thisSegmentData.begin());
    std::copy(otherData->constBegin()+otherSegment.begin(), otherData->constBegin()+otherSegment.end(), otherSegmentData.begin());
    // pointers to be able to swap them, depending which data range needs cropping:
    QVector<QPointF> *staticData = &thisSegmentData;
    QVector<QPointF> *croppedData = &otherSegmentData;

    // crop both vectors to ranges in which the keys overlap (which coord is key, depends on axisType):
    if (keyAxis->orientation() == Qt::Horizontal)
    {
      // x is key
      // crop lower bound:
      if (staticData->first().x() < croppedData->first().x()) // other one must be cropped
        qSwap(staticData, croppedData);
      const int lowBound = findIndexBelowX(croppedData, staticData->first().x());
      if (lowBound == -1) return QPolygonF(); // key ranges have no overlap
      croppedData->remove(0, lowBound);
      // set lowest point of cropped data to fit exactly key position of first static data point via linear interpolation:
      if (croppedData->size() < 2) return QPolygonF(); // need at least two points for interpolation
      double slope;
      if (!qFuzzyCompare(croppedData->at(1).x(), croppedData->at(0).x()))
        slope = (croppedData->at(1).y()-croppedData->at(0).y())/(croppedData->at(1).x()-croppedData->at(0).x());
      else
        slope = 0;
      (*croppedData)[0].setY(croppedData->at(0).y()+slope*(staticData->first().x()-croppedData->at(0).x()));
      (*croppedData)[0].setX(staticData->first().x());

      // crop upper bound:
      if (staticData->last().x() > croppedData->last().x()) // other one must be cropped
        qSwap(staticData, croppedData);
      int highBound = findIndexAboveX(croppedData, staticData->last().x());
      if (highBound == -1) return QPolygonF(); // key ranges have no overlap
      croppedData->remove(highBound+1, croppedData->size()-(highBound+1));
      // set highest point of cropped data to fit exactly key position of last static data point via linear interpolation:
      if (croppedData->size() < 2) return QPolygonF(); // need at least two points for interpolation
      const int li = croppedData->size()-1; // last index
      if (!qFuzzyCompare(croppedData->at(li).x(), croppedData->at(li-1).x()))
        slope = (croppedData->at(li).y()-croppedData->at(li-1).y())/(croppedData->at(li).x()-croppedData->at(li-1).x());
      else
        slope = 0;
      (*croppedData)[li].setY(croppedData->at(li-1).y()+slope*(staticData->last().x()-croppedData->at(li-1).x()));
      (*croppedData)[li].setX(staticData->last().x());
    } else // mKeyAxis->orientation() == Qt::Vertical
    {
      // y is key
      // crop lower bound:
      if (staticData->first().y() < croppedData->first().y()) // other one must be cropped
        qSwap(staticData, croppedData);
      int lowBound = findIndexBelowY(croppedData, staticData->first().y());
      if (lowBound == -1) return QPolygonF(); // key ranges have no overlap
      croppedData->remove(0, lowBound);
      // set lowest point of cropped data to fit exactly key position of first static data point via linear interpolation:
      if (croppedData->size() < 2) return QPolygonF(); // need at least two points for interpolation
      double slope;
      if (!qFuzzyCompare(croppedData->at(1).y(), croppedData->at(0).y())) // avoid division by zero in step plots
        slope = (croppedData->at(1).x()-croppedData->at(0).x())/(croppedData->at(1).y()-croppedData->at(0).y());
      else
        slope = 0;
      (*croppedData)[0].setX(croppedData->at(0).x()+slope*(staticData->first().y()-croppedData->at(0).y()));
      (*croppedData)[0].setY(staticData->first().y());

      // crop upper bound:
      if (staticData->last().y() > croppedData->last().y()) // other one must be cropped
        qSwap(staticData, croppedData);
      int highBound = findIndexAboveY(croppedData, staticData->last().y());
      if (highBound == -1) return QPolygonF(); // key ranges have no overlap
      croppedData->remove(highBound+1, croppedData->size()-(highBound+1));
      // set highest point of cropped data to fit exactly key position of last static data point via linear interpolation:
      if (croppedData->size() < 2) return QPolygonF(); // need at least two points for interpolation
      int li = croppedData->size()-1; // last index
      if (!qFuzzyCompare(croppedData->at(li).y(), croppedData->at(li-1).y())) // avoid division by zero in step plots
        slope = (croppedData->at(li).x()-croppedData->at(li-1).x())/(croppedData->at(li).y()-croppedData->at(li-1).y());
      else
        slope = 0;
      (*croppedData)[li].setX(croppedData->at(li-1).x()+slope*(staticData->last().y()-croppedData->at(li-1).y()));
      (*croppedData)[li].setY(staticData->last().y());
    }

    // return joined:
    for (int i=otherSegmentData.size()-1; i>=0; --i) // insert reversed, otherwise the polygon will be twisted
      thisSegmentData << otherSegmentData.at(i);
    return QPolygonF(thisSegmentData);
}

int Graph2D::findIndexBelowX(const QVector<QPointF> *data, double x) const
{DD;
    for (int i=0; i<data->size(); ++i)
    {
      if (data->at(i).x() > x)
      {
        if (i>0)
          return i-1;
        else
          return 0;
      }
    }
    return -1;
}

int Graph2D::findIndexAboveX(const QVector<QPointF> *data, double x) const
{DD;
    for (int i=data->size()-1; i>=0; --i)
    {
      if (data->at(i).x() < x)
      {
        if (i<data->size()-1)
          return i+1;
        else
          return data->size()-1;
      }
    }
    return -1;
}

int Graph2D::findIndexBelowY(const QVector<QPointF> *data, double y) const
{DD;
    for (int i=0; i<data->size(); ++i)
    {
      if (data->at(i).y() > y)
      {
        if (i>0)
          return i-1;
        else
          return 0;
      }
    }
    return -1;
}

int Graph2D::findIndexAboveY(const QVector<QPointF> *data, double y) const
{DD;
    for (int i=data->size()-1; i>=0; --i)
    {
      if (data->at(i).y() < y)
      {
        if (i<data->size()-1)
          return i+1;
        else
          return data->size()-1;
      }
    }
    return -1;
}

double Graph2D::pointDistance(const QPointF &pixelPoint, int &closestData) const
{DD;
    closestData = m_data->size();
    if (m_data->isEmpty())
      return -1.0;
    if (mLineStyle == lsNone && mScatterStyle.isNone())
      return -1.0;

    // calculate minimum distances to graph data points and find closestData iterator:
    double minDistSqr = (std::numeric_limits<double>::max)();
    // determine which key range comes into question, taking selection tolerance around pos into account:
    double posKeyMin, posKeyMax, dummy;
    pixelsToCoords(pixelPoint-QPointF(mParentPlot->selectionTolerance(), mParentPlot->selectionTolerance()), posKeyMin, dummy);
    pixelsToCoords(pixelPoint+QPointF(mParentPlot->selectionTolerance(), mParentPlot->selectionTolerance()), posKeyMax, dummy);
    if (posKeyMin > posKeyMax)
      qSwap(posKeyMin, posKeyMax);
    // iterate over found data points and then choose the one with the shortest distance to pos:
    auto begin = m_data->findBegin(posKeyMin, true);
    auto end = m_data->findEnd(posKeyMax, true);
    for (auto it = begin; it!=end; ++it)
    {
      const double currentDistSqr = QCPVector2D(coordsToPixels(m_data->mainKey(it), m_data->mainValue(it))-pixelPoint).lengthSquared();
      if (currentDistSqr < minDistSqr)
      {
        minDistSqr = currentDistSqr;
        closestData = it;
      }
    }

    // calculate distance to graph line if there is one (if so, will probably be smaller than distance to closest data point):
    if (mLineStyle != lsNone)
    {
      // line displayed, calculate distance to line segments:
      QVector<QPointF> lineData;
      getLines(&lineData, QCPDataRange(0, dataCount())); // don't limit data range further since with sharp data spikes, line segments may be closer to test point than segments with closer key coordinate
      QCPVector2D p(pixelPoint);
      for (int i=0; i<lineData.size()-1; i++)
      {
        const double currentDistSqr = p.distanceSquaredToLine(lineData.at(i), lineData.at(i+1));
        if (currentDistSqr < minDistSqr)
          minDistSqr = currentDistSqr;
      }
    }

    return qSqrt(minDistSqr);
}

bool Graph2D::isVisible() const
{DD;
    return visible();
}

void Graph2D::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{DD;
    // draw fill:
    if (mBrush.style() != Qt::NoBrush)
    {
      applyFillAntialiasingHint(painter);
      painter->fillRect(QRectF(rect.left(), rect.top()+rect.height()/2.0, rect.width(), rect.height()/3.0), mBrush);
    }
    // draw line vertically centered:
    if (mLineStyle != lsNone)
    {
      applyDefaultAntialiasingHint(painter);
      auto p = mPen;
      if (p.width()<2) p.setWidth(2);
      painter->setPen(p);
      painter->drawLine(QLineF(rect.left(), rect.top()+rect.height()/2.0, rect.right()+5, rect.top()+rect.height()/2.0)); // +5 on x2 else last segment is missing from dashed/dotted pens
    }
    // draw scatter symbol:
    if (!mScatterStyle.isNone())
    {
      applyScattersAntialiasingHint(painter);
      // scale scatter pixmap if it's too large to fit in legend icon rect:
      if (mScatterStyle.shape() == QCPScatterStyle::ssPixmap && (mScatterStyle.pixmap().size().width() > rect.width() || mScatterStyle.pixmap().size().height() > rect.height()))
      {
        QCPScatterStyle scaledStyle(mScatterStyle);
        scaledStyle.setPixmap(scaledStyle.pixmap().scaled(rect.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        scaledStyle.applyTo(painter, mPen);
        scaledStyle.drawShape(painter, QRectF(rect).center());
      } else
      {
        mScatterStyle.applyTo(painter, mPen);
        mScatterStyle.drawShape(painter, QRectF(rect).center());
      }
    }
}

void Graph2D::draw(QCPPainter *painter)
{DD;
    if (!mKeyAxis || !mValueAxis) { LOG(ERROR) << Q_FUNC_INFO << "invalid key or value axis"; return; }
    if (mKeyAxis.data()->range().size() <= 0 || m_data->isEmpty()) return;
    if (mLineStyle == lsNone && mScatterStyle.isNone()) return;

    QVector<QPointF> lines, scatters; // line and (if necessary) scatter pixel coordinates will be stored here while iterating over segments

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

      // draw fill of graph:
      if (isSelectedSegment && mSelectionDecorator)
        mSelectionDecorator->applyBrush(painter);
      else
        painter->setBrush(mBrush);
      painter->setPen(Qt::NoPen);
      drawFill(painter, &lines);

      // draw line:
      if (mLineStyle != lsNone)
      {
        if (isSelectedSegment && mSelectionDecorator)
          mSelectionDecorator->applyPen(painter);
        else
          painter->setPen(mPen);
        painter->setBrush(Qt::NoBrush);
        drawLinePlot(painter, lines); // also step plots can be drawn as a line plot
      }

      // draw scatters:
      QCPScatterStyle finalScatterStyle = mScatterStyle;
      if (isSelectedSegment && mSelectionDecorator)
        finalScatterStyle = mSelectionDecorator->getFinalScatterStyle(mScatterStyle);
      if (!finalScatterStyle.isNone())
      {
        getScatters(&scatters, allSegments.at(i));
        drawScatterPlot(painter, scatters, finalScatterStyle);
      }
    }

    // draw other selection decoration that isn't just line/scatter pens and brushes:
    if (mSelectionDecorator)
      mSelectionDecorator->drawDecoration(painter, selection());
}

int Graph2D::dataCount() const
{DD;
    return m_data->size();
}

double Graph2D::dataMainKey(int index) const
{DD;
    return m_data->mainKey(index);
}

double Graph2D::dataSortKey(int index) const
{DD;
    return m_data->sortKey(index);
}

double Graph2D::dataMainValue(int index) const
{DD;
    return m_data->mainValue(index);
}

QCPRange Graph2D::dataValueRange(int index) const
{DD;
    if (index >= 0 && index < m_data->size())
    {
        return m_data->valueRange(index);
    } else
    {
        LOG(ERROR) << Q_FUNC_INFO << "Index out of bounds" << index;
        return QCPRange(0, 0);
    }
}

QPointF Graph2D::dataPixelPosition(int index) const
{DD;
    if (index >= 0 && index < m_data->size())
    {
      return coordsToPixels(m_data->mainKey(index), m_data->mainValue(index));
    } else
    {
      LOG(ERROR) << Q_FUNC_INFO << "Index out of bounds" << index;
      return QPointF();
    }
}

bool Graph2D::sortKeyIsMainKey() const
{DD;
    return true;
}

QCPDataSelection Graph2D::selectTestRect(const QRectF &rect, bool onlySelectable) const
{DD;
    QCPDataSelection result;
    if ((onlySelectable && mSelectable == QCP::stNone) || m_data->isEmpty())
      return result;
    if (!mKeyAxis || !mValueAxis)
      return result;

    // convert rect given in pixels to ranges given in plot coordinates:
    double key1, value1, key2, value2;
    pixelsToCoords(rect.topLeft(), key1, value1);
    pixelsToCoords(rect.bottomRight(), key2, value2);
    QCPRange keyRange(key1, key2); // QCPRange normalizes internally so we don't have to care about whether key1 < key2
    QCPRange valueRange(value1, value2);

    int  begin = m_data->findBegin(keyRange.lower, false);
    int  end = m_data->findEnd(keyRange.upper, false);

    if (begin == end)
      return result;

    int currentSegmentBegin = -1; // -1 means we're currently not in a segment that's contained in rect
    for (int it=begin; it!=end; ++it)
    {
      if (currentSegmentBegin == -1)
      {
        if (valueRange.contains(m_data->mainValue(it)) && keyRange.contains(m_data->mainKey(it))) // start segment
          currentSegmentBegin = it;
      } else if (!valueRange.contains(m_data->mainValue(it)) || !keyRange.contains(m_data->mainKey(it))) // segment just ended
      {
        result.addDataRange(QCPDataRange(currentSegmentBegin, it), false);
        currentSegmentBegin = -1;
      }
    }
    // process potential last segment:
    if (currentSegmentBegin != -1)
      result.addDataRange(QCPDataRange(currentSegmentBegin, end), false);

    result.simplify();
    return result;
}

int Graph2D::findBegin(double sortKey, bool expandedRange) const
{DD;
    return m_data->findBegin(sortKey, expandedRange);
}

int Graph2D::findEnd(double sortKey, bool expandedRange) const
{DD;
    return m_data->findEnd(sortKey, expandedRange);
}


void Graph2D::attachTo(Plot *plot)
{DD;
    Curve::attachTo(plot);
    plot->impl()->checkableLegend->addItem(this, commonLegendData());
}

void Graph2D::detachFrom(Plot *plot)
{DD;
    Curve::detachFrom(plot);
    plot->impl()->checkableLegend->removeItem(this);
    plot->impl()->removePlottable(this, false);
    plot->impl()->replot();
}

QString Graph2D::title() const
{DD;
    return name();
}

void Graph2D::setTitle(const QString &title)
{DD;
    setName(title);
}

Enums::AxisType Graph2D::yAxis() const
{DD;
    if (auto ax = this->valueAxis()) return static_cast<Enums::AxisType>(ax->axisType());
    return Enums::AxisType::atInvalid;
}

void Graph2D::setYAxis(Enums::AxisType axis)
{DD;
    auto ax = parentPlot()->axisRect(0)->axis(static_cast<QCPAxis::AxisType>(axis));
    setValueAxis(ax);

//    foreach (PointLabel *l, labels)
//        l->setYAxis(toQwtAxisType(axis));
}

Enums::AxisType Graph2D::xAxis() const
{DD;
    if (auto ax = this->keyAxis()) return static_cast<Enums::AxisType>(ax->axisType());
    return Enums::AxisType::atInvalid;
}

void Graph2D::setXAxis(Enums::AxisType axis)
{DD;
    auto ax = parentPlot()->axisRect(0)->axis(static_cast<QCPAxis::AxisType>(axis));
    setKeyAxis(ax);

//    foreach (PointLabel *l, labels)
//        l->setXAxis(toQwtAxisType(axis));
}

QPen Graph2D::pen() const
{DD;
//    return QCPAbstractPlottable::pen();
    return oldPen;
}

SamplePoint Graph2D::samplePoint(SelectedPoint point) const
{DD;
    return {m_data->mainKey(point.x), m_data->mainValue(point.x), qQNaN()};
}

SelectedPoint Graph2D::closest(const QPoint &pos, double *dist1, double *dist2) const
{DD;
    int index = -1;

    const size_t numSamples = channel->data()->samplesCount();
    if ( numSamples <= 0 )
        return {-1, -1};

    int from = 0;
    int to = numSamples-1;
    auto range = keyAxis()->range();
    evaluateScale(from, to, range.lower, range.upper);

    double dmin = qInf();

    for ( int i = from; i <= to; i++ ) {
        const auto sample = samplePoint( {i,0} );

        const double cx = qAbs(keyAxis()->coordToPixel(sample.x) - pos.x());
        const double cy = qAbs(valueAxis()->coordToPixel(sample.y) - pos.y());

        const double f = cx*cx + cy*cy;
        if ( f < dmin ) {
            index = i;
            dmin = f;
            if (dist1) *dist1 = cx;
            if (dist2) *dist2 = cy;
        }
    }

    return {index, 0};
}

LegendData Graph2D::commonLegendData() const
{DD;
    auto data = Curve::commonLegendData();
    data.checked = visible();
    return data;
}

void Graph2D::updateScatter()
{DD;
    QCPScatterStyle ss;
    ss.setSize(m_markerSize);
    ss.setShape(static_cast<QCPScatterStyle::ScatterShape>(m_markerShape));
    QPen p;
    p.setColor(pen().color());
    p.setWidthF(pen().widthF());
    ss.setPen(p);

    setScatterStyle(ss);
//    setScatterSkip(10);
}

void Graph2D::updatePen()
{DD;
    auto p = oldPen;
    if (p.style() == Qt::NoPen) setLineStyle(lsNone);
    else setLineStyle(lineStyleByType(channel));
    if (Curve::selected()) p.setWidth(2);
    QCPAbstractPlottable::setPen(p);
    updateScatter();
    if (m_plot) m_plot->impl()->checkableLegend->updateItem(this, commonLegendData());
}

QIcon Graph2D::thumbnail() const
{DD;
    QPixmap pix(16,10);
    QCPPainter p(&pix);
    p.fillRect(0,0,16,10, Qt::white);
    drawLegendIcon(&p, pix.rect());
    return QIcon(pix);
}
