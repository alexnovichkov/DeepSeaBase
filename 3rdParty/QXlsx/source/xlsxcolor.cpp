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
    switch (c) {
        case SchemeColor::Background1: return "bg1";
        case SchemeColor::Text1: return "tx1";
        case SchemeColor::Background2: return "bg2";
        case SchemeColor::Text2: return "bg1";
        case SchemeColor::Accent1: return "accent1";
        case SchemeColor::Accent2: return "accent2";
        case SchemeColor::Accent3: return "accent3";
        case SchemeColor::Accent4: return "accent4";
        case SchemeColor::Accent5: return "accent5";
        case SchemeColor::Accent6: return "accent6";
        case SchemeColor::Hlink: return "hlink";
        case SchemeColor::FollowedHlink: return "folHlink";
        case SchemeColor::Style: return "phClr";
        case SchemeColor::Dark1: return "dk1";
        case SchemeColor::Light1: return "lt1";
        case SchemeColor::Dark2: return "dk2";
        case SchemeColor::Light2: return "lt2";
    }

    return QString();
}

QString Color::systemColor() const
{
    if (type != ColorType::SystemColor) return QString();

    SystemColor c = static_cast<SystemColor>(val.toInt());
    switch (c) {
        case SystemColor::scrollBar: return QLatin1String("scrollBar");
        case SystemColor::background: return QLatin1String("background");
        case SystemColor::activeCaption: return QLatin1String("activeCaption");
        case SystemColor::inactiveCaption: return QLatin1String("inactiveCaption");
        case SystemColor::menu: return QLatin1String("menu");
        case SystemColor::window: return QLatin1String("window");
        case SystemColor::windowFrame: return QLatin1String("windowFrame");
        case SystemColor::menuText: return QLatin1String("menuText");
        case SystemColor::windowText: return QLatin1String("windowText");
        case SystemColor::captionText: return QLatin1String("captionText");
        case SystemColor::activeBorder: return QLatin1String("activeBorder");
        case SystemColor::inactiveBorder: return QLatin1String("inactiveBorder");
        case SystemColor::appWorkspace: return QLatin1String("appWorkspace");
        case SystemColor::highlight: return QLatin1String("highlight");
        case SystemColor::highlightText: return QLatin1String("highlightText");
        case SystemColor::btnFace: return QLatin1String("btnFace");
        case SystemColor::btnShadow: return QLatin1String("btnShadow");
        case SystemColor::grayText: return QLatin1String("grayText");
        case SystemColor::btnText: return QLatin1String("btnText");
        case SystemColor::inactiveCaptionText: return QLatin1String("inactiveCaptionText");
        case SystemColor::btnHighlight: return QLatin1String("btnHighlight");
        case SystemColor::DkShadow3d: return QLatin1String("3dDkShadow");
        case SystemColor::Light3d: return QLatin1String("3dLight");
        case SystemColor::infoText: return QLatin1String("infoText");
        case SystemColor::infoBk: return QLatin1String("infoBk");
        case SystemColor::hotLight: return QLatin1String("hotLight");
        case SystemColor::gradientActiveCaption: return QLatin1String("gradientActiveCaption");
        case SystemColor::gradientInactiveCaption: return QLatin1String("gradientInactiveCaption");
        case SystemColor::menuHighlight: return QLatin1String("menuHighlight");
        case SystemColor::menuBar: return QLatin1String("menuBar");
    }

    return QString();
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
            const auto& colorString = attributes.value(QLatin1String("val"));
            if (colorString == QLatin1String("bg1")) setSchemeColor(SchemeColor::Background1);
            else if (colorString == QLatin1String("tx1")) setSchemeColor(SchemeColor::Text1);
            else if (colorString == QLatin1String("bg2")) setSchemeColor(SchemeColor::Background2);
            else if (colorString == QLatin1String("bg1")) setSchemeColor(SchemeColor::Text2);
            else if (colorString == QLatin1String("accent1")) setSchemeColor(SchemeColor::Accent1);
            else if (colorString == QLatin1String("accent2")) setSchemeColor(SchemeColor::Accent2);
            else if (colorString == QLatin1String("accent3")) setSchemeColor(SchemeColor::Accent3);
            else if (colorString == QLatin1String("accent4")) setSchemeColor(SchemeColor::Accent4);
            else if (colorString == QLatin1String("accent5")) setSchemeColor(SchemeColor::Accent5);
            else if (colorString == QLatin1String("accent6")) setSchemeColor(SchemeColor::Accent6);
            else if (colorString == QLatin1String("hlink")) setSchemeColor(SchemeColor::Hlink);
            else if (colorString == QLatin1String("folHlink")) setSchemeColor(SchemeColor::FollowedHlink);
            else if (colorString == QLatin1String("phClr")) setSchemeColor(SchemeColor::Style);
            else if (colorString == QLatin1String("dk1")) setSchemeColor(SchemeColor::Dark1);
            else if (colorString == QLatin1String("lt1")) setSchemeColor(SchemeColor::Light1);
            else if (colorString == QLatin1String("dk2")) setSchemeColor(SchemeColor::Dark2);
            else if (colorString == QLatin1String("lt2")) setSchemeColor(SchemeColor::Light2);
            tr.read(reader);
            break;
        }
        case ColorType::SystemColor: {
            const auto& colorString = attributes.value(QLatin1String("val"));
            if (colorString == QLatin1String("scrollBar")) setSystemColor(SystemColor::scrollBar);
            if (colorString == QLatin1String("background")) setSystemColor( SystemColor::background);
            if (colorString == QLatin1String("activeCaption")) setSystemColor( SystemColor::activeCaption);
            if (colorString == QLatin1String("inactiveCaption")) setSystemColor( SystemColor::inactiveCaption);
            if (colorString == QLatin1String("menu")) setSystemColor( SystemColor::menu);
            if (colorString == QLatin1String("window")) setSystemColor( SystemColor::window);
            if (colorString == QLatin1String("windowFrame")) setSystemColor( SystemColor::windowFrame);
            if (colorString == QLatin1String("menuText")) setSystemColor( SystemColor::menuText);
            if (colorString == QLatin1String("windowText")) setSystemColor( SystemColor::windowText);
            if (colorString == QLatin1String("captionText")) setSystemColor( SystemColor::captionText);
            if (colorString == QLatin1String("activeBorder")) setSystemColor( SystemColor::activeBorder);
            if (colorString == QLatin1String("inactiveBorder")) setSystemColor( SystemColor::inactiveBorder);
            if (colorString == QLatin1String("appWorkspace")) setSystemColor( SystemColor::appWorkspace);
            if (colorString == QLatin1String("highlight")) setSystemColor( SystemColor::highlight);
            if (colorString == QLatin1String("highlightText")) setSystemColor( SystemColor::highlightText);
            if (colorString == QLatin1String("btnFace")) setSystemColor( SystemColor::btnFace);
            if (colorString == QLatin1String("btnShadow")) setSystemColor( SystemColor::btnShadow);
            if (colorString == QLatin1String("grayText")) setSystemColor( SystemColor::grayText);
            if (colorString == QLatin1String("btnText")) setSystemColor( SystemColor::btnText);
            if (colorString == QLatin1String("inactiveCaptionText")) setSystemColor( SystemColor::inactiveCaptionText);
            if (colorString == QLatin1String("btnHighlight")) setSystemColor( SystemColor::btnHighlight);
            if (colorString == QLatin1String("3dDkShadow")) setSystemColor( SystemColor::DkShadow3d);
            if (colorString == QLatin1String("3dLight")) setSystemColor( SystemColor::Light3d);
            if (colorString == QLatin1String("infoText")) setSystemColor( SystemColor::infoText);
            if (colorString == QLatin1String("infoBk")) setSystemColor( SystemColor::infoBk);
            if (colorString == QLatin1String("hotLight")) setSystemColor( SystemColor::hotLight);
            if (colorString == QLatin1String("gradientActiveCaption")) setSystemColor( SystemColor::gradientActiveCaption);
            if (colorString == QLatin1String("gradientInactiveCaption")) setSystemColor( SystemColor::gradientInactiveCaption);
            if (colorString == QLatin1String("menuHighlight")) setSystemColor( SystemColor::menuHighlight);
            if (colorString == QLatin1String("menuBar")) setSystemColor( SystemColor::menuBar);

            tr.read(reader);

            //TODO: integrate lastColor to val
//            QColor lastColor;
//            if (attributes.hasAttribute(QLatin1String("lastClr")))
//                lastColor = fromARGBString(attributes.value("lastClr").toString());
            break;
        }
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
