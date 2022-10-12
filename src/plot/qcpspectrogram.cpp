#include "qcpspectrogram.h"

#include "fileformats/filedescriptor.h"
#include "qcpplot.h"
#include "plot/plot.h"
#include "checkablelegend.h"

QCPSpectrogram::QCPSpectrogram(const QString &title, Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis)
    : QCPAbstractPlottable(keyAxis, valueAxis), Curve(title, channel)
{
    m_data = new Data3D(channel->data());
    mMapImageInvalidated = true;

    setName(channel->legendName());

    setInterpolate(false);
    setTightBoundary(true);
    setSelectable(QCP::stSingleData);
    type = Curve::Type::Spectrogram;
}

QCPSpectrogram::~QCPSpectrogram()
{
    delete m_data;
}

void QCPSpectrogram::attachTo(Plot *plot)
{
    Curve::attachTo(plot);
    Curve::addToLegend(plot->impl()->checkableLegend);
}

void QCPSpectrogram::detachFrom(Plot *plot)
{
    Curve::detachFrom(plot);
    plot->impl()->checkableLegend->removeItem(this);
    plot->impl()->removePlottable(this, false);
    plot->impl()->replot();
}

QString QCPSpectrogram::title() const
{
    return name();
}

LegendData QCPSpectrogram::commonLegendData() const
{
    auto data = Curve::commonLegendData();
    data.checked = visible();
    return data;
}

void QCPSpectrogram::setTitle(const QString &title)
{
    setName(title);
}

Enums::AxisType QCPSpectrogram::yAxis() const
{
    if (auto ax = this->valueAxis()) return static_cast<Enums::AxisType>(ax->axisType());
    return Enums::AxisType::atInvalid;
}

void QCPSpectrogram::setYAxis(Enums::AxisType axis)
{
    auto ax = parentPlot()->axisRect(0)->axis(static_cast<QCPAxis::AxisType>(axis));
    setValueAxis(ax);

//    foreach (PointLabel *l, labels)
//        l->setYAxis(toQwtAxisType(axis));
}

Enums::AxisType QCPSpectrogram::xAxis() const
{
    if (auto ax = this->keyAxis()) return static_cast<Enums::AxisType>(ax->axisType());
    return Enums::AxisType::atInvalid;
}

void QCPSpectrogram::setXAxis(Enums::AxisType axis)
{
    auto ax = parentPlot()->axisRect(0)->axis(static_cast<QCPAxis::AxisType>(axis));
    setKeyAxis(ax);

//    foreach (PointLabel *l, labels)
//        l->setXAxis(toQwtAxisType(axis));
}

QPen QCPSpectrogram::pen() const
{
    return QCPAbstractPlottable::pen();
}

SamplePoint QCPSpectrogram::samplePoint(SelectedPoint point) const
{
    return {channel->data()->xValue(point.x),
                channel->data()->yValue(point.x, point.z),
                channel->data()->zValue(point.z)};
}

SelectedPoint QCPSpectrogram::closest(const QPoint &pos, double *dist, double *dist2) const
{
    SelectedPoint p;
    const size_t numSamples = channel->data()->samplesCount();
    if (numSamples <= 0) return p;

    auto xIndex = channel->data()->nearest(keyAxis()->pixelToCoord(pos.x()));
    auto zIndex = channel->data()->nearestZ(valueAxis()->pixelToCoord(pos.y()));

    if (xIndex == -1 || zIndex == -1) return p;

    if (dist) *dist = qAbs(pos.x() - keyAxis()->coordToPixel(channel->data()->xValue(xIndex)));
    if (dist2) *dist2 = qAbs(pos.y() - valueAxis()->coordToPixel(channel->data()->zValue(zIndex)));

    p.x = xIndex;
    p.z = zIndex;
    return p;
}

void QCPSpectrogram::updatePen()
{
    m_plot->impl()->checkableLegend->updateItem(this, commonLegendData());
}

