#include "xlsxfillproperties.h"
#include "xlsxfillproperties_p.h"
#include "xlsxutility_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE_XLSX

FillProperties::FillProperties(FillProperties::FillType type)
{
    d = new FillPropertiesPrivate;
    d->type = type;
}

FillProperties::FillProperties(const FillProperties &other) : d(other.d)
{

}

FillProperties &FillProperties::operator=(const FillProperties &rhs)
{
    d = rhs.d;
    return *this;
}

FillProperties::FillType FillProperties::type() const
{
    if (d) return d->type;
    return FillType::NoFill;
}

void FillProperties::setType(FillProperties::FillType type)
{
    if (!d) d = new FillPropertiesPrivate;
    d->type = type;
}

std::optional<Color> FillProperties::color() const
{
    if (d) return d->color;
    return {};
}

void FillProperties::setColor(const Color &color)
{
    if (!d) d = new FillPropertiesPrivate;
    d->color = color;
}

void FillProperties::setColor(const QColor &color)
{
    if (!d) d = new FillPropertiesPrivate;
    d->color = Color(Color::ColorType::RGBColor, color);
}

QMap<double, Color> FillProperties::gradientList() const
{
    if (d) return d->gradientList;
    return {};
}

void FillProperties::setGradientList(const QMap<double, Color> &list)
{
    if (!d) d = new FillPropertiesPrivate;
    d->gradientList = list;
}

std::optional<QPair<Angle, bool> > FillProperties::linearShade() const
{
    if (d->linearShadeAngle.has_value() || d->linearShadeScaled.has_value())
        return std::make_optional<QPair<Angle, bool>>(d->linearShadeAngle.value(),
                                                       d->linearShadeScaled.value());
    return {};
}

void FillProperties::setLinearShade(QPair<Angle, bool> val)
{
    if (!d) d = new FillPropertiesPrivate;
    d->linearShadeAngle = val.first;
    d->linearShadeScaled = val.second;
}

bool FillProperties::isValid() const
{
    if (d) return true;
    return false;
}

void FillProperties::write(QXmlStreamWriter &writer) const
{
    switch (d->type) {
        case FillType::NoFill : writeNoFill(writer); break;
        case FillType::SolidFill : writeSolidFill(writer); break;

        case FillType::GradientFill : writeGradientFill(writer); break;
        //TODO: all other fill types
//        case FillType::BlipFill : writeBlipFill(writer); break;
//        case FillType::PatternFill : writePatternFill(writer); break;
//        case FillType::GroupFill : writeGroupFill(writer); break;
    }
}

void FillProperties::read(QXmlStreamReader &reader)
{
    const QString &name = reader.name().toString();
    FillType t;
    fromString(name, t);

    if (t == FillType::NoFill) return;
    if (!d) {
        d = new FillPropertiesPrivate;
        d->type = t;
    }

    switch (d->type) {
        case FillType::SolidFill : readSolidFill(reader); break;

        case FillType::GradientFill : readGradientFill(reader); break;
        //TODO: all other fill types
//        case FillType::BlipFill : readBlipFill(reader); break;
//        case FillType::PatternFill : readPatternFill(reader); break;
//        case FillType::GroupFill : readGroupFill(reader); break;
        default: break;
    }
}

void FillProperties::readNoFill(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("noFill"));

    //NO_OP
}

void FillProperties::readSolidFill(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("solidFill"));

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement)
            d->color->read(reader);
        else if (token == QXmlStreamReader::EndElement && reader.name() == QLatin1String("solidFill"))
            break;
    }
}

void FillProperties::readGradientFill(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("gradFill"));

    const auto &attr = reader.attributes();
    if (attr.hasAttribute(QLatin1String("flip"))) {
        TileFlipMode t;
        fromString(attr.value(QLatin1String("flip")).toString(), t);
        d->tileFlipMode = t;
    }
    parseAttributeBool(attr, QLatin1String("rotWithShape"), d->rotWithShape);

//    <xsd:complexType name="CT_GradientFillProperties">
//        <xsd:sequence>
//          <xsd:element name="gsLst" type="CT_GradientStopList" minOccurs="0" maxOccurs="1"/>
//          <xsd:group ref="EG_ShadeProperties" minOccurs="0" maxOccurs="1"/>
//          <xsd:element name="tileRect" type="CT_RelativeRect" minOccurs="0" maxOccurs="1"/>
//        </xsd:sequence>
//        <xsd:attribute name="flip" type="ST_TileFlipMode" use="optional" default="none"/>
//        <xsd:attribute name="rotWithShape" type="xsd:boolean" use="optional"/>
//      </xsd:complexType>

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("gsLst")) {
                readGradientList(reader);
            }
            else if (reader.name() == QLatin1String("lin")) {
                //linear shade properties
                const auto &attr = reader.attributes();
                parseAttribute(attr, QLatin1String("ang"), d->linearShadeAngle);
                parseAttributeBool(attr, QLatin1String("scaled"), d->linearShadeScaled);
            }
            else if (reader.name() == QLatin1String("path")) {
                //path shade properties
                const auto &attr = reader.attributes();
                if (attr.hasAttribute(QLatin1String("path"))) {
                    QStringRef s = attr.value(QLatin1String("path"));
                    if (s == QLatin1String("shape")) d->pathShadeType = PathShadeType::Shape;
                    else if (s == QLatin1String("circle")) d->pathShadeType = PathShadeType::Circle;
                    else if (s == QLatin1String("rect")) d->pathShadeType = PathShadeType::Rectangle;
                }

                reader.readNextStartElement();
                if (reader.name() == QLatin1String("fillToRect")) {
                    const auto &attr = reader.attributes();
                    QRectF r;
                    r.setTop(fromST_Percent(attr.value("t")));
                    r.setLeft(fromST_Percent(attr.value("l")));
                    r.setRight(fromST_Percent(attr.value("r")));
                    r.setBottom(fromST_Percent(attr.value("b")));
                    d->pathShadeRect = r;
                }
            }
            else if (reader.name() == QLatin1String("tileRect")) {
                const auto &attr = reader.attributes();
                QRectF r;
                r.setTop(fromST_Percent(attr.value("t")));
                r.setLeft(fromST_Percent(attr.value("l")));
                r.setRight(fromST_Percent(attr.value("r")));
                r.setBottom(fromST_Percent(attr.value("b")));
                d->tileRect = r;
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == QLatin1String("gradFill"))
            break;
    }
}

