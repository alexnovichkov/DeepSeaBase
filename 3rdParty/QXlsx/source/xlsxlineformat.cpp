// xlsxlineformat.cpp

#include <QtGlobal>
#include <QDataStream>
#include <QDebug>

#include "xlsxlineformat.h"
#include "xlsxlineformat_p.h"

QT_BEGIN_NAMESPACE_XLSX

LineFormatPrivate::LineFormatPrivate()
{
}

LineFormatPrivate::LineFormatPrivate(const LineFormatPrivate &other)
    : QSharedData(other), fill(other.fill),
      width(other.width),
      compoundLineType(other.compoundLineType),
      strokeType(other.strokeType),
      lineCap(other.lineCap),
      penAlignment(other.penAlignment),
      lineJoin(other.lineJoin),
      tailType(other.tailType),
      headType(other.headType),
      tailLength(other.tailLength),
      headLength(other.headLength),
      tailWidth(other.tailWidth),
      headWidth(other.headWidth)
{

}

LineFormatPrivate::~LineFormatPrivate()
{

}

bool LineFormatPrivate::operator ==(const LineFormatPrivate &format) const
{
    if (fill != format.fill) return false;
    if (width != format.width) return false;
    if (compoundLineType != format.compoundLineType) return false;
    if (strokeType != format.strokeType) return false;
    if (lineCap != format.lineCap) return false;
    if (penAlignment != format.penAlignment) return false;
    if (lineJoin != format.lineJoin) return false;
    if (tailType != format.tailType) return false;
    if (headType != format.headType) return false;
    if (tailLength != format.tailLength) return false;
    if (headLength != format.headLength) return false;
    if (tailWidth != format.tailWidth) return false;
    if (headWidth != format.headWidth) return false;

    return true;
}

/*!
 *  Creates a new invalid format.
 */
LineFormat::LineFormat()
{
    //The d pointer is initialized with a null pointer
}

LineFormat::LineFormat(FillProperties::FillType fill, double widthInPt, QColor color)
{
    d = new LineFormatPrivate;
    d->fill.setType(fill);
    d->fill.setColor(color);
    setWidth(widthInPt);
}

LineFormat::LineFormat(FillProperties::FillType fill, qint64 widthInEMU, QColor color)
{
    d = new LineFormatPrivate;
    d->fill.setType(fill);
    d->fill.setColor(color);
    setWidth(widthInEMU);
}

/*!
   Creates a new format with the same attributes as the \a other format.
 */
LineFormat::LineFormat(const LineFormat &other)
	:d(other.d)
{

}

/*!
   Assigns the \a other format to this format, and returns a
   reference to this format.
 */
LineFormat &LineFormat::operator =(const LineFormat &other)
{
	d = other.d;
	return *this;
}

/*!
 * Destroys this format.
 */
LineFormat::~LineFormat()
{
}

FillProperties::FillType LineFormat::type() const
{
    if (d->fill.isValid()) return d->fill.type();
    return FillProperties::FillType::SolidFill; //default
}

void LineFormat::setType(FillProperties::FillType type)
{
    if (!d) d = new LineFormatPrivate;
    d->fill.setType(type);
}

Color LineFormat::color() const
{
    if (d && d->fill.isValid()) return d->fill.color();
    return Color();
}

void LineFormat::setColor(const Color &color)
{
    if (!d) d = new LineFormatPrivate;
    d->fill.setColor(color);
}

void LineFormat::setColor(const QColor &color)
{
    if (!d) d = new LineFormatPrivate;
    d->fill.setColor(color);
}

FillProperties LineFormat::fill() const
{
    if (d) return d->fill;
    return {};
}

void LineFormat::setFill(const FillProperties &fill)
{
    if (!d) d = new LineFormatPrivate;
    d->fill = fill;
}

Coordinate LineFormat::width() const
{
    if (d) return d->width;
    return Coordinate();
}

void LineFormat::setWidth(double widthInPt)
{
    if (!d) d = new LineFormatPrivate;
    d->width = Coordinate(widthInPt);
}

