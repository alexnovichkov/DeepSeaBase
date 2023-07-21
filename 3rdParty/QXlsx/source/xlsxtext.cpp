#include <QtGlobal>
#include <QDebug>
#include <QTextDocument>
#include <QTextFragment>

#include "xlsxtext.h"
#include "xlsxtext_p.h"
#include "xlsxformat_p.h"
#include "xlsxmain.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

//TextPrivate::TextPrivate() : type(Text::Type::None), dirty(true)
//{

//}

//TextPrivate::TextPrivate(const TextPrivate &other) : QSharedData(other),
//    type(other.type),
//    fragmentTexts(other.fragmentTexts),
//    fragmentFormats(other.fragmentFormats),
//    _idKey(other.idKey()),
//    dirty(other.dirty),
//    ref(other.ref),
//    textProperties(other.textProperties)
//{

//}

//TextPrivate::~TextPrivate()
//{

//}

QByteArray TextPrivate::idKey() const
{
    if (dirty) {
        TextPrivate *t = const_cast<TextPrivate *>(this);
        QByteArray bytes;

        //Generate a hash value base on QByteArray ?
        bytes.append("@@QtXlsxText=");
        for (int i=0; i<fragmentTexts.size(); ++i) {
            bytes.append("@Text");
            bytes.append(fragmentTexts[i].toUtf8());
            bytes.append("@Format");
            if (fragmentFormats[i].hasFontData())
                bytes.append(fragmentFormats[i].fontKey());
        }
        if (!ref.isEmpty()) {
            bytes.append("@Ref");
            bytes.append(ref.toUtf8());
        }

        t->_idKey = bytes;
        t->dirty = false;
    }

    return _idKey;
}

Text::Text()
{
}

Text::Text(const QString &text)
{
    d = new TextPrivate;
    d->type = Type::PlainText;
    addFragment(text, Format());
}

Text::Text(Text::Type type)
{
    d = new TextPrivate;
    d->type = type;
}

Text::Text(const Text &other) : d(other.d)
{

}

Text::~Text()
{

}

void Text::setType(Type type)
{
    d->type = type;

//    //adjust formatting
//    if (type != Type::RichText) {
//        d->fragmentFormats.clear();
//        QString s = d->fragmentTexts.join(QString());
//        d->fragmentTexts.clear();
//        d->fragmentTexts << s;
//    }
}

Text::Type Text::type() const
{
    return d->type;
}

void Text::setPlainString(const QString &s)
{
    d->type = Type::PlainText;
    d->fragmentTexts.clear();
    d->fragmentFormats.clear();
    d->ref.clear();
    d->fragmentTexts << s;
}

QString Text::toPlainString() const
{
    return d->fragmentTexts.join(QString());
}

void Text::setHtml(const QString &text)
{
    d->type = Type::RichText;
    d->fragmentTexts.clear();
    d->fragmentFormats.clear();
    d->ref.clear();

    QTextDocument doc;
    doc.setHtml(text);
    QTextBlock block = doc.firstBlock();
    QTextBlock::iterator it;
    for (it = block.begin(); !(it.atEnd()); ++it) {
        QTextFragment textFragment = it.fragment();
        if (textFragment.isValid()) {
            Format fmt;
            fmt.setFont(textFragment.charFormat().font());
            fmt.setFontColor(textFragment.charFormat().foreground().color());
            addFragment(textFragment.text(), fmt);
        }
    }
}

QString Text::toHtml() const
{
    //TODO:
}

void Text::setStringReference(const QString &ref)
{
    d->type = Type::StringRef;
    d->fragmentTexts.clear();
    d->fragmentFormats.clear();
    d->ref = ref;
}

void Text::setStringCashe(const QStringList &cashe)
{
    if (d->type != Type::StringRef) return;
    d->fragmentTexts = cashe;
}

QString Text::stringReference() const
{
    if (d->type != Type::StringRef) return QString();

    return d->ref;
}

QStringList Text::stringCashe() const
{
    if (d->type != Type::StringRef) return {};
    return d->fragmentTexts;
}

bool Text::isRichString() const
{
    return d->type == Type::RichText;
}

bool Text::isPlainString() const
{
    return d->type == Type::RichText;
}

bool Text::isStringReference() const
{
    return d->type == Type::StringRef;
}

