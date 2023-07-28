#ifndef XLSXFILLPROPERTIES_H
#define XLSXFILLPROPERTIES_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSharedData>

#include "xlsxglobal.h"
#include "xlsxcolor.h"
#include "xlsxmain.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

class FillPropertiesPrivate;

/**
 * @brief The FillProperties class
 * Sets fill properties for lines, shapes etc.
 */
class QXLSX_EXPORT FillProperties
{
public:
    enum class FillType {
        NoFill,
        SolidFill,
        GradientFill,
        BlipFill,
        PatternFill,
        GroupFill
    };
    enum class PathShadeType
    {
        Shape,
        Circle,
        Rectangle
    };
    enum class TileFlipMode
    {
        None,
        X,
        Y,
        XY
    };
    enum class PatternType
    {
        Percent5,
        Percent10,
        Percent20,
        Percent25,
        Percent30,
        Percent40,
        Percent50,
        Percent60,
        Percent70,
        Percent75,
        Percent80,
        Percent90,
        Horizontal,
        Vertical,
        LightHorizontal,
        LightVertical,
        DarkHorizontal,
        DarkVertical,
        NarrowHorizontal,
        NarrowVertical,
        DashedHorizontal,
        DashedVertical,
        Cross,
        DownwardDiagonal,
        UpwardDiagonal,
        LightDownwardDiagonal,
        LightUpwardDiagonal,
        DarkDownwardDiagonal,
        DarkUpwardDiagonal,
        WideDownwardDiagonal,
        WideUpwardDiagonal,
        DashedDownwardDiagonal,
        DashedUpwardDiagonal,
        DiagonalCross,
        SmallCheckerBoard,
        LargeCheckerBoard,
        SmallGrid,
        LargeGrid,
        DottedGrid,
        SmallConfetti,
        LargeConfetti,
        HorizontalBrick,
        DiagonalBrick,
        SolidDiamond,
        Opendiamond,
        DottedDiamond,
        Plaid,
        Sphere,
        Weave,
        Divot,
        Shingle,
        Wave,
        Trellis,
        ZigZag,
    };

    FillProperties();
    explicit FillProperties(FillType type);
    FillProperties(const FillProperties &other);
    FillProperties &operator=(const FillProperties &rhs);

    FillType type() const;
    void setType(FillType type);

    /* Solid fill properties */
    Color color() const;
    void setColor(const Color &color);
    void setColor(const QColor &color);

    /* Gradient fill properties */

    QMap<double, Color> gradientList() const;
    void setGradientList(const QMap<double, Color> &list);

    std::optional<Angle> linearShadeAngle() const;
    void setLinearShadeAngle(Angle val);

    std::optional<bool> linearShadeScaled() const;
    void setLinearShadeScaled(bool scaled);

    std::optional<PathShadeType> pathShadeType() const;
    void setPathShadeType(PathShadeType pathShadeType);

    std::optional<QRectF> pathShadeRect() const;
    void setPathShadeRect(QRectF rect);

    std::optional<QRectF> tileRect() const;
    void setTileRect(QRectF rect);

    std::optional<FillProperties::TileFlipMode> tileFlipMode() const;
    void setTileFlipMode(TileFlipMode tileFlipMode);

    std::optional<bool> rotateWithShape() const;
    void setRotateWithShape(bool val);

    /* Pattern fill properties */

    Color foregroundColor() const;
    void setForegroundColor(const Color &color);

    Color backgroundColor() const;
    void setBackgroundColor(const Color &color);

    std::optional<PatternType> patternType();
    void setPatternType(PatternType patternType);

    bool isValid() const;

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

