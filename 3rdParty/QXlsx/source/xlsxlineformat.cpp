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
	: QSharedData(other)
    , color(other.color), width(other.width)
{

}

LineFormatPrivate::~LineFormatPrivate()
{

}

/*!
 *  Creates a new invalid format.
 */
LineFormat::LineFormat()
{
	//The d pointer is initialized with a null pointer
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

LineFormat::LineType LineFormat::lineType() const
{
    if (d) return d->lineType;
    return LineType::LT_SolidLine;
}

void LineFormat::setLineType(LineFormat::LineType type)
{
    if (!d)
        d = new LineFormatPrivate;
    d->lineType = type;
}

QColor LineFormat::color() const
{
    if (d) return d->color;
    return QColor();
}

void LineFormat::setColor(const QColor &color)
{
    if (!d)
        d = new LineFormatPrivate;
    d->color = color;
}

bool LineFormat::smooth() const
{
    if (d) return d->smooth;
    return false;
}

void LineFormat::setSmooth(bool smooth)
{
    if (!d)
        d = new LineFormatPrivate;
    d->smooth = smooth;
}

double LineFormat::width() const
{
    if (d) return d->width;
    return 0;
}

void LineFormat::setWidth(double width)
{
    if (!d)
        d = new LineFormatPrivate;
    d->width = width;
}

LineFormat::CompoundLineType LineFormat::compoundLineType() const
{
    if (d) return d->compoundLineType;
    return LineFormat::CLT_Single;
}

void LineFormat::setCompoundLineType(LineFormat::CompoundLineType compoundLineType)
{
    if (!d)
        d = new LineFormatPrivate;
    d->compoundLineType = compoundLineType;
}

LineFormat::StrokeType LineFormat::strokeType() const
{
    if (d) return d->strokeType;
    return LineFormat::ST_Solid;
}

void LineFormat::setStrokeType(LineFormat::StrokeType strokeType)
{
    if (!d)
        d = new LineFormatPrivate;
    d->strokeType = strokeType;
}

LineFormat::PointType LineFormat::pointType() const
{
    if (d) return d->pointType;
    return LineFormat::PT_Round;
}

void LineFormat::setPointType(LineFormat::PointType pointType)
{
    if (!d)
        d = new LineFormatPrivate;
    d->pointType = pointType;
}

double LineFormat::alpha() const
{
    if (d) return d->alpha;
    return 0.0;
}

void LineFormat::setAlpha(double alpha)
{
    if (!d)
        d = new LineFormatPrivate;
    d->alpha = alpha;
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

QByteArray LineFormat::formatKey() const
{
    if (!d)
        return QByteArray();

    QByteArray key;
    QDataStream stream(&key, QIODevice::WriteOnly);

    stream<<"color"<<d->color;
    stream<<"width"<<d->width;

    return key;
}

/*!
	Returns ture if the \a format is equal to this format.
*/
bool LineFormat::operator ==(const LineFormat &format) const
{
	return this->formatKey() == format.formatKey();
}

/*!
	Returns ture if the \a format is not equal to this format.
*/
bool LineFormat::operator !=(const LineFormat &format) const
{
	return this->formatKey() != format.formatKey();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const LineFormat &f)
{
    dbg.nospace() << "QXlsx::LineFormat(" << f.d->color << f.d->width << ")";
	return dbg.space();
}
#endif

QT_END_NAMESPACE_XLSX
