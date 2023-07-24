// xlsxcolor_p.h

#ifndef QXLSX_XLSXCOLOR_H
#define QXLSX_XLSXCOLOR_H

#include <QtGlobal>
#include <QVariant>
#include <QColor>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "xlsxglobal.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

class Styles;

//<xsd:group name="EG_ColorChoice">
//    <xsd:choice>
//      <xsd:element name="scrgbClr" type="CT_ScRgbColor" minOccurs="1" maxOccurs="1
//      <xsd:element name="srgbClr" type="CT_SRgbColor" minOccurs="1" maxOccurs="1
//      <xsd:element name="hslClr" type="CT_HslColor" minOccurs="1" maxOccurs="1
//      <xsd:element name="sysClr" type="CT_SystemColor" minOccurs="1" maxOccurs="1
//      <xsd:element name="schemeClr" type="CT_SchemeColor" minOccurs="1" maxOccurs="1
//      <xsd:element name="prstClr" type="CT_PresetColor" minOccurs="1" maxOccurs="1
//    </xsd:choice>
//  </xsd:group>

class ColorTransform
{
public:
    enum class Type
    {
        Tint, //<xsd:element name="tint" type="CT_PositiveFixedPercentage" minOccurs="1" maxOccurs="1"/>
        Shade, //<xsd:element name="shade" type="CT_PositiveFixedPercentage" minOccurs="1" maxOccurs="1"/>
        Complement, //<xsd:element name="comp" type="CT_ComplementTransform" minOccurs="1" maxOccurs="1"/>
        Inverse, //<xsd:element name="inv" type="CT_InverseTransform" minOccurs="1" maxOccurs="1"/>
        Grayscale, //<xsd:element name="gray" type="CT_GrayscaleTransform" minOccurs="1" maxOccurs="1"/>
        Alpha, //<xsd:element name="alpha" type="CT_PositiveFixedPercentage" minOccurs="1" maxOccurs="1"/>
        AlphaOff, //<xsd:element name="alphaOff" type="CT_FixedPercentage" minOccurs="1" maxOccurs="1"/>
        AlphaModulation, //<xsd:element name="alphaMod" type="CT_PositivePercentage" minOccurs="1" maxOccurs="1"/>
        Hue, //<xsd:element name="hue" type="CT_PositiveFixedAngle" minOccurs="1" maxOccurs="1"/>
        HueOff, //<xsd:element name="hueOff" type="CT_Angle" minOccurs="1" maxOccurs="1"/>
        HueModulation, //<xsd:element name="hueMod" type="CT_PositivePercentage" minOccurs="1" maxOccurs="1"/>
        Saturation, //<xsd:element name="sat" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        SaturationOff, //<xsd:element name="satOff" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        SaturationModulation, //<xsd:element name="satMod" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        Luminescence, //<xsd:element name="lum" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        LuminescenceOff, //<xsd:element name="lumOff" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        LuminescenceModulation, //<xsd:element name="lumMod" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        Red, //<xsd:element name="red" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        RedOff, //<xsd:element name="redOff" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        RedModulation, //<xsd:element name="redMod" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        Green, //<xsd:element name="green" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        GreenOff, //<xsd:element name="greenOff" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        GreenModulation, //<xsd:element name="greenMod" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        Blue, //<xsd:element name="blue" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        BlueOff, //<xsd:element name="blueOff" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        BlueModulation, //<xsd:element name="blueMod" type="CT_Percentage" minOccurs="1" maxOccurs="1"/>
        Gamma, //<xsd:element name="gamma" type="CT_GammaTransform" minOccurs="1" maxOccurs="1"/>
        InverseGamma, //<xsd:element name="invGamma" type="CT_InverseGammaTransform" minOccurs="1" maxOccurs="1"/>
    };

    QMap<Type, QVariant> vals;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer) const;
};

class Color
{
public:
    enum class ColorType
    {
        Invalid,
        SimpleColor,
        CRGBColor,
        RGBColor,
        HSLColor,
        SystemColor,
        SchemeColor,
        PresetColor
    };
    enum class SchemeColor
    {
        Background1,
        Text1,
        Background2,
        Text2,
        Accent1,
        Accent2,
        Accent3,
        Accent4,
        Accent5,
        Accent6,
        Hlink,
        FollowedHlink,
        Style,
        Dark1,
        Light1,
        Dark2,
        Light2,
    };

    enum class SystemColor
    {
        None,
        ScrollBar,
        Background,
        ActiveCaption,
        InactiveCaption,
        Menu,
        Window,
        WindowFrame,
        MenuText,
        WindowText,
        CaptionText,
        ActiveBorder,
        InactiveBorder,
        ApplicationWorkspace,
        Highlight,
        HighlightText,
        ButtonFace,
        ButtonShadow,
        GrayText,
        ButtonText,
        InactiveCaptionText,
        ButtonHighlight,
        DarkShadow3d,
        Light3d,
        InfoText,
        InfoBackground,
        HotLight,
        GradientActiveCaption,
        GradientInactiveCaption,
        MenuHighlight,
        MenuBar,
    };

