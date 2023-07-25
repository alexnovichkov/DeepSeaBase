#ifndef XLSXAXIS_H
#define XLSXAXIS_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSharedData>

#include "xlsxglobal.h"
#include "xlsxshapeproperties.h"
#include "xlsxutility_p.h"
#include "xlsxtitle.h"

QT_BEGIN_NAMESPACE_XLSX

class Styles;
class Worksheet;
class WorksheetPrivate;
class RichStringPrivate;
class SharedStrings;

class AxisPrivate;

class QXLSX_EXPORT Axis
{
public:
    struct Scaling {
//        <xsd:complexType name="CT_Scaling">
//          <xsd:sequence>
//            <xsd:element name="logBase" type="CT_LogBase" minOccurs="0" maxOccurs="1"/>
//            <xsd:element name="orientation" type="CT_Orientation" minOccurs="0" maxOccurs="1"/>
//            <xsd:element name="max" type="CT_Double" minOccurs="0" maxOccurs="1"/>
//            <xsd:element name="min" type="CT_Double" minOccurs="0" maxOccurs="1"/>
//            <xsd:element name="extLst" type="CT_ExtensionList" minOccurs="0" maxOccurs="1"/>
//          </xsd:sequence>
//        </xsd:complexType>
        std::optional<double> logBase;
        std::optional<bool> reversed;
        std::optional<double> min;
        std::optional<double> max;
        void write(QXmlStreamWriter &writer) const;
        void read(QXmlStreamReader &reader);
    };
    enum class Type { None = (-1), Cat, Val, Date, Ser };
    enum class Position { None = (-1), Left, Right, Top, Bottom };
    enum class CrossesType {
        Minimum,
        Maximum,
        Position,
        AutoZero
    };

    Axis(Type type, Position position);
    Axis(Type type);

    bool isValid() const;

    Type type() const;
    Position position() const;
    void setPosition(Position position);

    bool visible() const;
    void setVisible(bool visible);

    int id() const;
    void setId(int id);

    int crossAxis() const;
    void setCrossAxis(Axis *axis);
    void setCrossAxis(int axisId);

    double crossesAt() const;
    void setCrossesAt(double val);
    CrossesType crossesType() const;
    void setCrossesAt(CrossesType val);

    void setMajorGridLines(const ShapeProperties &val);
    void setMinorGridLines(const ShapeProperties &val);
    void setMajorGridLines(const QColor &color, double width, LineFormat::StrokeType strokeType);
    void setMinorGridLines(const QColor &color, double width, LineFormat::StrokeType strokeType);


    QString titleAsString() const;
    Title title() const;
    void setTitle(const QString &title);

    Scaling *scaling();
    void setScaling(Scaling scaling);

    QPair<double, double> range() const;
    void setRange(double min, double max);

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

    bool operator == (const Axis &axis) const;
    bool operator != (const Axis &axis) const;

private:
    SERIALIZE_ENUM(Type, {
                       {Type::None, "none"},
                       {Type::Cat, "cat"},
                       {Type::None, "none"},
                       {Type::None, "none"}

                   });

    SERIALIZE_ENUM(Position, {
                       {Position::None, "none"},
                       {Position::Top, "t"},
                       {Position::Bottom, "b"},
                       {Position::Left, "l"},
                       {Position::Right, "r"}
                   });
    friend QDebug operator<<(QDebug, const Axis &axis);

    QSharedDataPointer<AxisPrivate> d;
};

class AxisPrivate : public QSharedData
{
public:
    int id;
    Axis::Scaling scaling;
    std::optional<bool> visible;
    Axis::Position position;
    Axis::Type type;

    int crossAxis = -1;
    std::optional<Axis::CrossesType> crossesType;
    std::optional<double> crossesPosition;

    ShapeProperties majorGridlines;
    ShapeProperties minorGridlines;

   // QString title; // temporary solution
    Title title;
//    NumberFormat numberFormat;

//    TickMark majorTickMark;
//    TickMark minorTickMark;

    AxisPrivate();
    AxisPrivate(const AxisPrivate &other);
    ~AxisPrivate() {}
};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const Axis &axis);
#endif

QT_END_NAMESPACE_XLSX

#endif // XLSXAXIS_H
