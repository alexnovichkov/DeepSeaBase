#include <QtGlobal>
#include <QDataStream>
#include <QDebug>

#include "xlsxmarkerformat.h"
#include "xlsxmarkerformat_p.h"

QT_BEGIN_NAMESPACE_XLSX

MarkerFormatPrivate::MarkerFormatPrivate()
{
}

MarkerFormatPrivate::MarkerFormatPrivate(const MarkerFormatPrivate &other)
    : QSharedData(other)

{

}

MarkerFormatPrivate::~MarkerFormatPrivate()
{

}

/*!
 *  Creates a new invalid format.
 */
MarkerFormat::MarkerFormat()
{
    //The d pointer is initialized with a null pointer
}

MarkerFormat::MarkerFormat(MarkerFormat::MarkerType type)
{
    d = new MarkerFormatPrivate;
    d->markerType = type;
}

/*!
   Creates a new format with the same attributes as the \a other format.
 */
MarkerFormat::MarkerFormat(const MarkerFormat &other)
    :d(other.d)
{

}

/*!
   Assigns the \a other format to this format, and returns a
   reference to this format.
 */
MarkerFormat &MarkerFormat::operator =(const MarkerFormat &other)
{
    d = other.d;
    return *this;
}

/*!
 * Destroys this format.
 */
MarkerFormat::~MarkerFormat()
{
}

std::optional<MarkerFormat::MarkerType> MarkerFormat::type() const
{
    if (d) return d->markerType;
    return {};
}

void MarkerFormat::setType(MarkerFormat::MarkerType type)
{
    if (!d) d = new MarkerFormatPrivate;
    d->markerType = type;
}

void MarkerFormat::write(QXmlStreamWriter &writer)
{
    if (!d) return;

    writer.writeStartElement("c:marker");

    if (d->markerType.has_value()) {
        writer.writeEmptyElement("c:symbol");
        switch (d->markerType.value()) {
            case MarkerType::X: writer.writeAttribute("val", "x"); break;
            case MarkerType::None: writer.writeAttribute("val", "none"); break;
            case MarkerType::Square: writer.writeAttribute("val", "square"); break;
            case MarkerType::Dot: writer.writeAttribute("val", "dot"); break;
            case MarkerType::Dash: writer.writeAttribute("val", "dash"); break;
            case MarkerType::Diamond: writer.writeAttribute("val", "diamond"); break;
            case MarkerType::Auto: writer.writeAttribute("val", "auto"); break;
            case MarkerType::Plus: writer.writeAttribute("val", "plus"); break;
            case MarkerType::Star: writer.writeAttribute("val", "star"); break;
            case MarkerType::Cross: writer.writeAttribute("val", "cross"); break;
            case MarkerType::Circle: writer.writeAttribute("val", "circle"); break;
            case MarkerType::Picture: writer.writeAttribute("val", "picture"); break;
            case MarkerType::Triangle: writer.writeAttribute("val", "triangle"); break;
        }
    }

    if (d->size.has_value()) {
        writer.writeEmptyElement("c:size");
        writer.writeAttribute("val", QString::number(d->size.value()));
    }

    if (d->shape.isValid()) {
        d->shape.write(writer, "c:spPr");
    }

    writer.writeEndElement();
}

void MarkerFormat::read(QXmlStreamReader &reader)
{
    //TODO: implement marker reading
}

ShapeProperties MarkerFormat::shape() const
{
    if (d) return d->shape;
    return {};
}

void MarkerFormat::setShape(ShapeProperties shape)
{
    if (!d) d = new MarkerFormatPrivate;
    d->shape = shape;
}

std::optional<int> MarkerFormat::size() const
{
    if (d) return d->size;
    return {};
}

void MarkerFormat::setSize(int size)
{
    if (!d) d = new MarkerFormatPrivate;
    d->size = size;
}

/*!
    Returns true if the format is valid; otherwise returns false.
 */
bool MarkerFormat::isValid() const
{
    if (d)
        return true;
    return false;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const MarkerFormat &f)
{
    dbg.nospace() << "QXlsx::MarkerFormat(" << f.d->markerType.value() << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE_XLSX
