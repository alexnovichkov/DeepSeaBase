#ifndef XLSXTEXT_H
#define XLSXTEXT_H

#include "xlsxglobal.h"
#include "xlsxformat.h"
#include <QVariant>
#include <QStringList>
#include <QSharedDataPointer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "xlsxmain.h"
#include "xlsxfont.h"

QT_BEGIN_NAMESPACE_XLSX

class Text;

uint qHash(const Text &rs, uint seed = 0) Q_DECL_NOTHROW;

class QXLSX_EXPORT TextProperties
{
public:
    enum class OverflowType
    {
        Overflow,
        Ellipsis,
        Clip
    };
    enum class VerticalType
    {
        EastAsianVertical,
        Horizontal,
        MongolianVertical,
        Vertical,
        Vertical270,
        WordArtVertical,
        WordArtVerticalRtl,
    };
    enum class Anchor
    {
        Bottom,
        Center,
        Distributed,
        Justified,
        Top
    };
    enum class TextAutofit
    {
        NoAutofit,
        NormalAutofit,
        ShapeAutofit
    };

    //Specifies whether the before and after paragraph spacing defined by the user is to be respected
    std::optional<bool> spcFirstLastPara;
    std::optional<OverflowType> verticalOverflow;
    std::optional<OverflowType> horizontalOverflow;
    std::optional<VerticalType> verticalOrientation;
    std::optional<Angle> rotation;
    std::optional<bool> wrap;
    std::optional<Coordinate> leftInset;
    std::optional<Coordinate> rightInset;
    std::optional<Coordinate> topInset;
    std::optional<Coordinate> bottomInset;
    std::optional<int> columnCount;
    std::optional<Coordinate> columnSpace;
    std::optional<bool> columnsRtl;
    std::optional<bool> fromWordArt;
    std::optional<Anchor> anchor;
    std::optional<bool> anchorCentering;
    std::optional<bool> forceAntiAlias;
    std::optional<bool> upright;
    std::optional<bool> compatibleLineSpacing;
    std::optional<PresetTextShape> textShape;

    // textScale and lineSpaceReduction are only valid if textAutofit == NormalAutofit
    std::optional<TextAutofit> textAutofit;
    std::optional<double> fontScale; // in %
    std::optional<double> lineSpaceReduction; // in %

    // either text3D or z
    std::optional<Scene3D> text3D;
    std::optional<Coordinate> z;

    //QByteArray idKey() const;
    bool isValid() const;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer);
};

/**
 * @brief The Size class
 * The class is used to set a size either in points or in percents.
 * Size can either be set as a whole positive number (int) or as a percentage (double).
 */
class Size
{
public:
    Size();
    Size(uint points); // [0..158400]
    Size(double percents); // 34.0 = 34.0%

    int toPoints() const;
    double toPercents() const;
    QString toString() const;

    void setPoints(int points);
    void setPercents(double percents);

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer, const QString &name) const;
private:
    QVariant val;
};

class QXLSX_EXPORT ParagraphProperties
{
//    <xsd:complexType name="CT_TextParagraphProperties">
//      <xsd:sequence>
//        <xsd:element name="lnSpc" type="CT_TextSpacing" minOccurs="0" maxOccurs="1"/>
//        <xsd:element name="spcBef" type="CT_TextSpacing" minOccurs="0" maxOccurs="1"/>
//        <xsd:element name="spcAft" type="CT_TextSpacing" minOccurs="0" maxOccurs="1"/>
//        <xsd:group ref="EG_TextBulletColor" minOccurs="0" maxOccurs="1"/>
//        <xsd:group ref="EG_TextBulletSize" minOccurs="0" maxOccurs="1"/>
//        <xsd:group ref="EG_TextBulletTypeface" minOccurs="0" maxOccurs="1"/>
//        <xsd:group ref="EG_TextBullet" minOccurs="0" maxOccurs="1"/>
//        <xsd:element name="tabLst" type="CT_TextTabStopList" minOccurs="0" maxOccurs="1"/>
//        <xsd:element name="defRPr" type="CT_TextCharacterProperties" minOccurs="0" maxOccurs="1"/>
//        <xsd:element name="extLst" type="CT_OfficeArtExtensionList" minOccurs="0" maxOccurs="1"/>
//      </xsd:sequence>
//      <xsd:attribute name="marL" type="ST_TextMargin" use="optional"/>
//      <xsd:attribute name="marR" type="ST_TextMargin" use="optional"/>
//      <xsd:attribute name="lvl" type="ST_TextIndentLevelType" use="optional"/>
//      <xsd:attribute name="indent" type="ST_TextIndent" use="optional"/>
//      <xsd:attribute name="algn" type="ST_TextAlignType" use="optional"/>
//      <xsd:attribute name="defTabSz" type="ST_Coordinate32" use="optional"/>
//      <xsd:attribute name="rtl" type="xsd:boolean" use="optional"/>
//      <xsd:attribute name="eaLnBrk" type="xsd:boolean" use="optional"/>
//      <xsd:attribute name="fontAlgn" type="ST_TextFontAlignType" use="optional"/>
//      <xsd:attribute name="latinLnBrk" type="xsd:boolean" use="optional"/>
//      <xsd:attribute name="hangingPunct" type="xsd:boolean" use="optional"/>
//    </xsd:complexType>
public:
    enum class TextAlign
    {
        Left,
        Center,
        Right,
        Justify,
        JustifyLow,
        Distrubute,
        DistrubuteThai,
    };