void FillProperties::writeNoFill(QXmlStreamWriter &writer) const
{
    writer.writeEmptyElement("a:noFill");
}

void FillProperties::writeSolidFill(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("a:solidFill");
    d->color->write(writer);
    writer.writeEndElement();
}

void FillProperties::writeGradientFill(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QLatin1String("a:gradFill"));
    if (d->tileFlipMode.has_value()) {
        QString s;
        toString(d->tileFlipMode.value(), s);
        writer.writeAttribute("flip", s);
    }
    if (d->rotWithShape.has_value()) writer.writeAttribute("rotWithShape", d->rotWithShape.value() ? "true" : "false");
    writeGradientList(writer);

    if (d->linearShadeAngle.has_value() || d->linearShadeScaled.has_value()) {
        writer.writeEmptyElement("a:lin");
        if (d->linearShadeAngle.has_value())
            writer.writeAttribute("ang", d->linearShadeAngle->toString());
        if (d->linearShadeScaled.has_value())
            writer.writeAttribute("scaled", d->linearShadeScaled.value() ? "true" : "false");
    }

    if (d->pathShadeType.has_value()) {
        writer.writeStartElement("a:path");
        switch (d->pathShadeType.value()) {
            case PathShadeType::Shape: writer.writeAttribute("path", "shape"); break;
            case PathShadeType::Circle: writer.writeAttribute("path", "circle"); break;
            case PathShadeType::Rectangle: writer.writeAttribute("path", "rect"); break;
        }
        if (d->pathShadeRect.has_value()) {
            writer.writeEmptyElement("a:fillToRect");
            writer.writeAttribute("t", toST_Percent(d->pathShadeRect->top()));
            writer.writeAttribute("b", toST_Percent(d->pathShadeRect->bottom()));
            writer.writeAttribute("l", toST_Percent(d->pathShadeRect->left()));
            writer.writeAttribute("r", toST_Percent(d->pathShadeRect->right()));
        }

        writer.writeEndElement();
    }
    if (d->tileRect.has_value()) {
        writer.writeEmptyElement("a:tileRect");
        writer.writeAttribute("t", toST_Percent(d->tileRect->top()));
        writer.writeAttribute("b", toST_Percent(d->tileRect->bottom()));
        writer.writeAttribute("l", toST_Percent(d->tileRect->left()));
        writer.writeAttribute("r", toST_Percent(d->tileRect->right()));
    }

    writer.writeEndElement();
}

void FillProperties::readGradientList(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("gs")) {
                double pos = fromST_Percent(reader.attributes().value("pos"));
                reader.readNextStartElement();
                Color col;
                col.read(reader);
                d->gradientList.insert(pos, col);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == QLatin1String("gsLst"))
            break;
    }
}

void FillProperties::writeGradientList(QXmlStreamWriter &writer) const
{
    if (d->gradientList.isEmpty()) return;
    writer.writeStartElement(QLatin1String("gsLst"));
    for (auto i = d->gradientList.constBegin(); i!= d->gradientList.constEnd(); ++i) {
        writer.writeStartElement(QLatin1String("gs"));
        writer.writeAttribute("pos", QString::number(i.key()*100)+'%');
        i.value().write(writer);
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

FillPropertiesPrivate::FillPropertiesPrivate()
{

}

FillPropertiesPrivate::FillPropertiesPrivate(const FillPropertiesPrivate &other)
    : QSharedData(other),
      type(other.type),
      color(other.color),
      gradientList(other.gradientList),
      linearShadeAngle(other.linearShadeAngle),
      linearShadeScaled(other.linearShadeScaled),
      pathShadeType(other.pathShadeType),
      pathShadeRect(other.pathShadeRect),
      tileRect(other.tileRect),
      tileFlipMode(other.tileFlipMode),
      rotWithShape(other.rotWithShape)
{

}

FillPropertiesPrivate::~FillPropertiesPrivate()
{

}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const FillProperties &f)
{
    QDebugStateSaver saver(dbg);

    dbg.nospace() << "QXlsx::FillProperties(" ;
    dbg.nospace() << "type=" << f.d->type << ", ";
    if (f.d->color.has_value())
        dbg.nospace() << "color=" << f.d->color.value() << ", ";
    else dbg.nospace() << "color=not set, ";

    return dbg;

    //Gradient fill properties
//    QMap<double, XlsxColor> gradientList;
//    std::optional<double> linearShadeAngle; //0..360
//    std::optional<bool> linearShadeScaled;
//    std::optional<FillProperties::PathShadeType> pathShadeType;
//    std::optional<QRectF> pathShadeRect;
//    std::optional<QRectF> tileRect;
//    std::optional<FillProperties::TileFlipMode> tileFlipMode;
//    std::optional<bool> rotWithShape;
}
#endif

QT_END_NAMESPACE_XLSX
