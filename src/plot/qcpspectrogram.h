#ifndef QCPSPECTROGRAM_H
#define QCPSPECTROGRAM_H

#include "qcustomplot.h"
#include "data3d.h"
#include "plot/curve.h"

class Channel;

class QCPSpectrogram : public QCPAbstractPlottable, public Curve
{
    Q_OBJECT
    /// \cond INCLUDE_QPROPERTIES
    Q_PROPERTY(QCPRange dataRange READ dataRange WRITE setDataRange NOTIFY dataRangeChanged)
    Q_PROPERTY(QCPAxis::ScaleType dataScaleType READ dataScaleType WRITE setDataScaleType NOTIFY dataScaleTypeChanged)
    Q_PROPERTY(QCPColorGradient gradient READ gradient WRITE setGradient NOTIFY gradientChanged)
    Q_PROPERTY(bool tightBoundary READ tightBoundary WRITE setTightBoundary)
    Q_PROPERTY(QCPColorScale* colorScale READ colorScale WRITE setColorScale)
    /// \endcond
  public:
    explicit QCPSpectrogram(const QString &title, Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis);
    virtual ~QCPSpectrogram() Q_DECL_OVERRIDE;

    // Curve interface
public:
    virtual bool isVisible() const override;
    virtual void attachTo(Plot *plot) override;
    virtual void detachFrom(Plot *plot) override;
    virtual QString title() const override;
    virtual void setTitle(const QString &title) override;
    virtual Enums::AxisType yAxis() const override;
    virtual void setYAxis(Enums::AxisType axis) override;
    virtual Enums::AxisType xAxis() const override;
    virtual void setXAxis(Enums::AxisType axis) override;
    virtual QPen pen() const override;
    virtual SamplePoint samplePoint(SelectedPoint point) const override;
    virtual SelectedPoint closest(const QPoint &pos, double *dist, double *dist2) const override;
    virtual LegendData commonLegendData() const override;
    virtual void updatePen() override;
    virtual QIcon thumbnail() const override;
private:
    Data3D *m_data;
public:
    // getters:
    QCPRange dataRange() const { return mDataRange; }
    QCPAxis::ScaleType dataScaleType() const { return mDataScaleType; }
    bool tightBoundary() const { return mTightBoundary; }
    QCPColorGradient gradient() const { return mGradient; }
    QCPColorScale *colorScale() const { return mColorScale.data(); }

    // setters:
    Q_SLOT void setDataRange(const QCPRange &dataRange);
    Q_SLOT void setDataScaleType(QCPAxis::ScaleType scaleType);
    Q_SLOT void setGradient(const QCPColorGradient &gradient);
    void setTightBoundary(bool enabled);
    void setColorScale(QCPColorScale *colorScale);

    // non-property methods:
    void rescaleDataRange(bool recalculateDataBounds=false);
    Q_SLOT void updateLegendIcon(Qt::TransformationMode transformMode=Qt::SmoothTransformation, const QSize &thumbSize=QSize(32, 18));

    // reimplemented virtual methods:
    virtual double selectTest(const QPointF &pos, bool onlySelectable, QVariant *details=nullptr) const Q_DECL_OVERRIDE;
    virtual QCPRange getKeyRange(bool &foundRange, QCP::SignDomain inSignDomain=QCP::sdBoth) const Q_DECL_OVERRIDE;
    virtual QCPRange getValueRange(bool &foundRange, QCP::SignDomain inSignDomain=QCP::sdBoth, const QCPRange &inKeyRange=QCPRange()) const Q_DECL_OVERRIDE;

  signals:
    void dataRangeChanged(const QCPRange &newRange);
    void dataScaleTypeChanged(QCPAxis::ScaleType scaleType);
    void gradientChanged(const QCPColorGradient &newGradient);

  protected:
    // property members:
    QCPRange mDataRange;
    QCPAxis::ScaleType mDataScaleType;
    QCPColorGradient mGradient;
    bool mTightBoundary;
    QPointer<QCPColorScale> mColorScale;

    // non-property members:
    QImage mMapImage, mUndersampledMapImage;
    QPixmap mLegendIcon;
    bool mMapImageInvalidated;

    // introduced virtual methods:
    virtual void updateMapImage();

    // reimplemented virtual methods:
    virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;
    virtual void drawLegendIcon(QCPPainter *painter, const QRectF &rect) const Q_DECL_OVERRIDE;

    friend class QCustomPlot;
    friend class QCPLegend;
};

#endif // QCPSPECTROGRAM_H