bool Text::isNull() const
{
    switch (d->type) {
        case Type::None: return true;
        case Type::PlainText:
        case Type::RichText: return d->fragmentTexts.isEmpty();
        case Type::StringRef: return d->ref.isNull();
    }

    return true;
}

bool Text::isEmpty() const
{
    switch (d->type) {
        case Type::None: return true;
        case Type::PlainText:
        case Type::RichText: return d->fragmentTexts.join(QString()).isEmpty();
        case Type::StringRef: return d->ref.isEmpty();
    }

    return true;
}

int Text::fragmentCount() const
{
    return d->fragmentTexts.size();
}

void Text::addFragment(const QString &text, const Format &format)
{
    d->fragmentTexts.append(text);
    d->fragmentFormats.append(format);
    d->dirty = true;
}

QString Text::fragmentText(int index) const
{
    return d->fragmentTexts.value(index);
}

Format Text::fragmentFormat(int index) const
{
    return d->fragmentFormats.value(index);
}

void Text::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("strRef")) {
                readStringReference(reader);
            }
            else if (reader.name() == QLatin1String("rich")) {
                readRichString(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Text::write(QXmlStreamWriter &writer)
{
    //TODO:
}

Text &Text::operator=(const Text &other)
{
    this->d = other.d;
    return *this;
}

void Text::readStringReference(QXmlStreamReader &reader)
{
    d->type = Type::StringRef;

    const auto &name = reader.name();
    int count = 0;
    int idx = -1;
    QVector<QString> list;
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("f")) {
                d->ref = reader.readElementText();
            }
            else if (reader.name() == QLatin1String("ptCount")) {
                count = reader.attributes().value(QLatin1String("val")).toInt();
                list.resize(count);
            }
            else if (reader.name() == QLatin1String("pt")) {
                idx = reader.attributes().value(QLatin1String("idx")).toInt();
            }
            else if (reader.name() == QLatin1String("v")) {
                QString s = reader.readElementText();
                if (idx >=0 && idx < list.size()) list[idx] = s;
                idx = -1;
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
    d->fragmentTexts.clear();
    std::copy(list.constBegin(), list.constEnd(), std::back_inserter(d->fragmentTexts));
}

void Text::readRichString(QXmlStreamReader &reader)
{
//    <xsd:complexType name="CT_TextBody">
//        <xsd:sequence>
//          <xsd:element name="bodyPr" type="CT_TextBodyProperties" minOccurs="1" maxOccurs="1"/>
//          <xsd:element name="lstStyle" type="CT_TextListStyle" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="p" type="CT_TextParagraph" minOccurs="1" maxOccurs="unbounded"/>
//        </xsd:sequence>
//      </xsd:complexType>
    d->type = Type::RichText;

    const auto &name = reader.name();
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("bodyPr")) {
                readTextProperties(reader);
            }
            else if (reader.name() == QLatin1String("lstStyle")) {
                readTextListStyle(reader);
            }
            else if (reader.name() == QLatin1String("p")) {
                readParagraph(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }

}

void Text::readTextProperties(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();
    parseAttribute(a, QLatin1String("rot"), d->textProperties.rotation);
    parseAttributeBool(a, QLatin1String("spcFirstLastPara"), d->textProperties.spcFirstLastPara);

    if (a.hasAttribute(QLatin1String("vertOverflow"))) {
        const auto &val = a.value(QLatin1String("vertOverflow"));
        if (val == QLatin1String("overflow")) d->textProperties.verticalOverflow = TextProperties::OverflowType::Overflow;
        if (val == QLatin1String("ellipsis")) d->textProperties.verticalOverflow = TextProperties::OverflowType::Ellipsis;
        if (val == QLatin1String("clip")) d->textProperties.verticalOverflow = TextProperties::OverflowType::Clip;
    }
    if (a.hasAttribute(QLatin1String("horzOverflow"))) {
        const auto &val = a.value(QLatin1String("horzOverflow"));
        if (val == QLatin1String("overflow")) d->textProperties.horizontalOverflow = TextProperties::OverflowType::Overflow;
        if (val == QLatin1String("ellipsis")) d->textProperties.horizontalOverflow = TextProperties::OverflowType::Ellipsis;
        if (val == QLatin1String("clip")) d->textProperties.horizontalOverflow = TextProperties::OverflowType::Clip;
    }
    if (a.hasAttribute(QLatin1String("vert"))) {
        const auto &val = a.value(QLatin1String("vert"));
        if (val == QLatin1String("horz")) d->textProperties.verticalOrientation = TextProperties::VerticalType::Horizontal;
        if (val == QLatin1String("vert")) d->textProperties.verticalOrientation = TextProperties::VerticalType::Vertical;
        if (val == QLatin1String("vert270")) d->textProperties.verticalOrientation = TextProperties::VerticalType::Vertical270;
        if (val == QLatin1String("wordArtVert")) d->textProperties.verticalOrientation = TextProperties::VerticalType::WordArtVertical;
        if (val == QLatin1String("eaVert")) d->textProperties.verticalOrientation = TextProperties::VerticalType::EastAsianVertical;
        if (val == QLatin1String("mongolianVert")) d->textProperties.verticalOrientation = TextProperties::VerticalType::MongolianVertical;
        if (val == QLatin1String("wordArtVertRtl")) d->textProperties.verticalOrientation = TextProperties::VerticalType::WordArtVerticalRtl;
    }
    if (a.hasAttribute(QLatin1String("wrap"))) {
        const auto &val = a.value(QLatin1String("wrap"));
        d->textProperties.wrap = val == QLatin1String("square"); //"none" -> false
    }
    parseAttribute(a, QLatin1String("lIns"), d->textProperties.leftInset);
    parseAttribute(a, QLatin1String("rIns"), d->textProperties.rightInset);
    parseAttribute(a, QLatin1String("tIns"), d->textProperties.topInset);
    parseAttribute(a, QLatin1String("bIns"), d->textProperties.bottomInset);
    parseAttributeInt(a, QLatin1String("numCol"), d->textProperties.columnCount);
    parseAttribute(a, QLatin1String("spcCol"), d->textProperties.columnSpace);
    parseAttributeBool(a, QLatin1String("rtlCol"), d->textProperties.columnsRtl);
    parseAttributeBool(a, QLatin1String("fromWordArt"), d->textProperties.fromWordArt);
    parseAttributeBool(a, QLatin1String("anchorCtr"), d->textProperties.anchorCentering);
    parseAttributeBool(a, QLatin1String("forceAA"), d->textProperties.forceAntiAlias);
    parseAttributeBool(a, QLatin1String("upright"), d->textProperties.upright);
    parseAttributeBool(a, QLatin1String("compatLnSpc"), d->textProperties.compatibleLineSpacing);

    if (a.hasAttribute(QLatin1String("anchor"))) {
        const auto &val = a.value(QLatin1String("anchor"));
        if (val == QLatin1String("b")) d->textProperties.anchor = TextProperties::Anchor::Bottom;
        if (val == QLatin1String("t")) d->textProperties.anchor = TextProperties::Anchor::Top;
        if (val == QLatin1String("ctr")) d->textProperties.anchor = TextProperties::Anchor::Center;
        if (val == QLatin1String("just")) d->textProperties.anchor = TextProperties::Anchor::Justified;
        if (val == QLatin1String("dist")) d->textProperties.anchor = TextProperties::Anchor::Distributed;
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("prstTxWarp")) {
                d->textProperties.textShape->read(reader);
            }
            else if (reader.name() == QLatin1String("noAutofit")) {
                d->textProperties.textAutofit = TextProperties::TextAutofit::NoAutofit;
            }
            else if (reader.name() == QLatin1String("spAutoFit")) {
                d->textProperties.textAutofit = TextProperties::TextAutofit::ShapeAutofit;
            }
            else if (reader.name() == QLatin1String("normAutofit")) {
                d->textProperties.textAutofit = TextProperties::TextAutofit::NormalAutofit;
                const auto &a = reader.attributes();
                parseAttributePercent(a, QLatin1String("fontScale"), d->textProperties.fontScale);
                parseAttributePercent(a, QLatin1String("lnSpcReduction"), d->textProperties.lineSpaceReduction);
            }
            else if (reader.name() == QLatin1String("sp3d")) {
                d->textProperties.text3D->read(reader);
            }
            else if (reader.name() == QLatin1String("flatTx")) {
                d->textProperties.z = Coordinate::create(reader.attributes().value(QLatin1String("z")).toString());
            }
            else if (reader.name() == QLatin1String("")) {
                //TODO:
            }

        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Text::readTextListStyle(QXmlStreamReader &reader)
{
    //TODO:
}

void Text::readParagraph(QXmlStreamReader &reader)
{
    //TODO:
}

void Text::writeStringReference(QXmlStreamWriter &writer)
{
    if (d->type != Type::StringRef) return;
    if (d->ref.isEmpty()) {
        writer.writeEmptyElement(QLatin1String("c:strRef"));
        return;
    }

    writer.writeStartElement(QLatin1String("c:strRef"));
    writer.writeTextElement(QLatin1String("c:f"), d->ref);
    if (!d->fragmentTexts.isEmpty()) {
        writer.writeStartElement(QLatin1String("c:strCashe"));
        writer.writeEmptyElement(QLatin1String("c:ptCount"));
        writer.writeAttribute(QLatin1String("val"), QString::number(d->fragmentTexts.size()));
        for (const QString &s: d->fragmentTexts) {
            writer.writeStartElement(QLatin1String("c:pt"));
            writer.writeTextElement(QLatin1String("c:v"), s);
            writer.writeEndElement(); //c:pt
        }
        writer.writeEndElement(); //c:strCashe
    }
    writer.writeEndElement(); //c:strRef
}

void Text::writeRichString(QXmlStreamWriter &writer)
{
//TODO:
}

void Text::writeTextProperties(QXmlStreamWriter &writer)
{
    if (!d->textProperties.isValid()) {
        writer.writeEmptyElement(QLatin1String("a:bodyPr"));
        return;
    }

    writer.writeStartElement(QLatin1String("a:bodyPr"));

    if (d->textProperties.rotation.has_value()) {
        writer.writeAttribute(QLatin1String("rot"), d->textProperties.rotation->toString());
    }
    if (d->textProperties.spcFirstLastPara.has_value()) {
        writer.writeAttribute(QLatin1String("spcFirstLastPara"), toST_Boolean(d->textProperties.spcFirstLastPara.value()));
    }
    if (d->textProperties.verticalOverflow.has_value()) {
        switch (d->textProperties.verticalOverflow.value()) {
            case TextProperties::OverflowType::Overflow:
                writer.writeAttribute(QLatin1String("vertOverflow"), QLatin1String("overflow"));
                break;
            case TextProperties::OverflowType::Ellipsis:
                writer.writeAttribute(QLatin1String("vertOverflow"), QLatin1String("ellipsis"));
                break;
            case TextProperties::OverflowType::Clip:
                writer.writeAttribute(QLatin1String("vertOverflow"), QLatin1String("clip"));
                break;
        }
    }
    if (d->textProperties.horizontalOverflow.has_value()) {
        switch (d->textProperties.horizontalOverflow.value()) {
            case TextProperties::OverflowType::Overflow:
                writer.writeAttribute(QLatin1String("horzOverflow"), QLatin1String("overflow"));
                break;
            case TextProperties::OverflowType::Ellipsis:
                writer.writeAttribute(QLatin1String("horzOverflow"), QLatin1String("ellipsis"));
                break;
            case TextProperties::OverflowType::Clip:
                writer.writeAttribute(QLatin1String("horzOverflow"), QLatin1String("clip"));
                break;
        }
    }
    if (d->textProperties.verticalOrientation.has_value()) {
        switch (d->textProperties.verticalOrientation.value()) {
            case TextProperties::VerticalType::Horizontal:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("horz"));
                break;
            case TextProperties::VerticalType::Vertical:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("vert"));
                break;
            case TextProperties::VerticalType::Vertical270:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("vert270"));
                break;
            case TextProperties::VerticalType::WordArtVertical:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("wordArtVert"));
                break;
            case TextProperties::VerticalType::EastAsianVertical:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("eaVert"));
                break;
            case TextProperties::VerticalType::MongolianVertical:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("mongolianVert"));
                break;
            case TextProperties::VerticalType::WordArtVerticalRtl:
                writer.writeAttribute(QLatin1String("vert"), QLatin1String("wordArtVertRtl"));
                break;
        }
    }

    if (d->textProperties.wrap.has_value()) {
        writer.writeAttribute(QLatin1String("wrap"),
                              d->textProperties.wrap.value() ? QLatin1String("square") : QLatin1String("none"));
    }
    if (d->textProperties.leftInset.has_value()) {
        writer.writeAttribute(QLatin1String("lIns"), d->textProperties.leftInset->toString());
    }
    if (d->textProperties.rightInset.has_value()) {
        writer.writeAttribute(QLatin1String("rIns"), d->textProperties.rightInset->toString());
    }
    if (d->textProperties.topInset.has_value()) {
        writer.writeAttribute(QLatin1String("tIns"), d->textProperties.topInset->toString());
    }
    if (d->textProperties.bottomInset.has_value()) {
        writer.writeAttribute(QLatin1String("bIns"), d->textProperties.bottomInset->toString());
    }
    if (d->textProperties.columnCount.has_value()) {
        writer.writeAttribute(QLatin1String("numCol"), QString::number(d->textProperties.columnCount.value()));
    }
    if (d->textProperties.columnSpace.has_value()) {
        writer.writeAttribute(QLatin1String("spcCol"), d->textProperties.columnSpace->toString());
    }
    if (d->textProperties.columnsRtl.has_value()) {
        writer.writeAttribute(QLatin1String("rtlCol"), toST_Boolean(d->textProperties.columnsRtl.value()));
    }
    if (d->textProperties.fromWordArt.has_value()) {
        writer.writeAttribute(QLatin1String("fromWordArt"), toST_Boolean(d->textProperties.fromWordArt.value()));
    }
    if (d->textProperties.anchorCentering.has_value()) {
        writer.writeAttribute(QLatin1String("anchorCtr"), toST_Boolean(d->textProperties.anchorCentering.value()));
    }
    if (d->textProperties.forceAntiAlias.has_value()) {
        writer.writeAttribute(QLatin1String("forceAA"), toST_Boolean(d->textProperties.forceAntiAlias.value()));
    }
    if (d->textProperties.upright.has_value()) {
        writer.writeAttribute(QLatin1String("upright"), toST_Boolean(d->textProperties.upright.value()));
    }
    if (d->textProperties.compatibleLineSpacing.has_value()) {
        writer.writeAttribute(QLatin1String("compatLnSpc"), toST_Boolean(d->textProperties.compatibleLineSpacing.value()));
    }
    if (d->textProperties.anchor.has_value()) {
        switch (d->textProperties.anchor.value()) {
            case TextProperties::Anchor::Bottom:
                writer.writeAttribute(QLatin1String("anchor"), QLatin1String("b")); break;
            case TextProperties::Anchor::Top:
                writer.writeAttribute(QLatin1String("anchor"), QLatin1String("t")); break;
            case TextProperties::Anchor::Center:
                writer.writeAttribute(QLatin1String("anchor"), QLatin1String("ctr")); break;
            case TextProperties::Anchor::Justified:
                writer.writeAttribute(QLatin1String("anchor"), QLatin1String("just")); break;
            case TextProperties::Anchor::Distributed:
                writer.writeAttribute(QLatin1String("anchor"), QLatin1String("dist")); break;
        }
    }
    if (d->textProperties.textShape.has_value())
        d->textProperties.textShape.value().write(writer, QLatin1String("a:prstTxWarp"));

    if (d->textProperties.textAutofit.has_value()) {
        switch (d->textProperties.textAutofit.value()) {
            case TextProperties::TextAutofit::NoAutofit:
                writer.writeEmptyElement(QLatin1String("a:noAutofit")); break;
            case TextProperties::TextAutofit::NormalAutofit:
                writer.writeEmptyElement(QLatin1String("a:normAutofit"));
                if (d->textProperties.fontScale.has_value())
                    writer.writeAttribute(QLatin1String("fontScale"), toST_Percent(d->textProperties.fontScale.value()));
                if (d->textProperties.fontScale.has_value())
                    writer.writeAttribute(QLatin1String("lnSpcReduction"), toST_Percent(d->textProperties.lineSpaceReduction.value()));
                break;
            case TextProperties::TextAutofit::ShapeAutofit:
                writer.writeEmptyElement(QLatin1String("a:spAutofit")); break;
        }
    }
    if (d->textProperties.text3D.has_value())
        d->textProperties.text3D.value().write(writer, QLatin1String("a:sp3d"));
    else if (d->textProperties.z.has_value()) {
        writer.writeEmptyElement(QLatin1String("a:flatTx"));
        writer.writeAttribute(QLatin1String("z"), d->textProperties.z.value().toString());
    }

    writer.writeEndElement();
}

