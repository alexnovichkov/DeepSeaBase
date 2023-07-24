#include "xlsxshapeproperties.h"
#include "xlsxshapeproperties_p.h"

QT_BEGIN_NAMESPACE_XLSX

ShapePropertiesPrivate::ShapePropertiesPrivate(const ShapePropertiesPrivate &other)
    : QSharedData(other) ,
      blackWhiteMode(other.blackWhiteMode),
      xfrm(other.xfrm),
      presetGeometry(other.presetGeometry),
      fill(other.fill),
      line(other.line)
{

}

std::optional<ShapeProperties::BlackWhiteMode> ShapeProperties::blackWhiteMode() const
{
    if (d) return d->blackWhiteMode;
    return {};
}

void ShapeProperties::setBlackWhiteMode(ShapeProperties::BlackWhiteMode val)
{
    if (!d) d = new ShapePropertiesPrivate;
    d->blackWhiteMode = val;
}

std::optional<Transform2D> ShapeProperties::xfrm() const
{
    if (d) return d->xfrm;
    return {};
}

void ShapeProperties::setXfrm(Transform2D val)
{
    if (!d) d = new ShapePropertiesPrivate;
    d->xfrm = val;
}

std::optional<PresetGeometry2D> ShapeProperties::presetGeometry() const
{
    if (d) return d->presetGeometry;
    return {};
}

void ShapeProperties::setPresetGeometry(PresetGeometry2D val)
{
    if (!d) d = new ShapePropertiesPrivate;
    d->presetGeometry = val;
}

FillProperties ShapeProperties::fill() const
{
    if (d) return d->fill;
    return {};
}

void ShapeProperties::setFill(const FillProperties &val)
{
    if (!d) d = new ShapePropertiesPrivate;
    d->fill = val;
}

LineFormat ShapeProperties::line() const
{
    if (d) return d->line;
    return {};
}

void ShapeProperties::setLine(const LineFormat &line)
{
    if (!d) d = new ShapePropertiesPrivate;
    d->line = line;
}

bool ShapeProperties::isValid() const
{
    if (d) return true;
    return false;
}

void ShapeProperties::write(QXmlStreamWriter &writer) const
{
    if (!d) return;

    writer.writeStartElement(QLatin1String("a:spPr"));
    if (d->blackWhiteMode.has_value()) writer.writeAttribute(QLatin1String("bwMode"), toString(d->blackWhiteMode.value()));
    if (d->xfrm.has_value()) d->xfrm->write(writer, QLatin1String("a:xfrm"));
    if (d->presetGeometry.has_value()) {
        d->presetGeometry->write(writer, QLatin1String("a:prstGeom"));
    }
    if (d->fill.isValid())
        d->fill.write(writer);
    if (d->line.isValid())
        d->line.write(writer);

    writer.writeEndElement(); //"a:spPr"
}

void ShapeProperties::read(QXmlStreamReader &reader)
{
    if (!d) d = new ShapePropertiesPrivate;

    const auto &name = reader.name();

    if (reader.attributes().hasAttribute(QLatin1String("bwMode"))) {
        auto bwMode = reader.attributes().value(QLatin1String("bwMode"));

        if (bwMode == QLatin1String("clr")) d->blackWhiteMode = BlackWhiteMode::Clear;
        else if (bwMode == QLatin1String("auto")) d->blackWhiteMode = BlackWhiteMode::Auto;
        else if (bwMode == QLatin1String("gray")) d->blackWhiteMode = BlackWhiteMode::Gray;
        else if (bwMode == QLatin1String("ltGray")) d->blackWhiteMode = BlackWhiteMode::LightGray;
        else if (bwMode == QLatin1String("invGray")) d->blackWhiteMode = BlackWhiteMode::InverseGray;
        else if (bwMode == QLatin1String("grayWhite")) d->blackWhiteMode = BlackWhiteMode::GrayWhite;
        else if (bwMode == QLatin1String("blackGray")) d->blackWhiteMode = BlackWhiteMode::BlackGray;
        else if (bwMode == QLatin1String("blackWhite")) d->blackWhiteMode = BlackWhiteMode::BlackWhite;
        else if (bwMode == QLatin1String("black")) d->blackWhiteMode = BlackWhiteMode::Black;
        else if (bwMode == QLatin1String("white")) d->blackWhiteMode = BlackWhiteMode::White;
        else if (bwMode == QLatin1String("hidden")) d->blackWhiteMode = BlackWhiteMode::Hidden;
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            auto val = reader.attributes().value(QLatin1String("val"));

            if (reader.name() == QLatin1String("xfrm")) {
                d->xfrm->read(reader);
            }
            else if (reader.name() == QLatin1String("prstGeom")) {
                d->presetGeometry->read(reader);
            }
            else if (reader.name() == QLatin1String("noFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("solidFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("gradFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("blipFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("pattFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("grpFill")) {
                d->fill.read(reader);
            }
            else if (reader.name() == QLatin1String("ln")) {
                d->line.read(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

QString ShapeProperties::toString(BlackWhiteMode bwMode) const
{
    switch (bwMode) {
        case BlackWhiteMode::Clear: return QLatin1String("clr");
        case BlackWhiteMode::Auto: return QLatin1String("auto");
        case BlackWhiteMode::Gray: return QLatin1String("gray");
        case BlackWhiteMode::LightGray: return QLatin1String("ltGray");
        case BlackWhiteMode::InverseGray: return QLatin1String("invGray");
        case BlackWhiteMode::GrayWhite: return QLatin1String("grayWhite");
        case BlackWhiteMode::BlackGray: return QLatin1String("blackGray");
        case BlackWhiteMode::BlackWhite: return QLatin1String("blackWhite");
        case BlackWhiteMode::Black: return QLatin1String("black");
        case BlackWhiteMode::White: return QLatin1String("white");
        case BlackWhiteMode::Hidden: return QLatin1String("hidden");
    }

    return QString();
}

QT_END_NAMESPACE_XLSX