void QCPSpectrogram::setDataRange(const QCPRange &dataRange)
{
  if (!QCPRange::validRange(dataRange)) return;
  if (mDataRange.lower != dataRange.lower || mDataRange.upper != dataRange.upper)
  {
    if (mDataScaleType == QCPAxis::stLogarithmic)
      mDataRange = dataRange.sanitizedForLogScale();
    else
      mDataRange = dataRange.sanitizedForLinScale();
    mMapImageInvalidated = true;
    emit dataRangeChanged(mDataRange);
  }
}

void QCPSpectrogram::setDataScaleType(QCPAxis::ScaleType scaleType)
{
  if (mDataScaleType != scaleType)
  {
    mDataScaleType = scaleType;
    mMapImageInvalidated = true;
    emit dataScaleTypeChanged(mDataScaleType);
    if (mDataScaleType == QCPAxis::stLogarithmic)
      setDataRange(mDataRange.sanitizedForLogScale());
  }
}

void QCPSpectrogram::setGradient(const QCPColorGradient &gradient)
{
  if (mGradient != gradient)
  {
    mGradient = gradient;
    mMapImageInvalidated = true;
    emit gradientChanged(mGradient);
  }
}

void QCPSpectrogram::setInterpolate(bool enabled)
{
  mInterpolate = enabled;
  mMapImageInvalidated = true; // because oversampling factors might need to change
}

void QCPSpectrogram::setTightBoundary(bool enabled)
{
  mTightBoundary = enabled;
}

