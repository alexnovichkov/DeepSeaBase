#ifndef XLSXEFFECT_H
#define XLSXEFFECT_H

#include "xlsxglobal.h"

#include <QSharedData>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "xlsxutility_p.h"
#include "xlsxmain.h"
#include "xlsxfillproperties.h"

QT_BEGIN_NAMESPACE_XLSX

class EffectPrivate;

/**
 * @brief The EffectPrivate class implements the EG_EffectProperties
 */
class Effect
{
public:
    enum class FillBlendMode
    {
        Overlay,//"over"/>
        Multiply, //"mult"/>
        Screen,//"screen"/>
        Darken,//"darken"/>
        Lighten//"lighten"/>
    };
    enum class Alignment
    {
        AlignTopLeft,
        AlignTop,
        AlignTopRight,
        AlignRight,
        AlignCenter,
        AlignLeft,
        AlignBottomLeft,
        AlignBottom,
        AlignBottomRight,
    };

    Effect();
    Effect(const Effect &other);
    ~Effect();
    Effect &operator=(const Effect &other);

    bool operator == (const Effect &other) const;
    bool operator != (const Effect &other) const;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer) const;
private:
    SERIALIZE_ENUM(FillBlendMode,
    {
        {FillBlendMode::Overlay, "over"},
        {FillBlendMode::Multiply, "mult"},
        {FillBlendMode::Screen, "screen"},
        {FillBlendMode::Darken, "darken"},
        {FillBlendMode::Lighten, "lighten"}
    });
    SERIALIZE_ENUM(Alignment,
    {
        {Alignment::AlignTopLeft, "tl"},
        {Alignment::AlignTop, "t"},
        {Alignment::AlignTopRight, "tr"},
        {Alignment::AlignRight, "r"},
        {Alignment::AlignCenter, "ctr"},
        {Alignment::AlignLeft, "l"},
        {Alignment::AlignBottomLeft, "bl"},
        {Alignment::AlignBottom, "b"},
        {Alignment::AlignBottomRight, "br"},
    });

    void readEffectList(QXmlStreamReader &reader);
    void writeEffectList(QXmlStreamWriter &writer) const;
    QSharedDataPointer<EffectPrivate> d;
};

class EffectPrivate : public QSharedData
{
public:
    EffectPrivate();
    EffectPrivate(const EffectPrivate &other);
    ~EffectPrivate();

    bool operator == (const EffectPrivate &other) const;
    bool operator != (const EffectPrivate &other) const;

    Coordinate blurRadius;
    std::optional<bool> blurGrow;

    FillProperties fillOverlay;
    Effect::FillBlendMode fillBlendMode;

    Color glowColor;
    Coordinate glowRadius;

    Color innerShadowColor;
    Coordinate innerShadowBlurRadius;
    Coordinate innerShadowOffset;
    std::optional<Angle> innerShadowDirection;


    Color outerShadowColor;
    Coordinate outerShadowBlurRadius;
    Coordinate outerShadowOffset;
    std::optional<Angle> outerShadowDirection;
    std::optional<double> outerShadowHorizontalScalingFactor; //in %
    std::optional<double> outerShadowVerticalScalingFactor; //in %
    std::optional<Angle> outerShadowHorizontalSkewFactor;
    std::optional<Angle> outerShadowVerticalSkewFactor;
    std::optional<bool> outerShadowRotateWithShape;
    std::optional<Effect::Alignment> outerShadowAlignment;

    Color presetShadowColor;
    Coordinate presetShadowOffset;
    std::optional<Angle> presetShadowDirection;
    std::optional<int> presetShadow;
};


QT_END_NAMESPACE_XLSX

#endif // XLSXEFFECT_H