QXlsx::Text::operator QVariant() const
{
    const auto& cref
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
        = QMetaType::fromType<RichString>();
#else
        = qMetaTypeId<Text>() ;
#endif
    return QVariant(cref, this);
}

/*!
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator==(const Text &rs1, const Text &rs2)
{
    return rs1.d->idKey() == rs2.d->idKey();
}

/*!
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator!=(const Text &rs1, const Text &rs2)
{
    return rs1.d->idKey() != rs2.d->idKey();
}

/*!
 * \internal
 */
bool operator<(const Text &rs1, const Text &rs2)
{
    return rs1.d->idKey() < rs2.d->idKey();
}

/*!
    \overload
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator ==(const Text &rs1, const QString &rs2)
{
    return rs1.d->fragmentTexts.join(QString()) == rs2;
}

/*!
    \overload
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator !=(const Text &rs1, const QString &rs2)
{
    return rs1.d->fragmentTexts.join(QString()) != rs2;
}

/*!
    \overload
    Returns true if this string \a rs1 is equal to string \a rs2;
    otherwise returns false.
 */
bool operator ==(const QString &rs1, const Text &rs2)
{
    return rs2 == rs1;
}

/*!
    \overload
    Returns true if this string \a rs1 is not equal to string \a rs2;
    otherwise returns false.
 */