void LineFormat::setWidthEMU(qint64 widthInEMU)
{
    if (!d) d = new LineFormatPrivate;
    d->width = Coordinate(widthInEMU);
}

std::optional<LineFormat::PenAlignment> LineFormat::penAlignment() const
{
    if (d) return d->penAlignment;
    return {};
}

void LineFormat::setPenAlignment(LineFormat::PenAlignment val)
{
    if (!d) d = new LineFormatPrivate;
    d->penAlignment = val;
}

std::optional<LineFormat::CompoundLineType> LineFormat::compoundLineType() const
{
    if (d) return d->compoundLineType;
    return {};
}

void LineFormat::setCompoundLineType(LineFormat::CompoundLineType val)
{
    if (!d) d = new LineFormatPrivate;
    d->compoundLineType = val;
}

std::optional<LineFormat::StrokeType> LineFormat::strokeType() const
{
    if (d) return d->strokeType;
    return {};
}

void LineFormat::setStrokeType(LineFormat::StrokeType val)
{
    if (!d) d = new LineFormatPrivate;
    d->strokeType = val;
}

LineFormat::LineCap LineFormat::lineCap() const
{
    if (d) return d->lineCap.value();
    return LineCap::Square;
}

void LineFormat::setLineCap(LineCap val)
{
    if (!d) d = new LineFormatPrivate;
    d->lineCap = val;
}

std::optional<LineFormat::LineJoin> LineFormat::lineJoin() const
{
    if (d) return d->lineJoin;
    return {};
}

void LineFormat::setLineJoin(LineFormat::LineJoin val)
{
    if (!d) d = new LineFormatPrivate;
    d->lineJoin = val;
}

std::optional<LineFormat::LineEndType> LineFormat::lineEndType()
{
    if (d) return d->tailType;
    return {};
}

std::optional<LineFormat::LineEndType> LineFormat::lineStartType()
{
    if (d) return d->headType;
    return {};
}

void LineFormat::setLineEndType(LineFormat::LineEndType val)
{
    if (!d) d = new LineFormatPrivate;
    d->tailType = val;
}

void LineFormat::setLineStartType(LineFormat::LineEndType val)
{
    if (!d) d = new LineFormatPrivate;
    d->headType = val;
}

std::optional<LineFormat::LineEndSize> LineFormat::lineEndLength()
{
    if (d) return d->tailLength;
    return {};
}

std::optional<LineFormat::LineEndSize> LineFormat::lineStartLength()
{
    if (d) return d->headLength;
    return {};
}

void LineFormat::setLineEndLength(LineFormat::LineEndSize val)
{
    if (!d) d = new LineFormatPrivate;
    d->tailLength = val;
}

void LineFormat::setLineStartLength(LineFormat::LineEndSize val)
{
    if (!d) d = new LineFormatPrivate;
    d->headLength = val;
}

std::optional<LineFormat::LineEndSize> LineFormat::lineEndWidth()
{
    if (d) return d->tailWidth;
    return {};
}

std::optional<LineFormat::LineEndSize> LineFormat::lineStartWidth()
{
    if (d) return d->headWidth;
    return {};
}

void LineFormat::setLineEndWidth(LineFormat::LineEndSize val)
{
    if (!d) d = new LineFormatPrivate;
    d->tailWidth = val;
}

void LineFormat::setLineStartWidth(LineFormat::LineEndSize val)
{
    if (!d) d = new LineFormatPrivate;
    d->headWidth = val;
}

/*!
	Returns true if the format is valid; otherwise returns false.
 */
bool LineFormat::isValid() const
{
	if (d)
		return true;
    return false;
}

