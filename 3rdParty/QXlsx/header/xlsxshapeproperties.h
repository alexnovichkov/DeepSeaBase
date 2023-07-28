#ifndef XLSXSHAPEPROPERTIES_H
#define XLSXSHAPEPROPERTIES_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSharedData>

#include "xlsxglobal.h"
#include "xlsxmain.h"
#include "xlsxfillproperties.h"
#include "xlsxlineformat.h"

QT_BEGIN_NAMESPACE_XLSX

class ShapePrivate;

class QXLSX_EXPORT Shape
{
public:
    enum class BlackWhiteMode {
        Clear, // "clr"
        Auto, // "auto"
        Gray, // "gray"
        LightGray, // "ltGray"
        InverseGray, // "invGray"
        GrayWhite, // "grayWhite"
        BlackGray, // "blackGray"
        BlackWhite, // "blackWhite"
        Black, // "black"
        White, // "white"
        Hidden, // "hidden
    };

    std::optional<Shape::BlackWhiteMode> blackWhiteMode() const;
    void setBlackWhiteMode(Shape::BlackWhiteMode val);

    std::optional<Transform2D> transform2D() const;
    void setTransform2D(Transform2D val);

    std::optional<PresetGeometry2D> presetGeometry() const;
    void setPresetGeometry(PresetGeometry2D val);

    FillProperties fill() const;
    void setFill(const FillProperties &val);

    LineFormat line() const;
    void setLine(const LineFormat &line);

    bool isValid() const;

    void write(QXmlStreamWriter &writer, const QString &name) const;
    void read(QXmlStreamReader &reader);

    bool operator == (const Shape &other) const;
    bool operator != (const Shape &other) const;

private:
    QString toString(BlackWhiteMode bwMode) const;
    friend   QDebug operator<<(QDebug, const Shape &f);

    QSharedDataPointer<ShapePrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Shape &f);
#endif

class ShapePrivate : public QSharedData
{
public:
    std::optional<Shape::BlackWhiteMode> blackWhiteMode; //attribute, optional
    std::optional<Transform2D> xfrm; //element, optional
    std::optional<PresetGeometry2D> presetGeometry;
    //TODO: CustomGeometry2D

    FillProperties fill; // element group, optional
    LineFormat line; //element, optional
    //EG_EffectProperties; // ?
    //Scene3D scene3D; // element, optional
    //Shape3D shape3D; // element, optional
    // TODO: extLst

    ShapePrivate();
    ShapePrivate(const ShapePrivate &other);
    ~ShapePrivate();

    bool operator == (const ShapePrivate &other) const;
    bool operator != (const ShapePrivate &other) const;
};

QT_END_NAMESPACE_XLSX

#endif // XLSXSHAPEPROPERTIES_H
