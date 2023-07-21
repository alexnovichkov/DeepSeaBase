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

class ShapePropertiesPrivate;

class QXLSX_EXPORT ShapeProperties
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

    std::optional<ShapeProperties::BlackWhiteMode> blackWhiteMode() const;
    void setBlackWhiteMode(ShapeProperties::BlackWhiteMode val);

    std::optional<Transform2D> xfrm() const;
    void setXfrm(Transform2D val);

    std::optional<PresetGeometry2D> presetGeometry() const;
    void setPresetGeometry(PresetGeometry2D val);

    FillProperties fill() const;
    void setFill(const FillProperties &val);

    LineFormat line() const;
    void setLine(const LineFormat &line);

    bool isValid() const;
    QByteArray formatKey() const;

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

    bool operator == (const ShapeProperties &format) const;
    bool operator != (const ShapeProperties &format) const;

private:
    QString toString(BlackWhiteMode bwMode) const;
    friend   QDebug operator<<(QDebug, const ShapeProperties &f);

    QSharedDataPointer<ShapePropertiesPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const ShapeProperties &f);
#endif

class ShapePropertiesPrivate : public QSharedData
{
public:
    std::optional<ShapeProperties::BlackWhiteMode> blackWhiteMode; //attribute, optional
    std::optional<Transform2D> xfrm; //element, optional
    std::optional<PresetGeometry2D> presetGeometry;
    //TODO: CustomGeometry2D

    FillProperties fill; // element group, optional
    LineFormat line; //element, optional
    //EG_EffectProperties; // ?
    //Scene3D scene3D; // element, optional
    //Shape3D shape3D; // element, optional
    // TODO: extLst

    ShapePropertiesPrivate() {}
    ShapePropertiesPrivate(const ShapePropertiesPrivate &other);
    ~ShapePropertiesPrivate() {}
};

QT_END_NAMESPACE_XLSX

#endif // XLSXSHAPEPROPERTIES_H