void LineFormat::write(QXmlStreamWriter &writer) const
{
    if (!isValid()) return;

    writer.writeStartElement("a:ln");
    if (d->width.isValid()) writer.writeAttribute("w", d->width.toString());
    if (d->compoundLineType.has_value()) {
        QString s;
        toString(d->compoundLineType.value(), s);
        writer.writeAttribute("cmpd", s);
    }
    if (d->lineCap.has_value()) {
        switch (d->lineCap.value()) {
            case LineCap::Flat: writer.writeAttribute("cap", "flat"); break;
            case LineCap::Round: writer.writeAttribute("cap", "rnd"); break;
            case LineCap::Square: writer.writeAttribute("cap", "sq"); break;
        }
    }
    if (d->penAlignment.has_value()) {
        switch (d->penAlignment.value()) {
            case PenAlignment::Center: writer.writeAttribute("algn", "ctr"); break;
            case PenAlignment::Inset: writer.writeAttribute("algn", "in"); break;
        }
    }
    if (d->strokeType.has_value()) {
        writer.writeEmptyElement("a:prstDash");
        switch (d->strokeType.value()) {
            case LineFormat::StrokeType::Solid: writer.writeAttribute("val", "solid"); break;
            case LineFormat::StrokeType::Dot: writer.writeAttribute("val", "sysDash"); break;
            case LineFormat::StrokeType::RoundDot: writer.writeAttribute("val", "sysDot"); break;
            case LineFormat::StrokeType::Dash: writer.writeAttribute("val", "dash"); break;
            case LineFormat::StrokeType::DashDot: writer.writeAttribute("val", "dashDot"); break;
            case LineFormat::StrokeType::LongDash: writer.writeAttribute("val", "lgDash"); break;
            case LineFormat::StrokeType::LongDashDot: writer.writeAttribute("val", "lgDashDot"); break;
            case LineFormat::StrokeType::LongDashDotDot: writer.writeAttribute("val", "lgDashDotDot"); break;
        }
    }
    if (d->lineJoin.has_value()) {
        switch (d->lineJoin.value()) {
            case LineJoin::Bevel: writer.writeEmptyElement("a:bevel"); break;
            case LineJoin::Miter: writer.writeEmptyElement("a:miter"); break;
            case LineJoin::Round: writer.writeEmptyElement("a:round"); break;
        }
    }
    if (d->headType.has_value() || d->headLength.has_value() || d->headWidth.has_value()) {
        writer.writeEmptyElement("a:headEnd");
        if (d->headType.has_value()) {
            switch (d->headType.value()) {
                case LineEndType::None: writer.writeAttribute("type", "none"); break;
                case LineEndType::Oval: writer.writeAttribute("type", "oval"); break;
                case LineEndType::Arrow: writer.writeAttribute("type", "arrow"); break;
                case LineEndType::Diamond: writer.writeAttribute("type", "diamond"); break;
                case LineEndType::Stealth: writer.writeAttribute("type", "stealth"); break;
                case LineEndType::Triangle: writer.writeAttribute("type", "triangle"); break;
            }
        }
        if (d->headLength.has_value()) {
            switch (d->headLength.value()) {
                case LineEndSize::Large: writer.writeAttribute("len", "lg"); break;
                case LineEndSize::Small: writer.writeAttribute("len", "sm"); break;
                case LineEndSize::Medium: writer.writeAttribute("len", "med"); break;
            }
        }
        if (d->headWidth.has_value()) {
            switch (d->headWidth.value()) {
                case LineEndSize::Large: writer.writeAttribute("w", "lg"); break;
                case LineEndSize::Small: writer.writeAttribute("w", "sm"); break;
                case LineEndSize::Medium: writer.writeAttribute("w", "med"); break;
            }
        }
    }
    if (d->tailType.has_value() || d->tailLength.has_value() || d->tailWidth.has_value()) {
        writer.writeEmptyElement("a:tailEnd");
        if (d->tailType.has_value()) {
            switch (d->tailType.value()) {
                case LineEndType::None: writer.writeAttribute("type", "none"); break;
                case LineEndType::Oval: writer.writeAttribute("type", "oval"); break;
                case LineEndType::Arrow: writer.writeAttribute("type", "arrow"); break;
                case LineEndType::Diamond: writer.writeAttribute("type", "diamond"); break;
                case LineEndType::Stealth: writer.writeAttribute("type", "stealth"); break;
                case LineEndType::Triangle: writer.writeAttribute("type", "triangle"); break;
            }
        }
        if (d->tailLength.has_value()) {
            switch (d->tailLength.value()) {
                case LineEndSize::Large: writer.writeAttribute("len", "lg"); break;
                case LineEndSize::Small: writer.writeAttribute("len", "sm"); break;
                case LineEndSize::Medium: writer.writeAttribute("len", "med"); break;
            }
        }
        if (d->tailWidth.has_value()) {
            switch (d->tailWidth.value()) {
                case LineEndSize::Large: writer.writeAttribute("w", "lg"); break;
                case LineEndSize::Small: writer.writeAttribute("w", "sm"); break;
                case LineEndSize::Medium: writer.writeAttribute("w", "med"); break;
            }
        }
    }

    if (d->fill.isValid()) d->fill.write(writer);

    writer.writeEndElement(); //a:ln
}

