// xlsxformat.h

#ifndef QXLSX_LINEFORMAT_H
#define QXLSX_LINEFORMAT_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QSharedData>

#include "xlsxglobal.h"
#include "xlsxcolor.h"
#include "xlsxfillproperties.h"
#include "xlsxmain.h"
#include "xlsxutility_p.h"

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
    enum class CompoundLineType {
        Single,
        Double,
        ThickThin,
        ThinThick,
        Triple,
    };

    enum class StrokeType {
        Solid,
        Dot,
        RoundDot,
        Dash,
        DashDot,
        LongDash,
        LongDashDot,
        LongDashDotDot,
    };

    enum class LineJoin {
        Round,
        Bevel,
        Miter
    };

    enum class LineCap {
        Square,
        Round,
        Flat,
    };

    enum class LineEndType {
        None,
        Triangle,
        Stealth,
        Oval,
        Arrow,
        Diamond
    };

    enum class LineEndSize {
        Small,
        Medium,
        Large
    };

    enum class PenAlignment
    {
        Center,
        Inset
    };

    LineFormat(); //no fill
    LineFormat(FillProperties::FillType fill, double widthInPt, QColor color); // quick create
    LineFormat(FillProperties::FillType fill, qint64 widthInEMU, QColor color); // quick create
    LineFormat(const LineFormat &other);
    LineFormat &operator=(const LineFormat &rhs);
    ~LineFormat();

    FillProperties::FillType type() const;
    void setType(FillProperties::FillType type);

    Color color() const;
    void setColor(const Color &color);
    void setColor(const QColor &color); //assume color type is sRGB

    FillProperties fill() const;
    void setFill(const FillProperties &fill);

    /**
     * @brief width returns width in range [0..20116800 EMU] or [0..1584 pt]
     * @return optional value
     */
    std::optional<Coordinate> width() const;
    /**
     * @brief setWidth sets line width in points
     * @param widthInPt width in range [0..1584 pt]
     */
    void setWidth(double widthInPt);

    /**
     * @brief setWidth sets line width in EMU
     * @param widthInEMU width in range [0..20116800 EMU]
     */
    void setWidth(qint64 widthInEMU);

    std::optional<PenAlignment> penAlignment() const;
    void setPenAlignment(PenAlignment val);

    std::optional<CompoundLineType> compoundLineType() const;
    void setCompoundLineType(CompoundLineType val);

    std::optional<StrokeType> strokeType() const;
    void setStrokeType(StrokeType val);

    LineCap lineCap() const;
    void setLineCap(LineCap val);

    std::optional<LineJoin> lineJoin() const;
    void setLineJoin(LineJoin val);

    std::optional<LineEndType> lineEndType();
    std::optional<LineEndType> lineStartType();
    void setLineEndType(LineEndType val);
    void setLineStartType(LineEndType val);

    std::optional<LineEndSize> lineEndLength();
    std::optional<LineEndSize> lineStartLength();
    void setLineEndLength(LineEndSize val);
    void setLineStartLength(LineEndSize val);

    std::optional<LineEndSize> lineEndWidth();
    std::optional<LineEndSize> lineStartWidth();
    void setLineEndWidth(LineEndSize val);
    void setLineStartWidth(LineEndSize val);

    bool isValid() const;

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

//    bool operator == (const LineFormat &format) const;
//    bool operator != (const LineFormat &format) const;

private:
    SERIALIZE_ENUM(CompoundLineType, {
        {CompoundLineType::Single, "sng"},
        {CompoundLineType::Double, "dbl"},
        {CompoundLineType::ThickThin, "thickThin"},
        {CompoundLineType::ThinThick, "thinThick"},
        {CompoundLineType::Triple, "tri"}
    });

    friend QDebug operator<<(QDebug, const LineFormat &f);

    QSharedDataPointer<LineFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const LineFormat &f);
#endif

QT_END_NAMESPACE_XLSX

#endif
