#ifndef FIELDS_H
#define FIELDS_H

#include <QtCore>

#include <strtk.hpp>

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
    virtual int read(QVariant &v, char *pos, qint64 &offset) = 0;
};

class DelimiterField: public AbstractField
{
public:
    virtual void print(const QVariant &/*v*/, QTextStream &stream) {
        stream.reset();
        stream << "    -1";
    }
    virtual void read(QVariant &, QTextStream &stream) {
        QString s = stream.read(6);  //qDebug()<<s;
        Q_ASSERT(s=="    -1");
    }
    virtual int read(QVariant &, char *pos, qint64 &offset) {
        //qDebug()<<"reading DelimiterField at"<<offset;
        QString s = QString::fromLatin1(pos + offset, 6);  //qDebug()<<s;
        Q_ASSERT(s=="    -1");
        //qDebug()<<"read"<<6;
        return 6;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Float13_5Field at"<<offset;
        int width=13;
        pos+=offset;
        while (*pos==' ') {
            width--;
            pos++;
        }
        double d = strtk::string_to_type_converter<double>(pos, pos+width);  //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<13;
        return 13;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Float15_7Field at"<<offset;
        int w=15;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        double d = strtk::string_to_type_converter<double>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<15;
        return 15;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Float20_12Field at"<<offset;
        int w=20;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        double d = strtk::string_to_type_converter<double>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<20;
        return 20;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Float25_17Field at"<<offset;
        int w=25;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        double d = strtk::string_to_type_converter<double>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<25;
        return 25;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Integer4Field at"<<offset;
        int w=4;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        int d = strtk::string_to_type_converter<int>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<4;
        return 4;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Integer5Field at"<<offset;
        int w=5;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        int d = strtk::string_to_type_converter<int>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<5;
        return 5;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Integer6Field at"<<offset;
        int width=6;
        pos+=offset;
        while (*pos==' ') {
            width--;
            pos++;
        }
        int d = strtk::string_to_type_converter<int>(pos, pos+width); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<6;
        return 6;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Integer10Field at"<<offset;
        int w=10;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        int d = strtk::string_to_type_converter<int>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<10;
        return 10;
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
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading Integer12Field at"<<offset;
        int w=12;
        pos+=offset;
        while (*pos==' ') {
            w--;
            pos++;
        }
        int d = strtk::string_to_type_converter<int>(pos, pos+w); //qDebug()<<d;
        v = d;
        //qDebug()<<"read"<<12;
        return 12;
    }
};

class String80Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        QString s = v.toString();
        if (s.trimmed().isEmpty())
            s = "NONE";
        stream << left << s.leftJustified(80, ' ',true);
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s;
        for (int i=0; i<80; ++i) {
            QString c = stream.read(1);
            if (c!="\n") s+=c;
            else {
                stream.seek(stream.pos()-1);
                break;
            }
        }
        if (s.trimmed() == "NONE") s = "";
        v = s.trimmed();
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //reading at most 80 chars
        int len = 0;
        //qDebug()<<"reading String80 at"<<offset;
        pos+=offset;
        for (; len<80; ++len) {
            if (*(pos+len) == '\n' || *(pos+len) == '\r') break;
//            pos += 1;
        }

        QString s = QString::fromLocal8Bit(pos, len); //qDebug()<<s;
        s=s.trimmed();
        if (s=="NONE") s.clear();
        v = s;
        //qDebug()<<"read"<<len;
        return len;
    }
};

class String10Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        QString s = v.toString();
        if (s.trimmed().isEmpty())
            s = "NONE";
        stream << " " << s.leftJustified(10, ' ', true);
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(11); //qDebug()<<s;
        Q_ASSERT(s.length()==11);
        if (s.trimmed() == "NONE") s = "";
        v = s.trimmed();
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading String10Field at"<<offset;
        QString s = QString::fromLocal8Bit(pos+offset, 11);  //qDebug()<<s;
        s=s.trimmed();
        if (s=="NONE") s.clear();
        v = s;
        //qDebug()<<"read"<<11;
        return 11;
    }
};

class String10aField: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        QString s = v.toString();
        if (s.trimmed().isEmpty())
            s = "NONE  NONE";
        stream << s.leftJustified(10, ' ', true);
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(10); //qDebug()<<s;
        Q_ASSERT(s.length()==10);
        if (s.trimmed() == "NONE  NONE") s = "";
        v = s.trimmed();
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading String10aField at"<<offset;
        QString s = QString::fromLocal8Bit(pos+offset, 10);  //qDebug()<<s;
        s=s.trimmed();
        if (s=="NONE  NONE") s.clear();
        v = s;
        //qDebug()<<"read"<<10;
        return 10;
    }
};