    enum class FontAlign
    {
        Auto,
        Top,
        Center,
        Base,
        Bottom
    };

    enum class BulletType
    {
        None,
        AutoNumber,
        Char,
        Blip
    };

    enum class BulletAutonumberType
    {
        AlphaLowcaseParentheses, // (a), (b), (c)
        AlphaUppercaseParentheses, // (A), (B), (C)
        AlphaLowcaseRightParentheses, // a), b), c)
        AlphaUppercaseRightParentheses, // A), B), C)
        AlphaLowcasePeriod, // a., b., c.
        AlphaUppercasePeriod, // A., B., C.
        ArabicParentheses, // (1), (2), (3)
        ArabicRightParentheses, // 1), 2), 3)
        ArabicPeriod, // 1., 2., 3.
        ArabicPlain, // 1, 2, 3
        RomanLowcaseParentheses, // (i), (ii), (iii)
        RomanUppercaseParentheses, // (I), (II), (III)
        RomanLowcaseRightParentheses, // i), ii), iii)
        RomanUppercaseRightParentheses, // I), II), III)
        RomanLowcasePeriod, // i., ii., iii.
        RomanUppercasePeriod, // I., II, III.
        Circle,
        CircleWindingsBlack,
        CircleWindingsWhite,
        ArabicDoubleBytePeriod,
        ArabicDoubleByte,
        SimplifiedChinesePeriod,
        SimplifiedChinese,
        TraditionalChinesePeriod,
        TraditionalChinese,
        JapanesePeriod,
        JapaneseKorean,
        JapaneseKoreanPeriod,
        BidiArabic1Minus,
        BidiArabic2Minus,
        BidiHebrewMinus,
        ThaiAlphaPeriod,
        ThaiAlphaRightParentheses,
        ThaiAlphaParentheses,
        ThaiNumberPeriod,
        ThaiNumberRightParentheses,
        ThaiNumberParentheses,
        HindiAlphaPeriod,
        HindiNumberPeriod,
        HindiNumberParentheses,
        HindiAlpha1Period,
    };

    enum class TabAlign
    {
        Left,
        Center,
        Right,
        Decimal
    };

    std::optional<Coordinate> leftMargin;
    std::optional<Coordinate> rightMargin;
    std::optional<Coordinate> indent;
    std::optional<int> indentLevel; //[0..8]
    std::optional<TextAlign> align;
    std::optional<Coordinate> defaultTabSize;
    std::optional<bool> rtl;
    std::optional<bool> eastAsianLineBreak;
    std::optional<bool> latinLineBreak;
    std::optional<bool> hangingPunctuation;
    std::optional<FontAlign> fontAlign;

    std::optional<Size> lineSpacing;
    std::optional<Size> spacingBefore;
    std::optional<Size> spacingAfter;

    std::optional<Color> bulletColor;
    std::optional<Size> bulletSize;
    Font bulletFont;
    std::optional<BulletType> bulletType;
    BulletAutonumberType bulletAutonumberType; //makes sense only if bulletType == Autonumber
    std::optional<int> bulletAutonumberStart;
    QString bulletChar; //makes sense only if bulletType == Char

