#ifndef GRAPH2D_H
#define GRAPH2D_H

#include "qcustomplot.h"
#include "plot/curve.h"

class Data2D;

class Graph2D : public QCPAbstractPlottable, public QCPPlottableInterface1D, public Curve
{
    Q_OBJECT

    Q_PROPERTY(LineStyle lineStyle READ lineStyle WRITE setLineStyle)
    Q_PROPERTY(QCPScatterStyle scatterStyle READ scatterStyle WRITE setScatterStyle)
    Q_PROPERTY(int scatterSkip READ scatterSkip WRITE setScatterSkip)
    Q_PROPERTY(Graph2D* channelFillGraph READ channelFillGraph WRITE setChannelFillGraph)
    Q_PROPERTY(bool adaptiveSampling READ adaptiveSampling WRITE setAdaptiveSampling)

public:
    enum LineStyle { lsNone        ///< data points are not connected with any lines (e.g. data only represented
                                   ///< with symbols according to the scatter style, see \ref setScatterStyle)
                     ,lsLine       ///< data points are connected by a straight line
                     ,lsStep       ///< line is drawn as steps for octave plots
                     ,lsImpulse    ///< each data point is represented by a line parallel to the value axis, which reaches from the data point to the zero-value-line
                   };
    Q_ENUMS(LineStyle)

  explicit Graph2D(Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis);
    virtual ~Graph2D() Q_DECL_OVERRIDE;

  void setData(Data2D *m_data);
  Data2D *data() const {return m_data;}

  // getters
  inline LineStyle lineStyle() const { return mLineStyle; }
  inline QCPScatterStyle scatterStyle() const { return mScatterStyle; }
  inline int scatterSkip() const { return mScatterSkip; }
  inline Graph2D *channelFillGraph() const { return mChannelFillGraph.data(); }
  inline bool adaptiveSampling() const { return mAdaptiveSampling; }

  // setters:
  void setLineStyle(LineStyle ls);
  void setScatterStyle(const QCPScatterStyle &style);
  void setScatterSkip(int skip);
  void setChannelFillGraph(Graph2D *targetGraph);
  void setAdaptiveSampling(bool enabled);

  // virtual methods of 1d plottable interface:
  virtual int dataCount() const Q_DECL_OVERRIDE;
  virtual double dataMainKey(int index) const Q_DECL_OVERRIDE;
  virtual double dataSortKey(int index) const Q_DECL_OVERRIDE;
  virtual double dataMainValue(int index) const Q_DECL_OVERRIDE;
  virtual QCPRange dataValueRange(int index) const Q_DECL_OVERRIDE;
  virtual QPointF dataPixelPosition(int index) const Q_DECL_OVERRIDE;
  virtual bool sortKeyIsMainKey() const Q_DECL_OVERRIDE;
  virtual QCPDataSelection selectTestRect(const QRectF &rect, bool onlySelectable) const Q_DECL_OVERRIDE;
  virtual int findBegin(double sortKey, bool expandedRange=true) const Q_DECL_OVERRIDE;
  virtual int findEnd(double sortKey, bool expandedRange=true) const Q_DECL_OVERRIDE;

  // reimplemented QCPAbstractPlottable virtual methods:
  virtual double selectTest(const QPointF &pos, bool onlySelectable, QVariant *details=nullptr) const Q_DECL_OVERRIDE;
  virtual QCPPlottableInterface1D *interface1D() Q_DECL_OVERRIDE { return this; }
  virtual QCPRange getKeyRange(bool &foundRange, QCP::SignDomain inSignDomain=QCP::sdBoth) const Q_DECL_OVERRIDE;
  virtual QCPRange getValueRange(bool &foundRange, QCP::SignDomain inSignDomain=QCP::sdBoth, const QCPRange &inKeyRange=QCPRange()) const Q_DECL_OVERRIDE;

protected:
  // property members:
  Data2D *m_data = nullptr;
  LineStyle mLineStyle = lsLine;
  QCPScatterStyle mScatterStyle;
  int mScatterSkip = 0;
  QPointer<Graph2D> mChannelFillGraph;
  bool mAdaptiveSampling = true;