class String20Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        QString s = v.toString();
        if (s.trimmed().isEmpty())
            s = "NONE";
        stream << " " << s.leftJustified(20, ' ', true);
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QString s = stream.read(21); //qDebug()<<s;
        Q_ASSERT(s.length()==21);
        if (s.trimmed() == "NONE") s = "";
        v = s.trimmed();
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading String20Field at"<<offset;
        QString s = QString::fromLocal8Bit(pos+offset, 21);  //qDebug()<<s;
        s=s.trimmed();
        if (s=="NONE") s.clear();
        v = s;
        //qDebug()<<"read"<<21;
        return 21;
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
        QString s = stream.read(18);
        d = QDateTime::fromString(s, " dd.MM.yy hh:mm:ss");
        if (!d.isValid()) d = QDateTime::fromString(s, "dd-MMM-yy hh:mm:ss");
        if (d.date().year()<1950) d=d.addYears(100);
        v = d;
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //qDebug()<<"reading TimeDateField at"<<offset;
        QString s = QString::fromLocal8Bit(pos+offset, 18);   //qDebug()<<s;
        QDateTime d = QDateTime::fromString(s, " dd.MM.yy hh:mm:ss");
        if (!d.isValid()) d = QDateTime::fromString(s, "dd-MMM-yy hh:mm:ss");
        if (d.date().year()<1950) d = d.addYears(100);
        v = d;
        //qDebug()<<"read"<<18;
        return 18;
    }
};

class TimeDate80Field: public AbstractField
{
public:
    virtual void print(const QVariant &v, QTextStream &stream) {
        stream.reset();
        stream << v.toDateTime().toString(" dd.MM.yy hh:mm:ss").leftJustified(80, ' ');
    }
    virtual void read(QVariant &v, QTextStream &stream) {
        QDateTime d;
        QString s;
        for (int i=0; i<80; ++i) {
            QString c = stream.read(1);
            if (c!="\n") s+=c;
            else {
                stream.seek(stream.pos()-1);
                break;
            }
        }

        d = QDateTime::fromString(s.trimmed(), "dd.MM.yy hh:mm:ss");
        if (!d.isValid()) d = QDateTime::fromString(s.trimmed(), "dd-MMM-yy hh:mm:ss");
        if (!d.isValid()) {
            QLocale l = QLocale::c();
            d = l.toDateTime(s.trimmed(), "dd-MMM-yy hh:mm:ss");
        }
        //qDebug()<<d;
        v = d;
    }
    virtual int read(QVariant &v, char *pos, qint64 &offset) {
        //reading at most 80 chars
        pos+=offset;
        //qDebug()<<"reading TimeDate80Field at"<<offset;
        int len = 0;
        for (; len<80; ++len) {
            if (*(pos+len) == '\n' || *(pos+len) == '\r') break;
        }

        QString s = QString::fromLocal8Bit(pos, len);  //qDebug()<<s;
        QDateTime d = QDateTime::fromString(s.trimmed(), "dd.MM.yy hh:mm:ss");
        if (!d.isValid()) d = QDateTime::fromString(s.trimmed(), "dd-MMM-yy hh:mm:ss");
        if (!d.isValid()) {
            QLocale l = QLocale::c();
            d = l.toDateTime(s.trimmed(), "dd-MMM-yy hh:mm:ss");
        }
        v = d;
        //qDebug()<<"read"<<len;
        return len;
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
    virtual int read(QVariant &, char *pos, qint64 &offset) {
        pos+=offset;
        //qDebug()<<"reading EmptyField at"<<offset;
        int len = 0;
        while (*pos == '\n' || *pos == '\r') {
            pos++;
            len++;
        }
        //qDebug()<<"read"<<len;
        return len;
    }
};

struct FieldDescription {
    FieldType type;
    QVariant value;
};

QDataStream &operator>>(QDataStream &stream, FieldDescription &field);
QDataStream &operator<<(QDataStream &stream, const FieldDescription &field);


void setType151(QVector<FieldDescription> &type151);

void setType164(QVector<FieldDescription> &type164);

void setType1858(QVector<FieldDescription> &type1858);

void setType58(QVector<FieldDescription> &type58);


#endif // FIELDS_H
