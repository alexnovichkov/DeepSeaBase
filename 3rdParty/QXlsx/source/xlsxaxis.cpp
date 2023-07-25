#include <QtGlobal>
#include <QDataStream>
#include <QDebug>

#include "xlsxaxis.h"
#include "xlsxaxis_p.h"
#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

void Axis::Scaling::write(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(QStringLiteral("c:scaling")); // CT_Scaling (mandatory value)
    if (reversed.has_value()) {
        writer.writeEmptyElement(QStringLiteral("c:orientation"));
        writer.writeAttribute(QStringLiteral("val"), reversed.value() ? QStringLiteral("maxMin") : QStringLiteral("minMax")); // ST_Orientation
    }
    if (logBase.has_value()) {
        writer.writeEmptyElement(QStringLiteral("c:logBase"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(logBase.value()));
    }
    if (min.has_value()) {
        writer.writeEmptyElement(QStringLiteral("c:min"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(min.value()));
    }
    if (max.has_value()) {
        writer.writeEmptyElement(QStringLiteral("c:max"));
        writer.writeAttribute(QStringLiteral("val"), QString::number(max.value()));
    }
    writer.writeEndElement(); // c:scaling
}

void Axis::Scaling::read(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QStringLiteral("scaling"));

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            auto val = reader.attributes().value(QLatin1String("val"));
            if (reader.name() == QLatin1String("orientation")) {
                reversed = val.toString() == "maxMin";
            }
            else if (reader.name() == QLatin1String("logBase")) {
                logBase = val.toDouble();
            }
            else if (reader.name() == QLatin1String("min")) {
                min = val.toDouble();
            }
            else if (reader.name() == QLatin1String("max")) {
                max = val.toDouble();
            }
        }
        else if (reader.tokenType() == QXmlStreamReader::EndElement &&
                 reader.name() == QLatin1String("scaling")) {
            break;
        }
    }
}

Axis::Axis(Axis::Type type, Axis::Position position)
{
    d = new AxisPrivate;
    d->type = type;
    d->position = position;
}

Axis::Axis(Axis::Type type)
{
    d = new AxisPrivate;
    d->type = type;
}

bool Axis::isValid() const
{
    if (d) return true;
    return false;
}

Axis::Type Axis::type() const
{
    if (d) return d->type;
    return Axis::Type::None;
}

Axis::Position Axis::position() const
{
    if (d) return d->position;
    return Axis::Position::None;
}

void Axis::setPosition(Axis::Position position)
{
    if (!d) d = new AxisPrivate;
    d->position = position;
}

bool Axis::visible() const
{
    if (d) return d->visible.value_or(true);
    return true;
}

void Axis::setVisible(bool visible)
{
    if (!d) d = new AxisPrivate;
    d->visible = visible;
}

int Axis::id() const
{
    if (d) return d->id;
    return -1;
}

void Axis::setId(int id)
{
    if (!d) d = new AxisPrivate;
    d->id = id;
}

int Axis::crossAxis() const
{
    if (d) return d->crossAxis;
    return -1;
}

void Axis::setCrossAxis(Axis *axis)
{
    if (!d) d = new AxisPrivate;
    if (d->crossAxis != axis->id()) {
        d->crossAxis = axis->id();
        if (axis) axis->setCrossAxis(this);
    }
}

void Axis::setCrossAxis(int axisId)
{
    if (!d) d = new AxisPrivate;
    d->crossAxis = axisId;
}

double Axis::crossesAt() const
{
    if (d) return d->crossesPosition.value_or(0.0);
    return 0.0;
}

void Axis::setCrossesAt(double val)
{
    if (!d) d = new AxisPrivate;
    d->crossesType = CrossesType::Position;
    d->crossesPosition = val;
}

Axis::CrossesType Axis::crossesType() const
{
    if (d) return d->crossesType.value_or(CrossesType::AutoZero);
    return CrossesType::AutoZero;
}

void Axis::setCrossesAt(Axis::CrossesType val)
{
    if (!d) d = new AxisPrivate;
    d->crossesType = val;
}

QString Axis::title() const
{
    if (d) return d->title;
    return QString();
}

void Axis::setTitle(const QString &title)
{
    if (!d) d = new AxisPrivate;
    d->title = title;
}

Axis::Scaling *Axis::scaling()
{
    if (d) return &d->scaling;
    return nullptr;
}

void Axis::setScaling(Axis::Scaling scaling)
{
    if (!d) d = new AxisPrivate;
    d->scaling = scaling;
}

QPair<double, double> Axis::range() const
{
    if (d) return {d->scaling.min.value_or(0.0), d->scaling.max.value_or(0.0)};
    return {0.0, 0.0};
}