    QList<QPair<Coordinate, TabAlign>> tabStops;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer, const QString &name) const;

    SERIALIZE_ENUM(BulletAutonumberType, {
        {BulletAutonumberType::AlphaLowcaseParentheses, "alphaLcParenBoth"}, // (a), (b), (c)
        {BulletAutonumberType::AlphaUppercaseParentheses, "alphaUcParenBoth"}, // (A), (B), (C)
        {BulletAutonumberType::AlphaLowcaseRightParentheses, "alphaLcParenR"}, // a), b), c)
        {BulletAutonumberType::AlphaUppercaseRightParentheses, "alphaUcParenR"}, // A), B), C)
        {BulletAutonumberType::AlphaLowcasePeriod, "alphaLcPeriod"}, // a., b., c.
        {BulletAutonumberType::AlphaUppercasePeriod, "alphaUcPeriod"}, // A., B., C.
        {BulletAutonumberType::ArabicParentheses, "arabicParenBoth"}, // (1), (2), (3)
        {BulletAutonumberType::ArabicRightParentheses, "arabicParenR"}, // 1), 2), 3)
        {BulletAutonumberType::ArabicPeriod, "arabicPeriod"}, // 1., 2., 3.
        {BulletAutonumberType::ArabicPlain, "arabicPlain"}, // 1, 2, 3
        {BulletAutonumberType::RomanLowcaseParentheses, "romanLcParenBoth"}, // (i), (ii), (iii)
        {BulletAutonumberType::RomanUppercaseParentheses, "romanUcParenBoth"}, // (I), (II), (III)
        {BulletAutonumberType::RomanLowcaseRightParentheses, "romanLcParenR"}, // i), ii), iii)
        {BulletAutonumberType::RomanUppercaseRightParentheses, "romanUcParenR"}, // I), II), III)
        {BulletAutonumberType::RomanLowcasePeriod, "romanLcPeriod"}, // i., ii., iii.
        {BulletAutonumberType::RomanUppercasePeriod, "romanUcPeriod"}, //I., II, III.
        {BulletAutonumberType::Circle, "circleNumDbPlain"},
        {BulletAutonumberType::CircleWindingsBlack, "circleNumWdBlackPlain"},
        {BulletAutonumberType::CircleWindingsWhite, "circleNumWdWhitePlain"},
        {BulletAutonumberType::ArabicDoubleBytePeriod, "arabicDbPeriod"},
        {BulletAutonumberType::ArabicDoubleByte, "arabicDbPlain"},
        {BulletAutonumberType::SimplifiedChinesePeriod, "ea1ChsPeriod"},
        {BulletAutonumberType::SimplifiedChinese, "ea1ChsPlain"},
        {BulletAutonumberType::TraditionalChinesePeriod, "ea1ChtPeriod"},
                       {BulletAutonumberType::TraditionalChinese, "ea1ChtPlain"},
                       {BulletAutonumberType::JapanesePeriod, "ea1JpnChsDbPeriod"},
                       {BulletAutonumberType::JapaneseKorean, "ea1JpnKorPlain"},
                       {BulletAutonumberType::JapaneseKoreanPeriod, "ea1JpnKorPeriod"},
                       {BulletAutonumberType::BidiArabic1Minus, "arabic1Minus"},
                       {BulletAutonumberType::BidiArabic2Minus, "arabic2Minus"},
                       {BulletAutonumberType::BidiHebrewMinus, "hebrew2Minus"},
                       {BulletAutonumberType::ThaiAlphaPeriod, "thaiAlphaPeriod"},
                       {BulletAutonumberType::ThaiAlphaRightParentheses, "thaiAlphaParenR"},
                       {BulletAutonumberType::ThaiAlphaParentheses, "thaiAlphaParenBoth"},
                       {BulletAutonumberType::ThaiNumberPeriod, "thaiNumPeriod"},
                       {BulletAutonumberType::ThaiNumberRightParentheses, "thaiNumParenR"},
                       {BulletAutonumberType::ThaiNumberParentheses, "thaiNumParenBoth"},
                       {BulletAutonumberType::HindiAlphaPeriod, "hindiAlphaPeriod"},
                       {BulletAutonumberType::HindiNumberPeriod, "hindiNumPeriod"},
                       {BulletAutonumberType::HindiNumberParentheses, "hindiNumParenR"},
                       {BulletAutonumberType::HindiAlpha1Period, "hindiAlpha1Period"},
    });
    SERIALIZE_ENUM(TabAlign, {
        {TabAlign::Left, "l"},
        {TabAlign::Right, "r"},
        {TabAlign::Center, "ctr"},
        {TabAlign::Decimal, "dec"},
    });