void LineFormat::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();
    if (a.hasAttribute("w")) d->width = Coordinate::create(a.value("w").toString());
    if (a.hasAttribute("cap")) {
        const auto &s = a.value("cap");
        if (s == "flat") d->lineCap = LineCap::Flat;
        if (s == "rnd") d->lineCap = LineCap::Round;
        if (s == "sq") d->lineCap = LineCap::Square;
    }
    if (a.hasAttribute("cmpd")) {
        const auto &s = a.value("cmpd");
        if (s == "sng") d->compoundLineType = CompoundLineType::Single;
        if (s == "dbl") d->compoundLineType = CompoundLineType::Double;
        if (s == "thickThin") d->compoundLineType = CompoundLineType::ThickThin;
        if (s == "thinThick") d->compoundLineType = CompoundLineType::ThinThick;
        if (s == "tri") d->compoundLineType = CompoundLineType::Triple;
    }
    if (a.hasAttribute("algn")) {
        const auto &s = a.value("algn");
        if (s == "ctr") d->penAlignment = PenAlignment::Center;
        if (s == "in") d->penAlignment = PenAlignment::Inset;
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == "noFill") {
                d->fill.read(reader);
            }
            if (reader.name() == "solidFill") {
                d->fill.read(reader);
            }
            if (reader.name() == "gradFill") {
                d->fill.read(reader);
            }
            if (reader.name() == "pattFill") {
                d->fill.read(reader);
            }
            if (reader.name() == "prstDash") {
                auto val = reader.attributes().value("val");
                if (val == QLatin1String("solid")) d->strokeType = LineFormat::StrokeType::Solid;
                if (val == QLatin1String("sysDash")) d->strokeType = LineFormat::StrokeType::Dot;
                if (val == QLatin1String("sysDot")) d->strokeType = LineFormat::StrokeType::RoundDot;
                if (val == QLatin1String("dash")) d->strokeType = LineFormat::StrokeType::Dash;
                if (val == QLatin1String("dashDot")) d->strokeType = LineFormat::StrokeType::DashDot;
                if (val == QLatin1String("lgDash")) d->strokeType = LineFormat::StrokeType::LongDash;
                if (val == QLatin1String("lgDashDot")) d->strokeType = LineFormat::StrokeType::LongDashDot;
                if (val == QLatin1String("lgDashDotDot")) d->strokeType = LineFormat::StrokeType::LongDashDotDot;
            }
            if (reader.name() == "round") d->lineJoin = LineFormat::LineJoin::Round;
            if (reader.name() == "bevel") d->lineJoin = LineFormat::LineJoin::Bevel;
            if (reader.name() == "miter") d->lineJoin = LineFormat::LineJoin::Miter;
            if (reader.name() == "headEnd") {
                const auto &a = reader.attributes();
                if (a.hasAttribute("type")) {
                    const auto &s = a.value("type");
                    if (s == "none") d->headType = LineEndType::None;
                    else if (s == "oval") d->headType = LineEndType::Oval;
                    else if (s == "arrow") d->headType = LineEndType::Arrow;
                    else if (s == "diamond") d->headType = LineEndType::Diamond;
                    else if (s == "stealth") d->headType = LineEndType::Stealth;
                    else if (s == "triangle") d->headType = LineEndType::Triangle;
                }
                if (a.hasAttribute("len")) {
                    const auto &s = a.value("len");
                    if (s == "lg") d->headLength = LineEndSize::Large;
                    else if (s == "sm") d->headLength = LineEndSize::Small;
                    else if (s == "med") d->headLength = LineEndSize::Medium;
                }
                if (a.hasAttribute("w")) {
                    const auto &s = a.value("w");
                    if (s == "lg") d->headWidth = LineEndSize::Large;
                    else if (s == "sm") d->headWidth = LineEndSize::Small;
                    else if (s == "med") d->headWidth = LineEndSize::Medium;
                }
            }if (reader.name() == "tailEnd") {
                const auto &a = reader.attributes();
                if (a.hasAttribute("type")) {
                    const auto &s = a.value("type");
                    if (s == "none") d->tailType = LineEndType::None;
                    else if (s == "oval") d->tailType = LineEndType::Oval;
                    else if (s == "arrow") d->tailType = LineEndType::Arrow;
                    else if (s == "diamond") d->tailType = LineEndType::Diamond;
                    else if (s == "stealth") d->tailType = LineEndType::Stealth;
                    else if (s == "triangle") d->tailType = LineEndType::Triangle;
                }
                if (a.hasAttribute("len")) {
                    const auto &s = a.value("len");
                    if (s == "lg") d->tailLength = LineEndSize::Large;
                    else if (s == "sm") d->tailLength = LineEndSize::Small;
                    else if (s == "med") d->tailLength = LineEndSize::Medium;
                }
                if (a.hasAttribute("w")) {
                    const auto &s = a.value("w");
                    if (s == "lg") d->tailWidth = LineEndSize::Large;
                    else if (s == "sm") d->tailWidth = LineEndSize::Small;
                    else if (s == "med") d->tailWidth = LineEndSize::Medium;
                }
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

bool LineFormat::operator ==(const LineFormat &other) const
{
    if (d == other.d) return true;
    if (!d || !other.d) return false;
    return *this->d.constData() == *other.d.constData();
}

bool LineFormat::operator !=(const LineFormat &other) const
{
    return !(operator==(other));
}

QXlsx::LineFormat::operator QVariant() const
{
    const auto& cref
#if QT_VERSION >= 0x060000 // Qt 6.0 or over
        = QMetaType::fromType<LineFormat>();
#else
        = qMetaTypeId<LineFormat>() ;
#endif
    return QVariant(cref, this);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const LineFormat &f)
{
    QDebugStateSaver saver(dbg);

    dbg.nospace() << "QXlsx::LineFormat(" ;
    dbg.nospace() << f.d->fill << ", ";
    dbg.nospace() << "width=" << (f.d->width.isValid() ? f.d->width.toString() : QString("not set")) << ", ";
    dbg.nospace() << "strokeType=" << (f.d->strokeType.has_value() ? static_cast<int>(f.d->strokeType.value()) : -1) << ", ";
    dbg.nospace() << "compoundLineType=" << (f.d->compoundLineType.has_value() ? static_cast<int>(f.d->compoundLineType.value()) : -1) << ", ";
    dbg.nospace() << "lineCap=" << (f.d->lineCap.has_value() ? static_cast<int>(f.d->lineCap.value()) : -1) << ", ";
    dbg.nospace() << "penAlignment=" << (f.d->penAlignment.has_value() ? static_cast<int>(f.d->penAlignment.value()) : -1) << ", ";
    dbg.nospace() << "lineJoin=" << (f.d->lineJoin.has_value() ? static_cast<int>(f.d->lineJoin.value()) : -1) << ", ";
    dbg.nospace() << ")";
    return dbg;
}
#endif

QT_END_NAMESPACE_XLSX
