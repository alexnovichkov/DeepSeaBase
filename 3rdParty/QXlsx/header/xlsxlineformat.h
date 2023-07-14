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
        CLT_Simple,
        CLT_Double,
        CLT_ThickThin,
        CLT_ThinThick,
        CLT_Triple,
    };

    enum PointType {
        PT_Rectangular,
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

    QColor color() const;
    void setColor(const QColor &color);

    double width() const;
    void setWidth(double width);

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