void QCPSpectrogram::setColorScale(QCPColorScale *colorScale)
{
  if (mColorScale) // unconnect signals from old color scale
  {
    disconnect(this, SIGNAL(dataRangeChanged(QCPRange)), mColorScale.data(), SLOT(setDataRange(QCPRange)));
    disconnect(this, SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), mColorScale.data(), SLOT(setDataScaleType(QCPAxis::ScaleType)));
    disconnect(this, SIGNAL(gradientChanged(QCPColorGradient)), mColorScale.data(), SLOT(setGradient(QCPColorGradient)));
    disconnect(mColorScale.data(), SIGNAL(dataRangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
    disconnect(mColorScale.data(), SIGNAL(gradientChanged(QCPColorGradient)), this, SLOT(setGradient(QCPColorGradient)));
    disconnect(mColorScale.data(), SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
  }
  mColorScale = colorScale;
  if (mColorScale) // connect signals to new color scale
  {
    setGradient(mColorScale.data()->gradient());
    setDataRange(mColorScale.data()->dataRange());
    setDataScaleType(mColorScale.data()->dataScaleType());
    connect(this, SIGNAL(dataRangeChanged(QCPRange)), mColorScale.data(), SLOT(setDataRange(QCPRange)));
    connect(this, SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), mColorScale.data(), SLOT(setDataScaleType(QCPAxis::ScaleType)));
    connect(this, SIGNAL(gradientChanged(QCPColorGradient)), mColorScale.data(), SLOT(setGradient(QCPColorGradient)));
    connect(mColorScale.data(), SIGNAL(dataRangeChanged(QCPRange)), this, SLOT(setDataRange(QCPRange)));
    connect(mColorScale.data(), SIGNAL(gradientChanged(QCPColorGradient)), this, SLOT(setGradient(QCPColorGradient)));
    connect(mColorScale.data(), SIGNAL(dataScaleTypeChanged(QCPAxis::ScaleType)), this, SLOT(setDataScaleType(QCPAxis::ScaleType)));
  }
}

void QCPSpectrogram::rescaleDataRange(bool recalculateDataBounds)
{
  if (recalculateDataBounds)
    m_data->recalculateDataBounds();
  setDataRange(m_data->dataBounds());
}

void QCPSpectrogram::updateLegendIcon(Qt::TransformationMode transformMode, const QSize &thumbSize)
{
  if (mMapImage.isNull() && !m_data->isEmpty())
    updateMapImage(); // try to update map image if it's null (happens if no draw has happened yet)

  if (!mMapImage.isNull()) // might still be null, e.g. if data is empty, so check here again
  {
    bool mirrorX = (keyAxis()->orientation() == Qt::Horizontal ? keyAxis() : valueAxis())->rangeReversed();
    bool mirrorY = (valueAxis()->orientation() == Qt::Vertical ? valueAxis() : keyAxis())->rangeReversed();
    mLegendIcon = QPixmap::fromImage(mMapImage.mirrored(mirrorX, mirrorY)).scaled(thumbSize, Qt::KeepAspectRatio, transformMode);
  }
}

double QCPSpectrogram::selectTest(const QPointF &pos, bool onlySelectable, QVariant *details) const
{
  Q_UNUSED(details)
  if ((onlySelectable && mSelectable == QCP::stNone) || m_data->isEmpty())
    return -1;
  if (!mKeyAxis || !mValueAxis)
    return -1;

  if (mKeyAxis.data()->axisRect()->rect().contains(pos.toPoint()) || mParentPlot->interactions().testFlag(QCP::iSelectPlottablesBeyondAxisRect))
  {
    double posKey, posValue;
    pixelsToCoords(pos, posKey, posValue);
    if (m_data->keyRange().contains(posKey) && m_data->valueRange().contains(posValue))
    {
      if (details)
        details->setValue(QCPDataSelection(QCPDataRange(0, 1))); // temporary solution, to facilitate whole-plottable selection. Replace in future version with segmented 2D selection.
      return mParentPlot->selectionTolerance()*0.99;
    }
  }
  return -1;
}

/* inherits documentation from base class */
QCPRange QCPSpectrogram::getKeyRange(bool &foundRange, QCP::SignDomain inSignDomain) const
{
  foundRange = true;
  QCPRange result = m_data->keyRange();
  result.normalize();
  if (inSignDomain == QCP::sdPositive)
  {
    if (result.lower <= 0 && result.upper > 0)
      result.lower = result.upper*1e-3;
    else if (result.lower <= 0 && result.upper <= 0)
      foundRange = false;
  } else if (inSignDomain == QCP::sdNegative)
  {
    if (result.upper >= 0 && result.lower < 0)
      result.upper = result.lower*1e-3;
    else if (result.upper >= 0 && result.lower >= 0)
      foundRange = false;
  }
  return result;
}

/* inherits documentation from base class */
QCPRange QCPSpectrogram::getValueRange(bool &foundRange, QCP::SignDomain inSignDomain, const QCPRange &inKeyRange) const
{
  if (inKeyRange != QCPRange())
  {
    if (m_data->keyRange().upper < inKeyRange.lower || m_data->keyRange().lower > inKeyRange.upper)
    {
      foundRange = false;
      return {};
    }
  }

  foundRange = true;
  QCPRange result = m_data->valueRange();
  result.normalize();
  if (inSignDomain == QCP::sdPositive)
  {
    if (result.lower <= 0 && result.upper > 0)
      result.lower = result.upper*1e-3;
    else if (result.lower <= 0 && result.upper <= 0)
      foundRange = false;
  } else if (inSignDomain == QCP::sdNegative)
  {
    if (result.upper >= 0 && result.lower < 0)
      result.upper = result.lower*1e-3;
    else if (result.upper >= 0 && result.lower >= 0)
      foundRange = false;
  }
  return result;
}

void QCPSpectrogram::updateMapImage()
{
  QCPAxis *keyAxis = mKeyAxis.data();
  if (!keyAxis) return;
  if (m_data->isEmpty()) return;

  const QImage::Format format = QImage::Format_ARGB32_Premultiplied;
  const int keySize = m_data->keySize();
  const int valueSize = m_data->valueSize();
  int keyOversamplingFactor = mInterpolate ? 1 : int(1.0+100.0/double(keySize)); // make mMapImage have at least size 100, factor becomes 1 if size > 200 or interpolation is on
  int valueOversamplingFactor = mInterpolate ? 1 : int(1.0+100.0/double(valueSize)); // make mMapImage have at least size 100, factor becomes 1 if size > 200 or interpolation is on

  // resize mMapImage to correct dimensions including possible oversampling factors, according to key/value axes orientation:
  if (keyAxis->orientation() == Qt::Horizontal && (mMapImage.width() != keySize*keyOversamplingFactor || mMapImage.height() != valueSize*valueOversamplingFactor))
    mMapImage = QImage(QSize(keySize*keyOversamplingFactor, valueSize*valueOversamplingFactor), format);
  else if (keyAxis->orientation() == Qt::Vertical && (mMapImage.width() != valueSize*valueOversamplingFactor || mMapImage.height() != keySize*keyOversamplingFactor))
    mMapImage = QImage(QSize(valueSize*valueOversamplingFactor, keySize*keyOversamplingFactor), format);

  if (mMapImage.isNull())
  {
    qDebug() << Q_FUNC_INFO << "Couldn't create map image (possibly too large for memory)";
    mMapImage = QImage(QSize(10, 10), format);
    mMapImage.fill(Qt::black);
  } else
  {
    QImage *localMapImage = &mMapImage; // this is the image on which the colorization operates. Either the final mMapImage, or if we need oversampling, mUndersampledMapImage
    if (keyOversamplingFactor > 1 || valueOversamplingFactor > 1)
    {
      // resize undersampled map image to actual key/value cell sizes:
      if (keyAxis->orientation() == Qt::Horizontal && (mUndersampledMapImage.width() != keySize || mUndersampledMapImage.height() != valueSize))
        mUndersampledMapImage = QImage(QSize(keySize, valueSize), format);
      else if (keyAxis->orientation() == Qt::Vertical && (mUndersampledMapImage.width() != valueSize || mUndersampledMapImage.height() != keySize))
        mUndersampledMapImage = QImage(QSize(valueSize, keySize), format);
      localMapImage = &mUndersampledMapImage; // make the colorization run on the undersampled image
    } else if (!mUndersampledMapImage.isNull())
      mUndersampledMapImage = QImage(); // don't need oversampling mechanism anymore (map size has changed) but mUndersampledMapImage still has nonzero size, free it

    if (keyAxis->orientation() == Qt::Horizontal)
    {
      const int blocksCount = valueSize;
      const int samplesCount = keySize;
      for (int line=0; line<blocksCount; ++line)
      {
        QRgb* pixels = reinterpret_cast<QRgb*>(localMapImage->scanLine(blocksCount-1-line)); // invert scanline index because QImage counts scanlines from top, but our vertical index counts from bottom (mathematical coordinate system)
//        mGradient.colorize(rawData + line*samplesCount, mDataRange, pixels, samplesCount, 1, mDataScaleType==QCPAxis::stLogarithmic);
        mGradient.colorize(m_data, mDataRange, pixels, line, samplesCount, 1, mDataScaleType==QCPAxis::stLogarithmic);
      }
    }
//    else{// keyAxis->orientation() == Qt::Vertical
//      const int lineCount = keySize;
//      const int rowCount = valueSize;
//      for (int line=0; line<lineCount; ++line)
//      {
//        QRgb* pixels = reinterpret_cast<QRgb*>(localMapImage->scanLine(lineCount-1-line)); // invert scanline index because QImage counts scanlines from top, but our vertical index counts from bottom (mathematical coordinate system)
//        mGradient.colorize(rawData+line, mDataRange, pixels, rowCount, lineCount, mDataScaleType==QCPAxis::stLogarithmic);
//      }
//    }

    if (keyOversamplingFactor > 1 || valueOversamplingFactor > 1)
    {
      if (keyAxis->orientation() == Qt::Horizontal)
        mMapImage = mUndersampledMapImage.scaled(keySize*keyOversamplingFactor, valueSize*valueOversamplingFactor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
      else
        mMapImage = mUndersampledMapImage.scaled(valueSize*valueOversamplingFactor, keySize*keyOversamplingFactor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
  }
  m_data->mDataModified = false;
  mMapImageInvalidated = false;
}

/* inherits documentation from base class */
void QCPSpectrogram::draw(QCPPainter *painter)
{
  if (m_data->isEmpty()) return;
  if (!mKeyAxis || !mValueAxis) return;
  applyDefaultAntialiasingHint(painter);

  if (m_data->mDataModified || mMapImageInvalidated)
    updateMapImage();

  // use buffer if painting vectorized (PDF):
  const bool useBuffer = painter->modes().testFlag(QCPPainter::pmVectorized);
  QCPPainter *localPainter = painter; // will be redirected to paint on mapBuffer if painting vectorized
  QRectF mapBufferTarget; // the rect in absolute widget coordinates where the visible map portion/buffer will end up in
  QPixmap mapBuffer;
  if (useBuffer)
  {
    const double mapBufferPixelRatio = 3; // factor by which DPI is increased in embedded bitmaps
    mapBufferTarget = painter->clipRegion().boundingRect();
    mapBuffer = QPixmap((mapBufferTarget.size()*mapBufferPixelRatio).toSize());
    mapBuffer.fill(Qt::transparent);
    localPainter = new QCPPainter(&mapBuffer);
    localPainter->scale(mapBufferPixelRatio, mapBufferPixelRatio);
    localPainter->translate(-mapBufferTarget.topLeft());
  }

  QRectF imageRect = QRectF(coordsToPixels(m_data->keyRange().lower, m_data->valueRange().lower),
                            coordsToPixels(m_data->keyRange().upper, m_data->valueRange().upper)).normalized();
  // extend imageRect to contain outer halves/quarters of bordering/cornering pixels (cells are centered on map range boundary):
  double halfCellWidth = 0; // in pixels
  double halfCellHeight = 0; // in pixels
  if (keyAxis()->orientation() == Qt::Horizontal)
  {
    if (m_data->keySize() > 1)
      halfCellWidth = 0.5*imageRect.width()/double(m_data->keySize()-1);
    if (m_data->valueSize() > 1)
      halfCellHeight = 0.5*imageRect.height()/double(m_data->valueSize()-1);
  } else // keyAxis orientation is Qt::Vertical
  {
    if (m_data->keySize() > 1)
      halfCellHeight = 0.5*imageRect.height()/double(m_data->keySize()-1);
    if (m_data->valueSize() > 1)
      halfCellWidth = 0.5*imageRect.width()/double(m_data->valueSize()-1);
  }
  imageRect.adjust(-halfCellWidth, -halfCellHeight, halfCellWidth, halfCellHeight);
  const bool mirrorX = (keyAxis()->orientation() == Qt::Horizontal ? keyAxis() : valueAxis())->rangeReversed();
  const bool mirrorY = (valueAxis()->orientation() == Qt::Vertical ? valueAxis() : keyAxis())->rangeReversed();
  const bool smoothBackup = localPainter->renderHints().testFlag(QPainter::SmoothPixmapTransform);
  localPainter->setRenderHint(QPainter::SmoothPixmapTransform, mInterpolate);
  QRegion clipBackup;
  if (mTightBoundary)
  {
    clipBackup = localPainter->clipRegion();
    QRectF tightClipRect = QRectF(coordsToPixels(m_data->keyRange().lower, m_data->valueRange().lower),
                                  coordsToPixels(m_data->keyRange().upper, m_data->valueRange().upper)).normalized();
    localPainter->setClipRect(tightClipRect, Qt::IntersectClip);
  }
  localPainter->drawImage(imageRect, mMapImage.mirrored(mirrorX, mirrorY));
  if (mTightBoundary)
    localPainter->setClipRegion(clipBackup);
  localPainter->setRenderHint(QPainter::SmoothPixmapTransform, smoothBackup);

  if (useBuffer) // localPainter painted to mapBuffer, so now draw buffer with original painter
  {
    delete localPainter;
    painter->drawPixmap(mapBufferTarget.toRect(), mapBuffer);
  }
}

/* inherits documentation from base class */
void QCPSpectrogram::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{
  applyDefaultAntialiasingHint(painter);
  // draw map thumbnail:
  if (!mLegendIcon.isNull())
  {
    QPixmap scaledIcon = mLegendIcon.scaled(rect.size().toSize(), Qt::KeepAspectRatio, Qt::FastTransformation);
    QRectF iconRect = QRectF(0, 0, scaledIcon.width(), scaledIcon.height());
    iconRect.moveCenter(rect.center());
    painter->drawPixmap(iconRect.topLeft(), scaledIcon);
  }
  /*
  // draw frame:
  painter->setBrush(Qt::NoBrush);
  painter->setPen(Qt::black);
  painter->drawRect(rect.adjusted(1, 1, 0, 0));
  */
}
