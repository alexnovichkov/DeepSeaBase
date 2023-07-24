// xlsxcolor.cpp

#include <QDataStream>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

#include "xlsxcolor.h"
#include "xlsxstyles_p.h"
#include "xlsxutility_p.h"
#include "xlsxmain.h"

QT_BEGIN_NAMESPACE_XLSX

Color::Color() : type(ColorType::Invalid)
{

}

Color::Color(Color::ColorType type) : type(type)
{

}

Color::Color(Color::ColorType type, QColor color) : type(type)
{
    setRgb(color);
}

bool Color::isInvalid() const
{
    return type == ColorType::Invalid || !val.isValid();
}

void Color::setRgb(const QColor &color)
{
    if (type == ColorType::RGBColor || type == ColorType::CRGBColor || type == ColorType::SimpleColor) {
        val = color;
    }
}

void Color::setIndexedColor(int index)
{
    if (type == ColorType::SimpleColor) val = index;
}

void Color::setAutoColor(bool autoColor)
{
    if (type == ColorType::SimpleColor) val = autoColor;
}

void Color::setThemeColor(uint theme, double tint)
{
    if (type == ColorType::SimpleColor) {
        QVariantMap map;
        map.insert("theme", theme);
        map.insert("tint", tint);
        val = map;
    }
}

void Color::setPresetColor(const QString &colorName)
{
    if (type == ColorType::PresetColor && !colorName.isEmpty())
        val = colorName;
}

void Color::setSchemeColor(Color::SchemeColor color)
{
    if (type == ColorType::SchemeColor)
        val = static_cast<int>(color);
}

void Color::setSystemColor(Color::SystemColor color)
{
    if (type == ColorType::SystemColor)
        val = static_cast<int>(color);
}

void Color::addTransform(ColorTransform::Type transform, QVariant val)
{
    tr.vals.insert(transform, val);
}

QColor Color::rgb() const
{
    return (type == ColorType::RGBColor || type == ColorType::CRGBColor || type == ColorType::SimpleColor)
            ? val.value<QColor>() : QColor();
}

QColor Color::presetColor() const
{
    return (type == ColorType::PresetColor) ? QColor(val.toString()) : QColor();
}

QString Color::schemeColor() const
{
    if (type != ColorType::SchemeColor) return QString();

    SchemeColor c = static_cast<SchemeColor>(val.toInt());
    QString s;
    toString(c, s);

    return s;
}

QString Color::systemColor() const
{
    if (type != ColorType::SystemColor) return QString();

    SystemColor c = static_cast<SystemColor>(val.toInt());
    QString s;
    toString(c, s);

    return s;
}

int Color::indexedColor() const
{
    return (type == ColorType::SimpleColor && val.type() == QVariant::Int) ? val.toInt() : -1;
}

bool Color::isAutoColor() const
{
    return (type == ColorType::SimpleColor && val.type() == QVariant::Bool) ? val.toBool() : false;
}

QPair<int, double> Color::themeColor() const
{
    if (type != ColorType::SimpleColor || val.type() != QVariant::Map) return {-1, 0.0};

    auto m = val.toMap();
    return {m.value("theme").toUInt(), m.value("tint").toDouble()};
}