private:
    void readTabStops(QXmlStreamReader &reader);
    void writeTabStops(QXmlStreamWriter &writer, const QString &name) const;
};

class QXLSX_EXPORT ListStypeProperties
{
public:
    QVector<ParagraphProperties> vals;
    bool isEmpty() const;
    ParagraphProperties getDefault() const;
    ParagraphProperties value(int level) const;
    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer, const QString &name) const;
};

class TextPrivate;

class QXLSX_EXPORT Text
{
    //<xsd:complexType name="CT_Tx">
    //  <xsd:sequence>
    //    <xsd:choice minOccurs="1" maxOccurs="1">
    //      <xsd:element name="strRef" type="CT_StrRef" minOccurs="1" maxOccurs="1"/>
    //      <xsd:element name="rich" type="a:CT_TextBody" minOccurs="1" maxOccurs="1"/>
    //    </xsd:choice>
    //  </xsd:sequence>
    //</xsd:complexType>
public:
    enum class Type {
        None,
        StringRef,
        RichText,
        PlainText
    };

    Text(); // null text of type None
    explicit Text(const QString& text); // text of type PlainText with no formatting
    explicit Text(Type type); //null text of specific type

    Text(const Text &other);
    ~Text();

    void setType(Type type);
    Type type() const;

    void setPlainString(const QString &s);
    QString toPlainString() const;

    void setHtml(const QString &text);
    QString toHtml() const;

    void setStringReference(const QString &ref);
    void setStringCashe(const QStringList &cashe);
    QString stringReference() const;
    QStringList stringCashe() const;


    bool isRichString() const; //only checks type, ignores actual formatting
    bool isPlainString() const;
    bool isStringReference() const;
    bool isNull() const;
    bool isEmpty() const;

    int fragmentCount() const;
    void addFragment(const QString &text, const Format &format);
    QString fragmentText(int index) const;
    Format fragmentFormat(int index) const;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer);

    operator QVariant() const;

    Text &operator=(const Text &other);
private:
    void readStringReference(QXmlStreamReader &reader);
    void readRichString(QXmlStreamReader &reader);
    void readParagraph(QXmlStreamReader &reader);


    void writeStringReference(QXmlStreamWriter &writer);
    void writeRichString(QXmlStreamWriter &writer);
    void writeTextProperties(QXmlStreamWriter &writer);
    friend uint qHash(const Text &rs, uint seed) Q_DECL_NOTHROW;
    friend bool operator==(const Text &rs1, const Text &rs2);
    friend bool operator!=(const Text &rs1, const Text &rs2);
    friend bool operator<(const Text &rs1, const Text &rs2);
    friend bool operator==(const Text &rs1, const QString &rs2);
    friend bool operator==(const QString &rs1, const Text &rs2);
    friend bool operator!=(const Text &rs1, const QString &rs2);
    friend bool operator!=(const QString &rs1, const Text &rs2);
    friend QDebug operator<<(QDebug dbg, const Text &rs);

    //QSharedDataPointer<TextPrivate> d;
    TextPrivate *d = nullptr;
};

  bool operator==(const Text &rs1, const Text &rs2);
  bool operator!=(const Text &rs1, const Text &rs2);
  bool operator<(const Text &rs1, const Text &rs2);
  bool operator==(const Text &rs1, const QString &rs2);
  bool operator==(const QString &rs1, const Text &rs2);
  bool operator!=(const Text &rs1, const QString &rs2);
  bool operator!=(const QString &rs1, const Text &rs2);

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Text &rs);
#endif

class TextPrivate //: public QSharedData
{
public:
//    TextPrivate();
//    TextPrivate(const TextPrivate &other);
//    ~TextPrivate();

    QByteArray idKey() const;

    Text::Type type = Text::Type::None;
    QStringList fragmentTexts;
    QList<Format> fragmentFormats;
    QByteArray _idKey;
    bool dirty = true;
    QString ref;

    TextProperties textProperties; //element, required
    ListStypeProperties paragraphProperties; //element, optional
};

QT_END_NAMESPACE_XLSX

Q_DECLARE_METATYPE(QXlsx::Text)

#endif // XLSXTEXT_H
