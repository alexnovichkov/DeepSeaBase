#ifndef XLSXFONT_H
#define XLSXFONT_H

#include <QtGlobal>
#include <QVariant>
#include <QFont>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QSharedDataPointer>

#include "xlsxglobal.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

//<xsd:complexType name="CT_TextFont">
//    <xsd:attribute name="typeface" type="ST_TextTypeface" use="required"/>
//    <xsd:attribute name="panose" type="s:ST_Panose" use="optional"/>
//    <xsd:attribute name="pitchFamily" type="ST_PitchFamily" use="optional" default="0"/>
//    <xsd:attribute name="charset" type="xsd:byte" use="optional" default="1"/>
//</xsd:complexType>

class FontPrivate;

class Font
{
public:
    enum class PitchFamily
    {
        DefaultPitchUnknownFamily = 0x00,
        FixedPitchUnknownFamily = 0x01,
        VariablePitchUnknownFamily = 0x02,
        DefaultPitchRomanFamily = 0x10,
        FixedPitchRomanFamily = 0x11,
        VariablePitchRomanFamily = 0x12,
        DefaultPitchSwissFamily = 0x20,
        FixedPitchSwissFamily = 0x21,
        VariablePitchSwissFamily = 0x22,
        DefaultPitchModernFamily = 0x30,
        FixedPitchModernFamily = 0x31,
        VariablePitchModernFamily = 0x32,
        DefaultPitchScriptFamily = 0x40,
        FixedPitchScriptFamily = 0x41,
        VariablePitchScriptFamily = 0x42,
        DefaultPitchDecorativeFamily = 0x50,
        FixedPitchDecorativeFamily = 0x51,
        VariablePitchDecorativeFamily = 0x52,
    };
    enum class Charset
    {
        Ansi = 0x00, // iso-8859-1
        Default = 0x01,
        Symbol = 0x02, // This value specifies that the characters in the
        //Unicode private use area (U+FF00 to U+FFFF) of the font should be
        //used to display characters in the range U+0000 to U+00FF.
        Macintosh = 0x4D, // macintosh
        JIS = 0x80, // shift_jis
        Hangul = 0x81, // ks_c_5601-1987
        Johab = 0x82, // KS C-5601-1992
        GB2312 = 0x86, // GBK
        Big5 = 0x88,
        Greek = 0xA1, // windows-1253
        Turkish = 0xA2, // iso-8859-9
        Vietnamese = 0xA3, // windows-1258
        Hebrew = 0xB1, // windows-1255
        Arabic = 0xB2, // windows-1256
        Baltic = 0xBA, // windows-1257
        Russian = 0xCC, // windows-1251
        Thai = 0xDE, // windows-874
        EasternEuropean = 0xEE, // windows-1250
        OEM = 0xFF, // not defined by ECMA-376
    };

    Font();
    explicit Font(QFont font);
    Font(const Font &other);
    Font &operator =(const Font &other);
    ~Font();

    bool isValid() const;

    //getters
    QFont font() const;
    QString typeface() const;
    Charset charset() const;
    PitchFamily pitchAndFamilySubstitute() const;

    //setters
    void setFont(const QFont &font);
    void setTypeface(const QString &typeface);
    void setCharset(Charset charset);
    void setPitchAndFamilySubstitute(PitchFamily val);

    operator QVariant() const;

    void write(QXmlStreamWriter &writer, const QString &name) const;
    void read(QXmlStreamReader &reader);

private:
    QSharedDataPointer<FontPrivate> d;
//    SERIALIZE_ENUM(SchemeColor, {
//        {SchemeColor::Background1, "bg1"},
//        {SchemeColor::Text1, "tx1"},
//        {SchemeColor::Background2, "bg2"},
//        {SchemeColor::Text2, "bg1"},
//        {SchemeColor::Accent1, "accent1"},
//        {SchemeColor::Accent2, "accent2"},
//        {SchemeColor::Accent3, "accent3"},
//        {SchemeColor::Accent4, "accent4"},
//        {SchemeColor::Accent5, "accent5"},
//        {SchemeColor::Accent6, "accent6"},
//        {SchemeColor::Hlink, "hlink"},
//        {SchemeColor::FollowedHlink, "folHlink"},
//        {SchemeColor::Style, "phClr"},
//        {SchemeColor::Dark1, "dk1"},
//        {SchemeColor::Light1, "lt1"},
//        {SchemeColor::Dark2, "dk2"},
//        {SchemeColor::Light2, "lt2"},
//    });

#if !defined(QT_NO_DATASTREAM)
//    friend QDataStream &operator<<(QDataStream &s, const XlsxColor &color);
//    friend QDataStream &operator>>(QDataStream &, XlsxColor &);
    friend QDebug operator<<(QDebug dbg, const Font &c);
#endif
};

#if !defined(QT_NO_DATASTREAM)
//  QDataStream &operator<<(QDataStream &, const XlsxColor &);
//  QDataStream &operator>>(QDataStream &, XlsxColor &);
#endif

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const Font &c);
#endif

QT_END_NAMESPACE_XLSX

Q_DECLARE_METATYPE(QXlsx::Font)

#endif // XLSXFONT_H