bool Color::write(QXmlStreamWriter &writer, const QString &node) const
{
    switch (type) {
        case ColorType::SimpleColor: {
            if (!node.isEmpty())
                writer.writeEmptyElement(node); //color, bgColor, fgColor
            else
                writer.writeEmptyElement(QStringLiteral("color"));

            if (val.userType() == qMetaTypeId<QColor>()) {
                writer.writeAttribute(QStringLiteral("rgb"), Color::toARGBString(val.value<QColor>()));
            } else if (val.userType() == QMetaType::QVariantMap) {
                auto themes = val.toMap();
                writer.writeAttribute(QStringLiteral("theme"), QString::number(themes.value(QStringLiteral("theme")).toUInt()));
                if (themes.value(QStringLiteral("tint")).toDouble() != 0.0)
                    writer.writeAttribute(QStringLiteral("tint"), QString::number(themes.value(QStringLiteral("tint")).toDouble()));
            } else if (val.userType() == QMetaType::Int) {
                writer.writeAttribute(QStringLiteral("indexed"), val.toString());
            } else {
                writer.writeAttribute(QStringLiteral("auto"), val.toBool() ? "true" : "false");
            }
            break;
        }
        case ColorType::CRGBColor: {
            writer.writeStartElement(QLatin1String("a:scrgbClr"));
            QColor col = val.value<QColor>();
            writer.writeAttribute(QLatin1String("r"), QString::number(col.redF()*100)+"%");
            writer.writeAttribute(QLatin1String("g"), QString::number(col.greenF()*100)+"%");
            writer.writeAttribute(QLatin1String("b"), QString::number(col.blueF()*100)+"%");

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        case ColorType::RGBColor: {
            writer.writeStartElement(QLatin1String("a:srgbClr"));
            writer.writeAttribute(QLatin1String("val"), toRGBString(val.value<QColor>()));

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        case ColorType::HSLColor: {
            writer.writeStartElement(QLatin1String("a:hslClr"));
            QColor col = val.value<QColor>();
            writer.writeAttribute(QLatin1String("hue"), QString::number(col.hueF()*21600000));
            writer.writeAttribute(QLatin1String("sat"), QString::number(col.saturationF()*100)+"%");
            writer.writeAttribute(QLatin1String("lum"), QString::number(col.lightnessF()*100)+"%");

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        case ColorType::PresetColor: {
            writer.writeStartElement(QLatin1String("a:prstClr"));
            writer.writeAttribute(QLatin1String("val"), val.toString());

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        case ColorType::SchemeColor: {
            writer.writeStartElement(QLatin1String("a:schemeClr"));
            writer.writeAttribute(QLatin1String("val"), schemeColor());

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        case ColorType::SystemColor: {
            writer.writeStartElement(QLatin1String("a:sysClr"));
            writer.writeAttribute(QLatin1String("val"), systemColor());
            //TODO: add writing of lastColor

            tr.write(writer);
            writer.writeEndElement();
            break;
        }
        default: break;
    }

    return true;
}

bool Color::read(QXmlStreamReader &reader)
{
    const auto& attributes = reader.attributes();
    const auto name = reader.name();
    if (type == ColorType::Invalid) {
        if (name == "scrgbClr") type = ColorType::CRGBColor;
        else if (name == "srgbClr") type = ColorType::RGBColor;
        else if (name == "sysClr") type = ColorType::SystemColor;
        else if (name == "hslClr") type = ColorType::HSLColor;
        else if (name == "prstClr") type = ColorType::PresetColor;
        else if (name == "schemeClr") type = ColorType::SchemeColor;
        else type = ColorType::SimpleColor;
    }
    //SimpleColor
    switch (type) {
        case ColorType::SimpleColor: {
            if (attributes.hasAttribute(QLatin1String("rgb"))) {
                const auto& colorString = attributes.value(QLatin1String("rgb")).toString();
                val.setValue(fromARGBString(colorString));
            } else if (attributes.hasAttribute(QLatin1String("indexed"))) {
                int index = attributes.value(QLatin1String("indexed")).toInt();
                val.setValue(index);
            } else if (attributes.hasAttribute(QLatin1String("theme"))) {
                const auto& theme = attributes.value(QLatin1String("theme")).toString();
                const auto& tint = attributes.value(QLatin1String("tint")).toString();
                setThemeColor(theme.toUInt(), tint.toDouble());
            } else if (attributes.hasAttribute(QLatin1String("auto"))) {
                val.setValue(fromST_Boolean(attributes.value(QLatin1String("auto"))));
            }
            break;
        }
        case ColorType::CRGBColor: {
            auto r = attributes.value(QLatin1String("r")).toString(); if (r.endsWith('%')) r.chop(1);
            auto g = attributes.value(QLatin1String("g")).toString(); if (g.endsWith('%')) g.chop(1);
            auto b = attributes.value(QLatin1String("b")).toString(); if (b.endsWith('%')) b.chop(1);
            QColor color = QColor::fromRgbF(r.toDouble() / 100.0,
                                            g.toDouble() / 100.0,
                                            b.toDouble() / 100.0);
            tr.read(reader);
            setRgb(color);
            break;
        }
        case ColorType::RGBColor: {
            const auto& colorString = attributes.value(QLatin1String("rgb")).toString();
            val.setValue(fromARGBString(colorString));
            tr.read(reader);
            break;
        }
        case ColorType::HSLColor: {
            auto h = attributes.value(QLatin1String("hue")).toString();
            auto s = attributes.value(QLatin1String("sat")).toString(); if (s.endsWith('%')) s.chop(1);
            auto l = attributes.value(QLatin1String("lum")).toString(); if (l.endsWith('%')) l.chop(1);
            QColor color = QColor::fromHslF(h.toInt() / 21600000,
                                            s.toDouble() / 100.0,
                                            l.toDouble() / 100.0);
            setRgb(color);
            tr.read(reader);
            break;
        }
        case ColorType::PresetColor: {
            const auto& colorString = attributes.value(QLatin1String("val")).toString();
            val.setValue(colorString);
            tr.read(reader);
            break;
        }
        case ColorType::SchemeColor: {
            const auto& colorString = attributes.value(QLatin1String("val")).toString();
            Color::SchemeColor col;
            fromString(colorString, col);
            setSchemeColor(col);
            tr.read(reader);
            break;
        }
        case ColorType::SystemColor: {
            const auto& colorString = attributes.value(QLatin1String("val")).toString();
            Color::SystemColor col;
            fromString(colorString, col);
            setSystemColor(col);
            tr.read(reader);

            //TODO: integrate lastColor to val
//            QColor lastColor;
//            if (attributes.hasAttribute(QLatin1String("lastClr")))
//                lastColor = fromARGBString(attributes.value("lastClr").toString());
            break;
        }
        default: break;
    }
    return true;
}

Color::operator QVariant() const
{
    const auto& cref
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
        = QMetaType::fromType<XlsxColor>();
#else
        = qMetaTypeId<Color>() ;
#endif
    return QVariant(cref, this);
}


QColor Color::fromARGBString(const QString &c)
{
    QColor color;
    if (c.startsWith(u'#')) {
        color.setNamedColor(c);
    } else {
        color.setNamedColor(QLatin1Char('#') + c);
    }
    return color;
}

QString Color::toARGBString(const QColor &c)
{
    return QString::asprintf("%02X%02X%02X%02X", c.alpha(), c.red(), c.green(), c.blue());
}

QString Color::toRGBString(const QColor &c)
{
    return QString::asprintf("%02X%02X%02X", c.red(), c.green(), c.blue());
}

#if !defined(QT_NO_DATASTREAM)
//QDataStream &operator<<(QDataStream &s, const XlsxColor &color)
//{
//    switch (color.type) {
//        case XlsxColor::ColorType::Invalid: s << 0; break;
//        case XlsxColor::ColorType::SimpleColor: s << 1 << color.rgb()
//    }

//    if (color.isInvalid())
//        s<<0;
//    else if (color.isRgbColor())
//        s<<1<<color.rgbColor();
//    else if (color.isIndexedColor())
//        s<<2<<color.indexedColor();
//    else if (color.isThemeColor())
//        s<<3<<color.themeColor();
//    else
//        s<<4;

//    return s;
//}

//QDataStream &operator>>(QDataStream &s, XlsxColor &color)
//{
//    int marker(4);
//    s>>marker;
//    if (marker == 0) {
//        color = XlsxColor();
//    } else if (marker == 1) {
//        QColor c;
//        s>>c;
//        color = XlsxColor(c);
//    } else if (marker == 2) {
//        int indexed;
//        s>>indexed;
//        color = XlsxColor(indexed);
//    } else if (marker == 3) {
//        QStringList list;
//        s>>list;
//        color = XlsxColor(list[0], list[1]);
//    }

//    return s;
//}

#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Color &c)
{
//    if (c.isInvalid())
//        dbg.nospace() << "XlsxColor(invalid)";
//    else if (c.isRgbColor())
//        dbg.nospace() << c.rgbColor();
//    else if (c.isIndexedColor())
//        dbg.nospace() << "XlsxColor(indexed," << c.indexedColor() << ")";
//    else if (c.isThemeColor())
//        dbg.nospace() << "XlsxColor(theme," << c.themeColor().join(QLatin1Char(':')) << ')';

    return dbg.space();
}

void ColorTransform::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            auto val = reader.attributes().value(QLatin1String("val"));
            if (reader.name() == QLatin1String("alpha"))
                vals.insert(Type::Alpha, fromST_Percent(val));
            if (reader.name() == QLatin1String("alphaOff"))
                vals.insert(Type::AlphaOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("alphaMod"))
                vals.insert(Type::AlphaModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("blue"))
                vals.insert(Type::Blue, fromST_Percent(val));
            if (reader.name() == QLatin1String("blueOff"))
                vals.insert(Type::BlueOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("blueMod"))
                vals.insert(Type::BlueModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("comp"))
                vals.insert(Type::Complement, true);
            if (reader.name() == QLatin1String("gamma"))
                vals.insert(Type::Gamma, true);
            if (reader.name() == QLatin1String("gray"))
                vals.insert(Type::Grayscale, true);
            if (reader.name() == QLatin1String("green"))
                vals.insert(Type::Green, fromST_Percent(val));
            if (reader.name() == QLatin1String("greenOff"))
                vals.insert(Type::GreenOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("greenMod"))
                vals.insert(Type::GreenModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("hue"))
                vals.insert(Type::Hue, fromST_Percent(val));
            if (reader.name() == QLatin1String("hueMod"))
                vals.insert(Type::HueModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("hueOff"))
                vals.insert(Type::HueOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("inv"))
                vals.insert(Type::Inverse, true);
            if (reader.name() == QLatin1String("invGamma"))
                vals.insert(Type::InverseGamma, true);
            if (reader.name() == QLatin1String("lum"))
                vals.insert(Type::Luminescence, fromST_Percent(val));
            if (reader.name() == QLatin1String("lumOff"))
                vals.insert(Type::LuminescenceOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("lumMod"))
                vals.insert(Type::LuminescenceModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("red"))
                vals.insert(Type::Red, fromST_Percent(val));
            if (reader.name() == QLatin1String("redOff"))
                vals.insert(Type::RedOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("redMod"))
                vals.insert(Type::RedModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("sat"))
                vals.insert(Type::Saturation, fromST_Percent(val));
            if (reader.name() == QLatin1String("satOff"))
                vals.insert(Type::SaturationOff, fromST_Percent(val));
            if (reader.name() == QLatin1String("satMod"))
                vals.insert(Type::SaturationModulation, fromST_Percent(val));
            if (reader.name() == QLatin1String("shade"))
                vals.insert(Type::Shade, fromST_Percent(val));
            if (reader.name() == QLatin1String("tint"))
                vals.insert(Type::Tint, fromST_Percent(val));
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void ColorTransform::write(QXmlStreamWriter &writer) const
{
    for (auto i = vals.constBegin(); i!= vals.constEnd(); ++i) {
        switch (i.key()) {
            case Type::Tint:
                writer.writeEmptyElement("a:tint");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Shade:
                writer.writeEmptyElement("a:shade");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Complement:
                writer.writeEmptyElement("a:comp");
                break;
            case Type::Inverse:
                writer.writeEmptyElement("a:inv");
                break;
            case Type::Grayscale:
                writer.writeEmptyElement("a:gray");
                break;
            case Type::Alpha:
                writer.writeEmptyElement("a:alpha");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::AlphaOff:
                writer.writeEmptyElement("a:alphaOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::AlphaModulation:
                writer.writeEmptyElement("a:alphaMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Hue:
                writer.writeEmptyElement("a:hue");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::HueOff:
                writer.writeEmptyElement("a:hueOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::HueModulation:
                writer.writeEmptyElement("a:hueMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Saturation:
                writer.writeEmptyElement("a:sat");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::SaturationOff:
                writer.writeEmptyElement("a:satOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::SaturationModulation:
                writer.writeEmptyElement("a:satMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Luminescence:
                writer.writeEmptyElement("a:lum");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::LuminescenceOff:
                writer.writeEmptyElement("a:lumOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::LuminescenceModulation:
                writer.writeEmptyElement("a:lumMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Red:
                writer.writeEmptyElement("a:red");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::RedOff:
                writer.writeEmptyElement("a:redOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::RedModulation:
                writer.writeEmptyElement("a:redMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Green:
                writer.writeEmptyElement("a:green");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::GreenOff:
                writer.writeEmptyElement("a:greenOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::GreenModulation:
                writer.writeEmptyElement("a:greenMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Blue:
                writer.writeEmptyElement("a:blue");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::BlueOff:
                writer.writeEmptyElement("a:blueOff");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::BlueModulation:
                writer.writeEmptyElement("a:blueMod");
                writer.writeAttribute("val", QString::number(i.value().toDouble() * 100.0)+"%");
                break;
            case Type::Gamma:
                writer.writeEmptyElement("a:gamma");
                break;
            case Type::InverseGamma:
                writer.writeEmptyElement("a:invGamma");
                break;
        }
    }
}

#endif


QT_END_NAMESPACE_XLSX