    explicit Color();
    explicit Color(ColorType type);
    explicit Color(ColorType type, QColor color);

    bool isInvalid() const;

    void setRgb(const QColor &color);
    void setIndexedColor(int index);
    void setAutoColor(bool autoColor);
    void setThemeColor(uint theme, double tint = 0.0);
    void setPresetColor(const QString &colorName);
    void setSchemeColor(SchemeColor color);
    void setSystemColor(SystemColor color);

    void addTransform(ColorTransform::Type transform, QVariant val);

    QColor rgb() const;
    int indexedColor() const;
    bool isAutoColor() const;
    QPair<int, double> themeColor() const;
    QColor presetColor() const;
    QString schemeColor() const;
    QString systemColor() const;

    operator QVariant() const;

    static QColor fromARGBString(const QString &c);
    static QString toARGBString(const QColor &c);
    static QString toRGBString(const QColor &c);

    bool write(QXmlStreamWriter &writer, const QString &node=QString()) const;
    bool read(QXmlStreamReader &reader);

private:
    SERIALIZE_ENUM(SchemeColor, {
        {SchemeColor::Background1, "bg1"},
        {SchemeColor::Text1, "tx1"},
        {SchemeColor::Background2, "bg2"},
        {SchemeColor::Text2, "bg1"},
        {SchemeColor::Accent1, "accent1"},
        {SchemeColor::Accent2, "accent2"},
        {SchemeColor::Accent3, "accent3"},
        {SchemeColor::Accent4, "accent4"},
        {SchemeColor::Accent5, "accent5"},
        {SchemeColor::Accent6, "accent6"},
        {SchemeColor::Hlink, "hlink"},
        {SchemeColor::FollowedHlink, "folHlink"},
        {SchemeColor::Style, "phClr"},
        {SchemeColor::Dark1, "dk1"},
        {SchemeColor::Light1, "lt1"},
        {SchemeColor::Dark2, "dk2"},
        {SchemeColor::Light2, "lt2"},
    });
    SERIALIZE_ENUM(SystemColor, {
        {SystemColor::ScrollBar, "scrollBar"},
        {SystemColor::Background, "background"},
        {SystemColor::ActiveCaption, "activeCaption"},
        {SystemColor::InactiveCaption, "inactiveCaption"},
        {SystemColor::Menu, "menu"},
        {SystemColor::Window, "window"},
        {SystemColor::WindowFrame, "windowFrame"},
        {SystemColor::MenuText, "menuText"},
        {SystemColor::WindowText, "windowText"},
        {SystemColor::CaptionText, "captionText"},
        {SystemColor::ActiveBorder, "activeBorder"},
        {SystemColor::InactiveBorder, "inactiveBorder"},
        {SystemColor::ApplicationWorkspace, "appWorkspace"},
        {SystemColor::Highlight, "highlight"},
        {SystemColor::HighlightText, "highlightText"},
        {SystemColor::ButtonFace, "btnFace"},
        {SystemColor::ButtonShadow, "btnShadow"},
        {SystemColor::GrayText, "grayText"},
        {SystemColor::ButtonText, "btnText"},
        {SystemColor::InactiveCaptionText, "inactiveCaptionText"},
        {SystemColor::ButtonHighlight, "btnHighlight"},
        {SystemColor::DarkShadow3d, "3dDkShadow"},
        {SystemColor::Light3d, "3dLight"},
        {SystemColor::InfoText, "infoText"},
        {SystemColor::InfoBackground, "infoBk"},
        {SystemColor::HotLight, "hotLight"},
        {SystemColor::GradientActiveCaption, "gradientActiveCaption"},
        {SystemColor::GradientInactiveCaption, "gradientInactiveCaption"},
        {SystemColor::MenuHighlight, "menuHighlight"},
        {SystemColor::MenuBar, "menuBar"},
    });

    QVariant val;
    ColorType type = ColorType::Invalid;
    ColorTransform tr;
#if !defined(QT_NO_DATASTREAM)
//    friend QDataStream &operator<<(QDataStream &s, const XlsxColor &color);
//    friend QDataStream &operator>>(QDataStream &, XlsxColor &);
    friend QDebug operator<<(QDebug dbg, const Color &c);
#endif
};

#if !defined(QT_NO_DATASTREAM)
//  QDataStream &operator<<(QDataStream &, const XlsxColor &);
//  QDataStream &operator>>(QDataStream &, XlsxColor &);
#endif

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const Color &c);
#endif

QT_END_NAMESPACE_XLSX

Q_DECLARE_METATYPE(QXlsx::Color)

#endif // QXLSX_XLSXCOLOR_P_H
