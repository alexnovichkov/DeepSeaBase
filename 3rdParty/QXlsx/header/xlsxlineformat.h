// xlsxformat.h

#ifndef QXLSX_LINEFORMAT_H
#define QXLSX_LINEFORMAT_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QExplicitlySharedDataPointer>
#include <QVariant>

#include "xlsxglobal.h"

QT_BEGIN_NAMESPACE_XLSX

class Styles;
class Worksheet;
class WorksheetPrivate;
class RichStringPrivate;
class SharedStrings;

class LineFormatPrivate;

class QXLSX_EXPORT LineFormat
{
public:
    enum LineType {
        LT_NoLine,
        LT_SolidLine,
        LT_GradientLine
    };

    enum ColorType {
        CT_RGB,
        CT_Theme,
        CT_Standard,
    };

    //прозрачность - double [0..1]

    enum CompoundLineType {
        CLT_Single,
        CLT_Double,
        CLT_ThickThin,
        CLT_ThinThick,
        CLT_Triple,
    };

    enum StrokeType {
        ST_Solid,
        ST_Dot,
        ST_RoundDot,
        ST_Dash,
        ST_DashDot,
        ST_LongDash,
        ST_LongDashDot,
        ST_LongDashDotDot,
    };

    enum PointType {
        PT_Square,
        PT_Round,
        PT_Flat,
    };

    enum ConnectionType {
        CoT_Rounded,
        CoT_Relief,
        CoT_Straight,
    };

    enum ArrowType {
        AT_NoArrow,
        AT_SimpleArrow,
        AT_OpenArrow,
        AT_IncurvedArrow,
        AT_DiamondArrow,
        AT_RoundArrow,
    };

    // размер стрелки - int [1..9]

    // сглаженная линия - bool

    LineFormat();
    LineFormat(const LineFormat &other);
    LineFormat &operator=(const LineFormat &rhs);
    ~LineFormat();

    LineType lineType() const;
    void setLineType(LineType type);

    QColor color() const;
    void setColor(const QColor &color);

    bool smooth() const;
    void setSmooth(bool smooth);

    double width() const;
    void setWidth(double width);

    CompoundLineType compoundLineType() const;
    void setCompoundLineType(CompoundLineType compoundLineType);

    StrokeType strokeType() const;
    void setStrokeType(StrokeType strokeType);

    PointType pointType() const;
    void setPointType(PointType pointType);

    double alpha() const;
    void setAlpha(double alpha);

    /* marker */

    bool isValid() const;
    QByteArray formatKey() const;

    bool operator == (const LineFormat &format) const;
    bool operator != (const LineFormat &format) const;

private:
    friend   QDebug operator<<(QDebug, const LineFormat &f);

    QExplicitlySharedDataPointer<LineFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const LineFormat &f);
#endif

QT_END_NAMESPACE_XLSX

#endif
