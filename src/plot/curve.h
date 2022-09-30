#ifndef CURVE_H
#define CURVE_H

#include <QPen>

class FileDescriptor;
class PointLabel;
class Channel;
class DataHolder;
class Plot;
class PointMarker;
class Implementation;

#include <qglobal.h>
#include "selectable.h"
#include "enums.h"

class Curve : public Selectable
{
public:
    enum class Type {
        Line,
        Spectrogram,
        Unknown
    };
    enum class MarkerShape {
        NoMarker,
        Dot,
        Cross,
        Plus,
        Circle,
        Disc,
        Square,
        Diamond,
        Star,
        Triangle,
        TriangleInverted,
        CrossSquare,
        PlusSquare,
        CrossCircle,
        PlusCircle,
        Peace
    };
    static QString markerShapeDescription(MarkerShape shape);


    Curve(const QString &title, Channel *channel);
    virtual ~Curve();

    virtual void attachTo(Plot *plot);
    virtual void detachFrom(Plot *plot);

    virtual QString title() const = 0;
    virtual void setTitle(const QString &title) = 0;

    virtual Enums::AxisType yAxis() const = 0;
    virtual void setYAxis(Enums::AxisType axis) = 0;

    virtual Enums::AxisType xAxis() const = 0;
    virtual void setXAxis(Enums::AxisType axis) = 0;

    virtual QPen pen() const = 0;
    void setPen(const QPen &pen) {oldPen = pen; updatePen();}

    MarkerShape markerShape() const {return m_markerShape;}
    void setMarkerShape(MarkerShape markerShape);
    virtual void updateScatter() {}

    int markerSize() const {return m_markerSize;}
    void setMarkerSize(int markerSize);

    virtual SamplePoint samplePoint(SelectedPoint point) const = 0;

    void setVisible(bool visible);

    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    void removeLabels();
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos);
    /** find label by point on a curve */
    PointLabel *findLabel(SelectedPoint point);
    virtual SelectedPoint closest(const QPoint &pos, double *dist = nullptr, double *dist2 = nullptr) const = 0;

    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;

    virtual LegendData commonLegendData() const;

    virtual double yMin() const;
    virtual double yMax() const;
    virtual double xMin() const;
    virtual double xMax() const;
    int samplesCount() const;

    void updateLabels();

    Channel *channel;
    QList<PointLabel*> labels;
    MarkerShape m_markerShape = MarkerShape::NoMarker;
    int m_markerSize = 6;


    Implementation *impl = nullptr;

    int fileNumber=0;
    bool duplicate;
    bool fixed = false;
    Type type = Type::Unknown;

public:

    void evaluateScale(int &from, int &to, double startX, double endX) const;
    void switchFixed();
    virtual void resetCashedData() {}

    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY,
                            SelectedPoint *point) const override;
    virtual void moveLeft(int count = 1) override;
    virtual void moveRight(int count = 1) override;
    virtual void moveUp(int count = 1) override;
    virtual void moveDown(int count = 1) override;
    virtual void fix() override;
    virtual void remove() override;
    virtual bool draggable() const override;
protected:
    virtual void updateSelection(SelectedPoint point) override;
    inline virtual bool updateAnyway() const override {return true;}
    virtual void updatePen() = 0;
    Plot *m_plot = nullptr;
    mutable SelectedPoint selectedPoint;
    PointMarker *pointMarker = nullptr;
    QPen oldPen;
};





#endif // CURVE_H
