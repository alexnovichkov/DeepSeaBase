#include "converters.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

QString doubletohex(const double d)
{
    QString s;
    QByteArray ba;
    QDataStream stream(&ba,QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << d;
    s = "("+ba.toHex()+")";
    return s;
}

double hextodouble(QString hex)
{
    if (hex.startsWith("("))
        hex.remove(0,1);
    if (hex.endsWith(")"))
        hex.chop(1);

    double result=0.0l;

    QByteArray ba;
    for (int i=0; i<16; i+=2) {
        quint8 c = hex.mid(i,2).toUInt(0,16);
        ba.append(c);
    }

    QDataStream stream(ba);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> result;

    return result;
}



float hextofloat(QString hex)
{
    if (hex.startsWith("("))
        hex.remove(0,1);
    if (hex.endsWith(")"))
        hex.chop(1);

    float result=0.0;

    QByteArray ba;
    for (int i=0; i<8; i+=2) {
        quint8 c = hex.mid(i,2).toUInt(0,16);
        ba.append(c);
    }

    QDataStream stream(ba);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> result;

    return result;
}

QString floattohex(const float f)
{
    QString s;
    QByteArray ba;
    QDataStream stream(&ba,QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << f;
    s = "("+ba.toHex()+")";
    return s;
}
