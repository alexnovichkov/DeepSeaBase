#ifndef FIELDS_H
#define FIELDS_H

#include <QtCore>

enum FieldType {
    FTDelimiter,
    FTFloat13_5,
    FTFloat15_7,
    FTFloat20_12,
    FTFloat25_17,
    FTInteger4,
    FTInteger5,
    FTInteger6,
    FTInteger10,
    FTInteger12,
    FTString80,
    FTString10,
    FTString10a,
    FTString20,
    FTTimeDate,
    FTTimeDate80,
    FTEmpty
};

class AbstractField
{
public:
    virtual void print(const QVariant &, QTextStream &)=0;
    virtual void read(QVariant &, QTextStream &)=0;
};

class DelimiterField: public AbstractField
{
public:
    virtual void print(const QVariant &/*v*/, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(6) << right << -1;
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        Q_UNUSED(v);
        QString s = stream.read(6);  //qDebug()<<s;
        Q_ASSERT(s=="    -1");
    }
};

class Float13_5Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(13) << qSetRealNumberPrecision(5) << scientific << uppercasebase << right << v.toDouble();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        double d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Float15_7Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(15) << qSetRealNumberPrecision(7) << scientific << right << v.toFloat();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        double d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Float20_12Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(20) << qSetRealNumberPrecision(12) << scientific << right << v.toDouble();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        double d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Float25_17Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(25) << qSetRealNumberPrecision(17) << scientific << right << v.toFloat();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        double d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Integer4Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(4) << right << v.toInt();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        int d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Integer5Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(5) << right << v.toInt();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        int d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Integer6Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(6) << right << v.toInt();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        int d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Integer10Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(10) << right << v.toInt();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        int d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class Integer12Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(12) << right << v.toInt();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        int d;
        stream >> d; //qDebug()<<d;
        v = d;
    }
};

class String80Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(80) << left << v.toString();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(80); //qDebug()<<s;
        Q_ASSERT(s.length()==80);
        v = s.trimmed();
    }
};

class String10Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << " " << qSetFieldWidth(10) << left << v.toString();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(11); //qDebug()<<s;
        Q_ASSERT(s.length()==11);
        v = s.trimmed();
    }
};

class String10aField: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(10) << left << v.toString();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(10); //qDebug()<<s;
        Q_ASSERT(s.length()==10);
        v = s.trimmed();
    }
};

class String20Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << " " << qSetFieldWidth(20) << left << v.toString();
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(21); //qDebug()<<s;
        Q_ASSERT(s.length()==21);
        v = s.trimmed();
    }
};

class TimeDateField: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << v.toDateTime().toString(" dd.MM.yy hh:mm:ss");
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QDateTime d;
        d = QDateTime::fromString(stream.read(18), " dd.MM.yy hh:mm:ss"); //qDebug()<<d;
        v = d;
    }
};

class TimeDate80Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << qSetFieldWidth(80) << left << v.toDateTime().toString(" dd.MM.yy hh:mm:ss");
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QDateTime d;
        d = QDateTime::fromString(stream.read(80).trimmed(), "dd.MM.yy hh:mm:ss"); //qDebug()<<d;
        v = d;
    }
};

class EmptyField: public AbstractField
{
public:
    virtual void print(const QVariant &, QTextStream &stream) {
        stream.reset();
        stream << endl;
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        Q_UNUSED(v);
        stream.readLine();
    }
};

struct FieldDescription {
    FieldType type;
    QVariant value;
};

#endif // FIELDS_H