void Axis::setRange(double min, double max)
{
    if (!d) d = new AxisPrivate;
    d->scaling.min = min;
    d->scaling.max = max;
}

void Axis::write(QXmlStreamWriter &writer) const
{
    QString name;
    switch (d->type) {
        case Type::Cat: name = "c:catAx"; break;
        case Type::Ser: name = "c:serAx"; break;
        case Type::Val: name = "c:valAx"; break;
        case Type::Date: name = "c:dateAx"; break;
        default: break;
    }
    if (name.isEmpty()) return;

    writer.writeStartElement(name);
    writer.writeEmptyElement("c:axId");
    writer.writeAttribute("val", QString::number(d->id));
    d->scaling.write(writer);

    if (d->visible.has_value()) {
        writer.writeEmptyElement("c:delete");
        writer.writeAttribute("val", d->visible.value() ? "true" : "false");
    }

    writer.writeEmptyElement("c:axPos");
    QString s; toString(d->position, s); writer.writeAttribute(QLatin1String("val"), s);

    if (d->majorGridlines.isValid()) {
        writer.writeStartElement("c:majorGridlines");
        d->majorGridlines.write(writer, "c:spPr");
        writer.writeEndElement();
    }

    if (d->minorGridlines.isValid()) {
        writer.writeStartElement("c:minorGridlines");
        d->minorGridlines.write(writer, "c:spPr");
        writer.writeEndElement();
    }

    writer.writeEmptyElement("c:crossAx");
    if (d->crossAxis != -1)
        writer.writeAttribute("val", QString::number(d->crossAxis));

    if (d->crossesType.has_value()) {
        switch (d->crossesType.value()) {
            case CrossesType::Maximum: writer.writeEmptyElement(QLatin1String("c:crosses"));
                writer.writeAttribute("val", "max");
                break;
            case CrossesType::Minimum: writer.writeEmptyElement(QLatin1String("c:crosses"));
                writer.writeAttribute("val", "min");
                break;
            case CrossesType::AutoZero: writer.writeEmptyElement(QLatin1String("c:crosses"));
                writer.writeAttribute("val", "autoZero");
                break;
            case CrossesType::Position: writer.writeEmptyElement(QLatin1String("c:crossesAt"));
                writer.writeAttribute("val", QString::number(d->crossesPosition.value_or(0.0)));
                break;
        }
    }

    writer.writeEndElement();
}

void Axis::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    d->type = Type::None;
    if (name == "catAx") d->type = Type::Cat;
    if (name == "valAx") d->type = Type::Val;
    if (name == "serAx") d->type = Type::Ser;
    if (name == "dateAx") d->type = Type::Date;

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("axId")) {
                parseAttributeInt(reader.attributes(), QLatin1String("val"), d->id);
            }
            else if (reader.name() == QLatin1String("scaling")) {
                d->scaling.read(reader);
            }
            else if (reader.name() == QLatin1String("delete")) {
                parseAttributeBool(reader.attributes(), QLatin1String("val"), d->visible);
            }
            else if (reader.name() == QLatin1String("axPos")) {
                const auto &pos = reader.attributes().value("val").toString();
                fromString(pos, d->position);
            }
            else if (reader.name() == QLatin1String("majorGridlines")) {
                reader.readNextStartElement();
                d->majorGridlines.read(reader);
            }
            else if (reader.name() == QLatin1String("minorGridlines")) {
                reader.readNextStartElement();
                d->minorGridlines.read(reader);
            }
            else if (reader.name() == QLatin1String("title")) {

            }
            else if (reader.name() == QLatin1String("crossAx")) {
                d->crossAxis = reader.attributes().value("val").toInt();
            }
            else if (reader.name() == QLatin1String("crosses")) {
                const auto &s = reader.attributes().value(QLatin1String("val"));
                if (s == QLatin1String("autoZero")) d->crossesType = CrossesType::AutoZero;
                else if (s == QLatin1String("min")) d->crossesType = CrossesType::Minimum;
                if (s == QLatin1String("max")) d->crossesType = CrossesType::Maximum;
            }
            else if (reader.name() == QLatin1String("crossesAt")) {
                d->crossesType = CrossesType::Position;
                d->crossesPosition = reader.attributes().value(QLatin1String("val")).toDouble();
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

bool Axis::operator ==(const Axis &axis) const
{
    //TODO:
    return true;
}

bool Axis::operator !=(const Axis &axis) const
{
    //TODO:
    return false;
}

AxisPrivate::AxisPrivate() : position(Axis::Position::None), type(Axis::Type::None)
{

}

AxisPrivate::AxisPrivate(const AxisPrivate &other) : QSharedData(other)
  //TODO: add all members
{

}

QT_END_NAMESPACE_XLSX



