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
    delete d;
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
                d->textProperties.read(reader);
            }
            else if (reader.name() == QLatin1String("lstStyle")) {
                d->paragraphProperties.read(reader);
            }
            else if (reader.name() == QLatin1String("p")) {
                readParagraph(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }

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

void TextProperties::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();
    parseAttribute(a, QLatin1String("rot"), rotation);
    parseAttributeBool(a, QLatin1String("spcFirstLastPara"), spcFirstLastPara);

    if (a.hasAttribute(QLatin1String("vertOverflow"))) {
        const auto &val = a.value(QLatin1String("vertOverflow"));
        if (val == QLatin1String("overflow")) verticalOverflow = TextProperties::OverflowType::Overflow;
        if (val == QLatin1String("ellipsis")) verticalOverflow = TextProperties::OverflowType::Ellipsis;
        if (val == QLatin1String("clip")) verticalOverflow = TextProperties::OverflowType::Clip;
    }
    if (a.hasAttribute(QLatin1String("horzOverflow"))) {
        const auto &val = a.value(QLatin1String("horzOverflow"));
        if (val == QLatin1String("overflow")) horizontalOverflow = TextProperties::OverflowType::Overflow;
        if (val == QLatin1String("ellipsis")) horizontalOverflow = TextProperties::OverflowType::Ellipsis;
        if (val == QLatin1String("clip")) horizontalOverflow = TextProperties::OverflowType::Clip;
    }
    if (a.hasAttribute(QLatin1String("vert"))) {
        const auto &val = a.value(QLatin1String("vert"));
        if (val == QLatin1String("horz")) verticalOrientation = TextProperties::VerticalType::Horizontal;
        if (val == QLatin1String("vert")) verticalOrientation = TextProperties::VerticalType::Vertical;
        if (val == QLatin1String("vert270")) verticalOrientation = TextProperties::VerticalType::Vertical270;
        if (val == QLatin1String("wordArtVert")) verticalOrientation = TextProperties::VerticalType::WordArtVertical;
        if (val == QLatin1String("eaVert")) verticalOrientation = TextProperties::VerticalType::EastAsianVertical;
        if (val == QLatin1String("mongolianVert")) verticalOrientation = TextProperties::VerticalType::MongolianVertical;
        if (val == QLatin1String("wordArtVertRtl")) verticalOrientation = TextProperties::VerticalType::WordArtVerticalRtl;
    }
    if (a.hasAttribute(QLatin1String("wrap"))) {
        const auto &val = a.value(QLatin1String("wrap"));
        wrap = val == QLatin1String("square"); //"none" -> false
    }
    parseAttribute(a, QLatin1String("lIns"), leftInset);
    parseAttribute(a, QLatin1String("rIns"), rightInset);
    parseAttribute(a, QLatin1String("tIns"), topInset);
    parseAttribute(a, QLatin1String("bIns"), bottomInset);
    parseAttributeInt(a, QLatin1String("numCol"), columnCount);
    parseAttribute(a, QLatin1String("spcCol"), columnSpace);
    parseAttributeBool(a, QLatin1String("rtlCol"), columnsRtl);
    parseAttributeBool(a, QLatin1String("fromWordArt"), fromWordArt);
    parseAttributeBool(a, QLatin1String("anchorCtr"), anchorCentering);
    parseAttributeBool(a, QLatin1String("forceAA"), forceAntiAlias);
    parseAttributeBool(a, QLatin1String("upright"), upright);
    parseAttributeBool(a, QLatin1String("compatLnSpc"), compatibleLineSpacing);

    if (a.hasAttribute(QLatin1String("anchor"))) {
        const auto &val = a.value(QLatin1String("anchor"));
        if (val == QLatin1String("b")) anchor = TextProperties::Anchor::Bottom;
        if (val == QLatin1String("t")) anchor = TextProperties::Anchor::Top;
        if (val == QLatin1String("ctr")) anchor = TextProperties::Anchor::Center;
        if (val == QLatin1String("just")) anchor = TextProperties::Anchor::Justified;
        if (val == QLatin1String("dist")) anchor = TextProperties::Anchor::Distributed;
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("prstTxWarp")) {
                textShape->read(reader);
            }
            else if (reader.name() == QLatin1String("noAutofit")) {
                textAutofit = TextProperties::TextAutofit::NoAutofit;
            }
            else if (reader.name() == QLatin1String("spAutoFit")) {
                textAutofit = TextProperties::TextAutofit::ShapeAutofit;
            }
            else if (reader.name() == QLatin1String("normAutofit")) {
                textAutofit = TextProperties::TextAutofit::NormalAutofit;
                const auto &a = reader.attributes();
                parseAttributePercent(a, QLatin1String("fontScale"), fontScale);
                parseAttributePercent(a, QLatin1String("lnSpcReduction"), lineSpaceReduction);
            }
            else if (reader.name() == QLatin1String("sp3d")) {
                text3D->read(reader);
            }
            else if (reader.name() == QLatin1String("flatTx")) {
                z = Coordinate::create(reader.attributes().value(QLatin1String("z")).toString());
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

Size::Size()
{

}

Size::Size(uint points)
{
    val = QVariant::fromValue<uint>(points);
}

Size::Size(double percents)
{
    val = QVariant::fromValue<double>(percents);
}

int Size::toPoints() const
{
    return val.toUInt();
}

double Size::toPercents() const
{
    return val.toDouble();
}

QString Size::toString() const
{
    if (val.userType() == QMetaType::UInt) return QString::number(val.toUInt());
    if (val.userType() == QMetaType::Double) return QString::number(val.toDouble())+'%';
    return QString();
}

void Size::setPoints(int points)
{
    val = QVariant::fromValue<uint>(points);
}

void Size::setPercents(double percents)
{
    val = QVariant::fromValue<double>(percents);
}

void Size::read(QXmlStreamReader &reader)
{
    //    <xsd:complexType name="CT_TextSpacing">
    //      <xsd:choice>
    //        <xsd:element name="spcPct" type="CT_TextSpacingPercent"/>
    //        <xsd:element name="spcPts" type="CT_TextSpacingPoint"/>
    //      </xsd:choice>
    //    </xsd:complexType>

    //    <xsd:complexType name="CT_TextSpacingPoint">
    //      <xsd:attribute name="val" type="ST_TextSpacingPoint" use="required"/>
    //    </xsd:complexType>
    //    <xsd:simpleType name="ST_TextSpacingPoint">
    //        <xsd:restriction base="xsd:int">
    //          <xsd:minInclusive value="0"/>
    //          <xsd:maxInclusive value="158400"/>
    //        </xsd:restriction>
    //      </xsd:simpleType>
    //    <xsd:simpleType name="ST_TextSpacingPercentOrPercentString">
    //      <xsd:union memberTypes="s:ST_Percentage"/>
    //    </xsd:simpleType>
    //    <xsd:complexType name="CT_TextSpacingPercent">
    //      <xsd:attribute name="val" type="ST_TextSpacingPercentOrPercentString" use="required"/>
    //    </xsd:complexType>
    reader.readNextStartElement();
    if (reader.name() == QLatin1String("spcPct")) {
        std::optional<double> v;
        parseAttributePercent(reader.attributes(), QLatin1String("val"), v);
        if (v.has_value()) val = v.value();
    }
    if (reader.name() == QLatin1String("spcPts")) {
        int v;
        parseAttributeInt(reader.attributes(), QLatin1String("val"), v);
        val = v;
    }
}

void Size::write(QXmlStreamWriter &writer, const QString &name) const
{
    if (val.userType() == QMetaType::Int) {
        writer.writeEmptyElement(QLatin1String("spcPts"));
        writer.writeAttribute(QLatin1String("val"), QString::number(val.toInt()));
    }
    if (val.userType() == QMetaType::Double) {
        writer.writeEmptyElement(QLatin1String("spcPct"));
        writer.writeAttribute(QLatin1String("val"), toST_Percent(val.toDouble()));
    }
}

void ParagraphProperties::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    const auto &a = reader.attributes();

    parseAttribute(a, QLatin1String("marL"), leftMargin);
    parseAttribute(a, QLatin1String("marR"), rightMargin);
    parseAttributeInt(a, QLatin1String("lvl"), indentLevel);
    parseAttribute(a, QLatin1String("indent"), indent);

    if (a.hasAttribute(QLatin1String("algn"))) {
        const auto s = a.value(QLatin1String("algn"));
        if (s == QLatin1String("l")) align = TextAlign::Left;
        if (s == QLatin1String("r")) align = TextAlign::Right;
        if (s == QLatin1String("ctr")) align = TextAlign::Center;
        if (s == QLatin1String("just")) align = TextAlign::Justify;
        if (s == QLatin1String("justLow")) align = TextAlign::JustifyLow;
        if (s == QLatin1String("dist")) align = TextAlign::Distrubute;
        if (s == QLatin1String("thaiDist")) align = TextAlign::DistrubuteThai;
    }
    parseAttribute(a, QLatin1String("defTabSz"), defaultTabSize);
    parseAttributeBool(a, QLatin1String("rtl"), rtl);
    parseAttributeBool(a, QLatin1String("eaLnBrk"), eastAsianLineBreak);

    if (a.hasAttribute(QLatin1String("fontAlgn"))) {
        const auto s = a.value(QLatin1String("fontAlgn"));
        if (s == QLatin1String("t")) fontAlign = FontAlign::Top;
        if (s == QLatin1String("b")) fontAlign = FontAlign::Bottom;
        if (s == QLatin1String("auto")) fontAlign = FontAlign::Auto;
        if (s == QLatin1String("ctr")) fontAlign = FontAlign::Center;
        if (s == QLatin1String("base")) fontAlign = FontAlign::Base;
    }
    parseAttributeBool(a, QLatin1String("latinLnBrk"), latinLineBreak);
    parseAttributeBool(a, QLatin1String("hangingPunct"), hangingPunctuation);

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("lnSpc")) {
                reader.readNextStartElement();
                if (reader.name() == QLatin1String("spcPct")) {
                    std::optional<double> v;
                    parseAttributePercent(reader.attributes(), QLatin1String("val"), v);
                    if (v.has_value()) lineSpacing = Size(v.value());
                }
                if (reader.name() == QLatin1String("spcPts")) {
                    uint v;
                    parseAttributeUInt(reader.attributes(), QLatin1String("val"), v);
                    lineSpacing = Size(v);
                }
            }
            else if (reader.name() == QLatin1String("spcBef")) {
                reader.readNextStartElement();
                if (reader.name() == QLatin1String("spcPct")) {
                    std::optional<double> v;
                    parseAttributePercent(reader.attributes(), QLatin1String("val"), v);
                    if (v.has_value()) spacingBefore = Size(v.value());
                }
                if (reader.name() == QLatin1String("spcPts")) {
                    uint v;
                    parseAttributeUInt(reader.attributes(), QLatin1String("val"), v);
                    spacingBefore = Size(v);
                }
            }
            else if (reader.name() == QLatin1String("spcAft")) {
                reader.readNextStartElement();
                if (reader.name() == QLatin1String("spcPct")) {
                    std::optional<double> v;
                    parseAttributePercent(reader.attributes(), QLatin1String("val"), v);
                    if (v.has_value()) spacingAfter = Size(v.value());
                }
                if (reader.name() == QLatin1String("spcPts")) {
                    uint v;
                    parseAttributeUInt(reader.attributes(), QLatin1String("val"), v);
                    spacingAfter = Size(v);
                }
            }
            else if (reader.name() == QLatin1String("buClrTx")) {
                // bullet color follows text
            }
            else if (reader.name() == QLatin1String("buClr")) {
                reader.readNextStartElement();
                bulletColor = Color();
                bulletColor->read(reader);
            }
            else if (reader.name() == QLatin1String("buSzTx")) {
                // bullet size follows text
            }
            else if (reader.name() == QLatin1String("buSzPct")) {
                std::optional<double> v;
                parseAttributePercent(reader.attributes(), QLatin1String("val"), v);
                if (v.has_value()) bulletSize = Size(v.value());
            }
            else if (reader.name() == QLatin1String("buSzPts")) {
                uint v;
                parseAttributeUInt(reader.attributes(), QLatin1String("val"), v);
                bulletSize = Size(v);
            }
            else if (reader.name() == QLatin1String("buFontTx")) {
                // bullet font follows text
            }
            else if (reader.name() == QLatin1String("buFont")) {
                bulletFont.read(reader);
            }
            else if (reader.name() == QLatin1String("buNone")) {
                bulletType = BulletType::None;
            }
            else if (reader.name() == QLatin1String("buAutoNum")) {
                bulletType = BulletType::AutoNumber;
                const auto &a = reader.attributes();
                const QString &type = a.value(QLatin1String("type")).toString();
                fromString(type, bulletAutonumberType);
                if (a.hasAttribute(QLatin1String("startAt")))
                    bulletAutonumberStart = a.value(QLatin1String("startAt")).toInt();
            }
            else if (reader.name() == QLatin1String("buChar")) {
                bulletType = BulletType::Char;
                bulletChar = reader.attributes().value(QLatin1String("char")).toString();
            }
            else if (reader.name() == QLatin1String("buBlip")) {
                //TODO:
                bulletType = BulletType::Blip;
            }
            else if (reader.name() == QLatin1String("tabLst")) {
                readTabStops(reader);
            }
            else if (reader.name() == QLatin1String("defRPr")) {
                TextCharacterProperties p;
                p.read(reader);
                defaultTextCharacterProperties = p;
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void ParagraphProperties::write(QXmlStreamWriter &writer, const QString &name) const
{

}

bool ListStypeProperties::isEmpty() const
{
    return vals.isEmpty();
}

ParagraphProperties ListStypeProperties::getDefault() const
{
    if (vals.size()>0) return vals[0];
    return {};
}

ParagraphProperties ListStypeProperties::value(int level) const
{
    if (vals.size()>level && level > 0) return vals[level];
    return {};
}

void ListStypeProperties::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("defPPr")) {
                if (vals.size() < 1) vals.resize(1);
                ParagraphProperties p;
                p.read(reader);
                vals[0] = p;
            }
            else if (reader.name() == QLatin1String("lvl1pPr")) {
                if (vals.size() < 2) vals.resize(2);
                ParagraphProperties p;
                p.read(reader);
                vals[1] = p;
            }
            else if (reader.name() == QLatin1String("lvl2pPr")) {
                if (vals.size() < 3) vals.resize(3);
                ParagraphProperties p;
                p.read(reader);
                vals[2] = p;
            }
            else if (reader.name() == QLatin1String("lvl3pPr")) {
                if (vals.size() < 4) vals.resize(4);
                ParagraphProperties p;
                p.read(reader);
                vals[3] = p;
            }
            else if (reader.name() == QLatin1String("lvl4pPr")) {
                if (vals.size() < 5) vals.resize(5);
                ParagraphProperties p;
                p.read(reader);
                vals[4] = p;
            }
            else if (reader.name() == QLatin1String("lvl5pPr")) {
                if (vals.size() < 6) vals.resize(6);
                ParagraphProperties p;
                p.read(reader);
                vals[5] = p;
            }
            else if (reader.name() == QLatin1String("lvl6pPr")) {
                if (vals.size() < 7) vals.resize(7);
                ParagraphProperties p;
                p.read(reader);
                vals[6] = p;
            }
            else if (reader.name() == QLatin1String("lvl7pPr")) {
                if (vals.size() < 8) vals.resize(8);
                ParagraphProperties p;
                p.read(reader);
                vals[7] = p;
            }
            else if (reader.name() == QLatin1String("lvl8pPr")) {
                if (vals.size() < 9) vals.resize(9);
                ParagraphProperties p;
                p.read(reader);
                vals[8] = p;
            }
            else if (reader.name() == QLatin1String("lvl9pPr")) {
                if (vals.size() < 10) vals.resize(10);
                ParagraphProperties p;
                p.read(reader);
                vals[9] = p;
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void ListStypeProperties::write(QXmlStreamWriter &writer, const QString &name) const
{
    if (vals.isEmpty()) {
        writer.writeEmptyElement(name);
        return;
    }

    writer.writeStartElement(name);
    if (vals.size()>0) vals[0].write(writer, QLatin1String("defPPr"));
    if (vals.size()>1) vals[1].write(writer, QLatin1String("lvl1pPr"));
    if (vals.size()>2) vals[2].write(writer, QLatin1String("lvl2pPr"));
    if (vals.size()>3) vals[3].write(writer, QLatin1String("lvl3pPr"));
    if (vals.size()>4) vals[4].write(writer, QLatin1String("lvl4pPr"));
    if (vals.size()>5) vals[5].write(writer, QLatin1String("lvl5pPr"));
    if (vals.size()>6) vals[6].write(writer, QLatin1String("lvl6pPr"));
    if (vals.size()>7) vals[7].write(writer, QLatin1String("lvl7pPr"));
    if (vals.size()>8) vals[8].write(writer, QLatin1String("lvl8pPr"));
    if (vals.size()>9) vals[9].write(writer, QLatin1String("lvl9pPr"));
    writer.writeEndElement();
}

#endif

void ParagraphProperties::readTabStops(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement && reader.name() == QLatin1String("tab")) {
            const auto &a = reader.attributes();
            QPair<Coordinate, TabAlign> p;
            if (a.hasAttribute(QLatin1String("pos"))) p.first = Coordinate::create(a.value(QLatin1String("pos")));
            if (a.hasAttribute(QLatin1String("algn"))) {
                TabAlign t;
                fromString(a.value(QLatin1String("algn")).toString(), t);
                p.second = t;
                tabStops << p;
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void ParagraphProperties::writeTabStops(QXmlStreamWriter &writer, const QString &name) const
{
    if (tabStops.isEmpty()) return;
    writer.writeStartElement(name); //
    for (const auto &p: tabStops) {
        writer.writeEmptyElement(QLatin1String("a:tab"));
        writer.writeAttribute(QLatin1String("pos"), p.first.toString());
        QString s;
        toString(p.second, s);
        writer.writeAttribute(QLatin1String("algn"), s);
    }
    writer.writeEndElement();
}

void TextCharacterProperties::read(QXmlStreamReader &reader)
{
//    <xsd:complexType name="CT_TextCharacterProperties">
//        <xsd:sequence>
//          <xsd:element name="ln" type="CT_LineProperties" minOccurs="0" maxOccurs="1"/>
//          <xsd:group ref="EG_FillProperties" minOccurs="0" maxOccurs="1"/>
//          <xsd:group ref="EG_EffectProperties" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="highlight" type="CT_Color" minOccurs="0" maxOccurs="1"/>
//          <xsd:group ref="EG_TextUnderlineLine" minOccurs="0" maxOccurs="1"/>
//          <xsd:group ref="EG_TextUnderlineFill" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="latin" type="CT_TextFont" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="ea" type="CT_TextFont" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="cs" type="CT_TextFont" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="sym" type="CT_TextFont" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="hlinkClick" type="CT_Hyperlink" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="hlinkMouseOver" type="CT_Hyperlink" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="rtl" type="CT_Boolean" minOccurs="0"/>
//          <xsd:element name="extLst" type="CT_OfficeArtExtensionList" minOccurs="0" maxOccurs="1"/>
//        </xsd:sequence>
//        <xsd:attribute name="kumimoji" type="xsd:boolean" use="optional"/>
//        <xsd:attribute name="lang" type="s:ST_Lang" use="optional"/>
//        <xsd:attribute name="altLang" type="s:ST_Lang" use="optional"/>
//        <xsd:attribute name="sz" type="ST_TextFontSize" use="optional"/>
//        <xsd:attribute name="b" type="xsd:boolean" use="optional"/>
//        <xsd:attribute name="i" type="xsd:boolean" use="optional"/>
//        <xsd:attribute name="u" type="ST_TextUnderlineType" use="optional"/>
//        <xsd:attribute name="strike" type="ST_TextStrikeType" use="optional"/>
//        <xsd:attribute name="kern" type="ST_TextNonNegativePoint" use="optional"/>
//        <xsd:attribute name="cap" type="ST_TextCapsType" use="optional" default="none"/>
//        <xsd:attribute name="spc" type="ST_TextPoint" use="optional"/>
//        <xsd:attribute name="normalizeH" type="xsd:boolean" use="optional"/>
//        <xsd:attribute name="baseline" type="ST_Percentage" use="optional"/>
//        <xsd:attribute name="noProof" type="xsd:boolean" use="optional"/>
//        <xsd:attribute name="dirty" type="xsd:boolean" use="optional" default="true"/>
//        <xsd:attribute name="err" type="xsd:boolean" use="optional" default="false"/>
//        <xsd:attribute name="smtClean" type="xsd:boolean" use="optional" default="true"/>
//        <xsd:attribute name="smtId" type="xsd:unsignedInt" use="optional" default="0"/>
//        <xsd:attribute name="bmk" type="xsd:string" use="optional"/>
//      </xsd:complexType>
    const auto &name = reader.name();

    const auto &a = reader.attributes();
    parseAttributeBool(a, QLatin1String("kumimoji"), kumimoji);
    parseAttributeString(a, QLatin1String("lang"), language);
    parseAttributeString(a, QLatin1String("altLang"), alternateLanguage);
    if (a.hasAttribute(QLatin1String("sz"))) fontSize = a.value(QLatin1String("sz")).toDouble() / 100;
    parseAttributeBool(a, QLatin1String("b"), bold);
    parseAttributeBool(a, QLatin1String("i"), italic);
    if (a.hasAttribute(QLatin1String("u"))) {
        UnderlineType t;
        fromString(a.value(QLatin1String("u")).toString(), t);
        underline = t;
    }
    if (a.hasAttribute(QLatin1String("strike"))) {
        StrikeType t;
        fromString(a.value(QLatin1String("strike")).toString(), t);
        strike = t;
    }
    if (a.hasAttribute(QLatin1String("kern"))) kerningFontSize = a.value(QLatin1String("kern")).toDouble() / 100;
    if (a.hasAttribute(QLatin1String("cap"))) {
        CapitalizationType t;
        fromString(a.value(QLatin1String("cap")).toString(), t);
        capitalization = t;
    }
    if (a.hasAttribute(QLatin1String("spc"))) spacing = TextPoint::create(a.value(QLatin1String("spc")));
    parseAttributeBool(a, QLatin1String("normalizeH"), normalizeHeights);
    parseAttributeBool(a, QLatin1String("noProof"), noProofing);
    parseAttributeBool(a, QLatin1String("dirty"), proofingNeeded);
    parseAttributeBool(a, QLatin1String("err"), spellingErrorFound);
    parseAttributeBool(a, QLatin1String("smtClean"), checkForSmartTagsNeeded);
    parseAttributePercent(a, QLatin1String("baseline"), baseline);
    parseAttributeInt(a, QLatin1String("smtId"), smartTagId);

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("tab")) {

            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void TextCharacterProperties::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    if (kumimoji.has_value()) writer.writeAttribute(QLatin1String("kumimoji"),
                                                    toST_Boolean(kumimoji.value()));
    if (!language.isEmpty()) writer.writeAttribute(QLatin1String("lang"), language);
    if (!alternateLanguage.isEmpty()) writer.writeAttribute(QLatin1String("lang"),
                                                            alternateLanguage);
    if (fontSize.has_value()) writer.writeAttribute(QLatin1String("sz"),
                                                    QString::number(qRound(fontSize.value()*100)));
    if (underline.has_value()) {
        QString s; toString(underline.value(), s);
        writer.writeAttribute(QLatin1String("u"), s);
    }
    if (strike.has_value()) {
        QString s; toString(strike.value(), s);
        writer.writeAttribute(QLatin1String("strike"), s);
    }
    if (kerningFontSize.has_value()) writer.writeAttribute(QLatin1String("kern"),
                                                           QString::number(qRound(kerningFontSize.value()*100)));
    if (capitalization.has_value()) {
        QString s; toString(capitalization.value(), s);
        writer.writeAttribute(QLatin1String("cap"), s);
    }
    if (spacing.has_value()) writer.writeAttribute(QLatin1String("spc"), spacing.value().toString());
    if (normalizeHeights.has_value()) writer.writeAttribute(QLatin1String("normalizeH"),
                                                    toST_Boolean(normalizeHeights.value()));
    if (baseline.has_value()) writer.writeAttribute(QLatin1String("baseline"),
                                                    toST_Percent(baseline.value()));
    if (noProofing.has_value()) writer.writeAttribute(QLatin1String("noProof"),
                                                    toST_Boolean(noProofing.value()));
    if (proofingNeeded.has_value()) writer.writeAttribute(QLatin1String("dirty"),
                                                    toST_Boolean(proofingNeeded.value()));
    if (spellingErrorFound.has_value()) writer.writeAttribute(QLatin1String("err"),
                                                    toST_Boolean(spellingErrorFound.value()));
    if (checkForSmartTagsNeeded.has_value()) writer.writeAttribute(QLatin1String("smtClean"),
                                                    toST_Boolean(checkForSmartTagsNeeded.value()));
    if (smartTagId.has_value()) writer.writeAttribute(QLatin1String("smtId"),
                                                    QString::number(smartTagId.value()));
    writer.writeEndElement();
}


QT_END_NAMESPACE_XLSX