  // helpers for subclasses:
  void getDataSegments(QList<QCPDataRange> &selectedSegments, QList<QCPDataRange> &unselectedSegments) const;
  void drawPolyline(QCPPainter *painter, const QVector<QPointF> &lineData) const;

  // reimplemented virtual methods:
  virtual void draw(QCPPainter *painter) Q_DECL_OVERRIDE;
  virtual void drawLegendIcon(QCPPainter *painter, const QRectF &rect) const Q_DECL_OVERRIDE;

  // introduced virtual methods:
  virtual void drawFill(QCPPainter *painter, QVector<QPointF> *lines) const;
  virtual void drawScatterPlot(QCPPainter *painter, const QVector<QPointF> &scatters, const QCPScatterStyle &style) const;
  virtual void drawLinePlot(QCPPainter *painter, const QVector<QPointF> &lines) const;
  virtual void drawImpulsePlot(QCPPainter *painter, const QVector<QPointF> &lines) const;

  virtual void getOptimizedLineData(QVector<QCPGraphData> *lineData, const int begin, const int end) const;
  virtual void getOptimizedScatterData(QVector<QCPGraphData> *scatterData, int begin, int end) const;

  // non-virtual methods:
  void getVisibleDataBounds(int &begin, int &end, const QCPDataRange &rangeRestriction) const;
  void getLines(QVector<QPointF> *lines, const QCPDataRange &dataRange) const;
  void getScatters(QVector<QPointF> *scatters, const QCPDataRange &dataRange) const;

  QVector<QPointF> dataToLines(const QVector<QCPGraphData> &data) const;
  QVector<QPointF> dataToStepLines(const QVector<QCPGraphData> &data) const;
  QVector<QPointF> dataToStepRightLines(const QVector<QCPGraphData> &data) const;
  QVector<QCPDataRange> getNonNanSegments(const QVector<QPointF> *lineData, Qt::Orientation keyOrientation) const;
  QVector<QPair<QCPDataRange, QCPDataRange> > getOverlappingSegments(QVector<QCPDataRange> thisSegments, const QVector<QPointF> *thisData, QVector<QCPDataRange> otherSegments, const QVector<QPointF> *otherData) const;
  bool segmentsIntersect(double aLower, double aUpper, double bLower, double bUpper, int &bPrecedence) const;
  QPointF getFillBasePoint(QPointF matchingDataPoint) const;
  const QPolygonF getFillPolygon(const QVector<QPointF> *lineData, QCPDataRange segment) const;
  const QPolygonF getChannelFillPolygon(const QVector<QPointF> *thisData, QCPDataRange thisSegment, const QVector<QPointF> *otherData, QCPDataRange otherSegment) const;
  int findIndexBelowX(const QVector<QPointF> *data, double x) const;
  int findIndexAboveX(const QVector<QPointF> *data, double x) const;
  int findIndexBelowY(const QVector<QPointF> *data, double y) const;
  int findIndexAboveY(const QVector<QPointF> *data, double y) const;
  double pointDistance(const QPointF &pixelPoint, int &closestData) const;

  friend class QCustomPlot;
  friend class QCPLegend;
private:
  Q_DISABLE_COPY(Graph2D)
  mutable QVector<QCPGraphData> cashedLineData;
  mutable int cashedBegin = 0;
  mutable int cashedEnd = 0;
  mutable double cashedKeyPixelSpan = 0;

  // Curve interface
public:
  virtual bool isVisible() const override;
  virtual void detachFrom(QCPPlot *plot) override;
  virtual QString title() const override;
  virtual void setTitle(const QString &title) override;
  virtual Enums::AxisType yAxis() const override;
  virtual void setYAxis(Enums::AxisType axis) override;
  virtual Enums::AxisType xAxis() const override;
  virtual void setXAxis(Enums::AxisType axis) override;
  virtual QPen pen() const override;
  virtual SamplePoint samplePoint(SelectedPoint point) const override;
  virtual SelectedPoint closest(const QPoint &pos, double *dist1, double *dist2) const override;
  virtual LegendData commonLegendData() const override;
  virtual void updateScatter() override;
  virtual void updatePen() override;
  virtual QIcon thumbnail() const override;
protected:
  friend class QCPCheckableLegendItem;
};

#endif // GRAPH2D_H
