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

    FillProperties() {}
    FillProperties(FillType type);
    FillProperties(const FillProperties &other);
    FillProperties &operator=(const FillProperties &rhs);

    FillType type() const;
    void setType(FillType type);

    std::optional<Color> color() const;
    void setColor(const Color &color);
    void setColor(const QColor &color);

    QMap<double, Color> gradientList() const;
    void setGradientList(const QMap<double, Color> &list);

    std::optional<QPair<Angle, bool>> linearShade() const;
    void setLinearShade(QPair<Angle, bool> val);

    //TODO: path shade get / set
    //TODO: tile get / set

    bool isValid() const;
    //QByteArray formatKey() const;

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

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
    friend QDebug operator<<(QDebug, const FillProperties &f);
    QSharedDataPointer<FillPropertiesPrivate> d;
    void readNoFill(QXmlStreamReader &reader);
    void readSolidFill(QXmlStreamReader &reader);
    void readGradientFill(QXmlStreamReader &reader);

    void writeNoFill(QXmlStreamWriter &writer) const;
    void writeSolidFill(QXmlStreamWriter &writer) const;
    void writeGradientFill(QXmlStreamWriter &writer) const;

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
    std::optional<Color> color;

    //Gradient fill properties
    QMap<double, Color> gradientList;
    std::optional<Angle> linearShadeAngle; //0..360
    std::optional<bool> linearShadeScaled;
    std::optional<FillProperties::PathShadeType> pathShadeType;
    std::optional<QRectF> pathShadeRect;
    std::optional<QRectF> tileRect;
    std::optional<FillProperties::TileFlipMode> tileFlipMode;
    std::optional<bool> rotWithShape;


};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const FillProperties &f);
#endif

QT_END_NAMESPACE_XLSX

#endif // XLSXFILLPROPERTIES_H
