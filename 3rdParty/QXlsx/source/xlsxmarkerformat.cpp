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

MarkerFormat::MarkerType MarkerFormat::markerType() const
{
    if (d) return d->markerType;
    return MarkerType::MT_Diamond;
}

void MarkerFormat::setMarkerType(MarkerFormat::MarkerType type)
{
    if (!d)
        d = new MarkerFormatPrivate;
    d->markerType = type;
}

int MarkerFormat::size() const
{
    if (d) return d->size;
    return 7;
}

void MarkerFormat::setSize(int size)
{
    if (!d)
        d = new MarkerFormatPrivate;
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

QByteArray MarkerFormat::formatKey() const
{
    if (!d)
        return QByteArray();

    QByteArray key;
    QDataStream stream(&key, QIODevice::WriteOnly);

    stream<<"type"<<d->markerType;

    return key;
}

/*!
    Returns ture if the \a format is equal to this format.
*/
bool MarkerFormat::operator ==(const MarkerFormat &format) const
{
    return this->formatKey() == format.formatKey();
}

/*!
    Returns ture if the \a format is not equal to this format.
*/
bool MarkerFormat::operator !=(const MarkerFormat &format) const
{
    return this->formatKey() != format.formatKey();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const MarkerFormat &f)
{
    dbg.nospace() << "QXlsx::MarkerFormat(" << f.d->markerType << ")";
    return dbg.space();
}
#endif

QT_END_NAMESPACE_XLSX