    bool operator==(const FillProperties &other) const;
    bool operator!=(const FillProperties &other) const;

private:
    SERIALIZE_ENUM(FillType, {
        {FillType::NoFill, "noFill"},
        {FillType::SolidFill, "solidFill"},
        {FillType::GradientFill, "gradFill"},
        {FillType::BlipFill, "blipFill"},
        {FillType::PatternFill, "pattFill"},
        {FillType::GroupFill, "grpFill"},
    });
    SERIALIZE_ENUM(PathShadeType, {
        {PathShadeType::Shape, "shape"},
        {PathShadeType::Circle, "circle"},
        {PathShadeType::Rectangle, "rect"},
    });
    SERIALIZE_ENUM(TileFlipMode, {
        {TileFlipMode::None, "none"},
        {TileFlipMode::X, "x"},
        {TileFlipMode::Y, "y"},
        {TileFlipMode::XY, "xy"},
    });
    SERIALIZE_ENUM(PatternType, {
        {PatternType::Percent5, "pct5"},
        {PatternType::Percent10, "pct10"},
        {PatternType::Percent20, "pct20"},
        {PatternType::Percent25, "pct25"},
        {PatternType::Percent30, "pct30"},
        {PatternType::Percent40, "pct40"},
        {PatternType::Percent50, "pct50"},
        {PatternType::Percent60, "pct60"},
        {PatternType::Percent70, "pct70"},
        {PatternType::Percent75, "pct75"},
        {PatternType::Percent80, "pct80"},
        {PatternType::Percent90, "pct90"},
        {PatternType::Horizontal, "horz"},
        {PatternType::Vertical, "vert"},
        {PatternType::LightHorizontal, "ltHorz"},
        {PatternType::LightVertical, "ltVert"},
        {PatternType::DarkHorizontal, "dkHorz"},
        {PatternType::DarkVertical, "dkVert"},
        {PatternType::NarrowHorizontal, "narHorz"},
        {PatternType::NarrowVertical, "narVert"},
        {PatternType::DashedHorizontal, "dashHorz"},
        {PatternType::DashedVertical, "dashVert"},
        {PatternType::Cross, "cross"},
        {PatternType::DownwardDiagonal, "dnDiag"},
        {PatternType::UpwardDiagonal, "upDiag"},
        {PatternType::LightDownwardDiagonal, "ltDnDiag"},
        {PatternType::LightUpwardDiagonal, "ltUpDiag"},
        {PatternType::DarkDownwardDiagonal, "dkDnDiag"},
        {PatternType::DarkUpwardDiagonal, "dkUpDiag"},
        {PatternType::WideDownwardDiagonal, "wdDnDiag"},
        {PatternType::WideUpwardDiagonal, "wdUpDiag"},
        {PatternType::DashedDownwardDiagonal, "dashDnDiag"},
        {PatternType::DashedUpwardDiagonal, "dashUpDiag"},
        {PatternType::DiagonalCross, "diagCross"},
        {PatternType::SmallCheckerBoard, "smCheck"},
        {PatternType::LargeCheckerBoard, "lgCheck"},
        {PatternType::SmallGrid, "smGrid"},
        {PatternType::LargeGrid, "lgGrid"},
        {PatternType::DottedGrid, "dotGrid"},
        {PatternType::SmallConfetti, "smConfetti"},
        {PatternType::LargeConfetti, "lgConfetti"},
        {PatternType::HorizontalBrick, "horzBrick"},
        {PatternType::DiagonalBrick, "diagBrick"},
        {PatternType::SolidDiamond, "solidDmnd"},
        {PatternType::Opendiamond, "openDmnd"},
        {PatternType::DottedDiamond, "dotDmnd"},
        {PatternType::Plaid, "plaid"},
        {PatternType::Sphere, "sphere"},
        {PatternType::Weave, "weave"},
        {PatternType::Divot, "divot"},
        {PatternType::Shingle, "shingle"},
        {PatternType::Wave, "wave"},
        {PatternType::Trellis, "trellis"},
        {PatternType::ZigZag, "zigZag"},
    });
    friend QDebug operator<<(QDebug, const FillProperties &f);
    QSharedDataPointer<FillPropertiesPrivate> d;
    void readNoFill(QXmlStreamReader &reader);
    void readSolidFill(QXmlStreamReader &reader);
    void readGradientFill(QXmlStreamReader &reader);
    void readPatternFill(QXmlStreamReader &reader);
    void readGroupFill(QXmlStreamReader &reader);

    void writeNoFill(QXmlStreamWriter &writer) const;
    void writeSolidFill(QXmlStreamWriter &writer) const;
    void writeGradientFill(QXmlStreamWriter &writer) const;
    void writePatternFill(QXmlStreamWriter &writer) const;
    void writeGroupFill(QXmlStreamWriter &writer) const;

    void readGradientList(QXmlStreamReader &reader);
    void writeGradientList(QXmlStreamWriter &writer) const;
};

class FillPropertiesPrivate: public QSharedData
{
public:
    FillPropertiesPrivate();
    FillPropertiesPrivate(const FillPropertiesPrivate &other);
    ~FillPropertiesPrivate();

    FillProperties::FillType type;
    Color color;

    //Gradient fill properties
    QMap<double, Color> gradientList;
    std::optional<Angle> linearShadeAngle; //0..360
    std::optional<bool> linearShadeScaled;
    std::optional<FillProperties::PathShadeType> pathShadeType;
    std::optional<QRectF> pathShadeRect;
    std::optional<QRectF> tileRect;
    std::optional<FillProperties::TileFlipMode> tileFlipMode;
    std::optional<bool> rotWithShape;

    //Pattern fill properties
    Color foregroundColor;
    Color backgroundColor;
    std::optional<FillProperties::PatternType> patternType;

    bool operator==(const FillPropertiesPrivate &other) const;
};

QDebug operator<<(QDebug dbg, const FillProperties &f);

QT_END_NAMESPACE_XLSX

Q_DECLARE_TYPEINFO(QXlsx::FillProperties, Q_MOVABLE_TYPE);

#endif // XLSXFILLPROPERTIES_H