bool operator !=(const QString &rs1, const Text &rs2)
{
    return rs2 != rs1;
}

uint qHash(const Text &rs, uint seed) Q_DECL_NOTHROW
{
   return qHash(rs.d->idKey(), seed);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Text &rs)
{
    switch (rs.d->type) {
        case Text::Type::RichText:
            dbg.nospace() << "QXlsx::RichString(" << rs.toHtml() << ")";
            break;
        case Text::Type::PlainText:
            dbg.nospace() << "QXlsx::RichString(" << rs.d->fragmentTexts.join(QString()) << ")";
            break;
        case Text::Type::StringRef:
            dbg.nospace() << "QXlsx::RichString(" << rs.d->ref << ")";
        default: break;
    }
    return dbg.space();
}

bool TextProperties::isValid() const
{
    if (spcFirstLastPara.has_value()) return true;
    if (verticalOverflow.has_value()) return true;
    if (horizontalOverflow.has_value()) return true;
    if (verticalOrientation.has_value()) return true;
    if (rotation.has_value()) return true;
    if (wrap.has_value()) return true;
    if (leftInset.has_value()) return true;
    if (rightInset.has_value()) return true;
    if (topInset.has_value()) return true;
    if (bottomInset.has_value()) return true;
    if (columnCount.has_value()) return true;
    if (columnSpace.has_value()) return true;
    if (columnsRtl.has_value()) return true;
    if (fromWordArt.has_value()) return true;
    if (anchor.has_value()) return true;
    if (anchorCentering.has_value()) return true;
    if (forceAntiAlias.has_value()) return true;
    if (upright.has_value()) return true;
    if (compatibleLineSpacing.has_value()) return true;

    return false;
}

TextSpacing::TextSpacing()
{

}

TextSpacing::TextSpacing(uint points)
{
    val = QVariant::fromValue<uint>(points);
}

TextSpacing::TextSpacing(double percents)
{
    val = QVariant::fromValue<double>(percents);
}

int TextSpacing::toPoints() const
{
    return val.toUInt();
}

double TextSpacing::toPercents() const
{
    return val.toDouble();
}

QString TextSpacing::toString() const
{
    if (val.userType() == QMetaType::UInt) return QString::number(val.toUInt());
    if (val.userType() == QMetaType::Double) return QString::number(val.toDouble())+'%';
    return QString();
}

void TextSpacing::setPoints(int points)
{
    val = QVariant::fromValue<uint>(points);
}

void TextSpacing::setPercents(double percents)
{
    val = QVariant::fromValue<double>(percents);
}

void ParagraphProperties::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    const auto &a = reader.attributes();

    if (a.hasAttribute(QLatin1String("marL"))) {

    }
    if (a.hasAttribute(QLatin1String("marR"))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }
    if (a.hasAttribute(QLatin1String(""))) {

    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("")) {

            }
            else if (reader.name() == QLatin1String("")) {

            }
            else if (reader.name() == QLatin1String("")) {

            }
            else if (reader.name() == QLatin1String("")) {

            }
            else if (reader.name() == QLatin1String("")) {

            }
            else if (reader.name() == QLatin1String("")) {

            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

#endif

QT_END_NAMESPACE_XLSX
