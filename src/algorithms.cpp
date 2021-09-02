#include "algorithms.h"
#include "logging.h"
#include <QDir>
#include "fileformats/formatfactory.h"

QDebug operator <<(QDebug debug, const std::complex<double> &val)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << val.real() << ", " << val.imag() << ')';
    return debug;
}

QString replaceWinChars(QString s)
{DD;
    static const struct ReplaceParams {
        const char *what;
        const char *byWhat;
    } params[] = {
        {"\\","_"},
        {":","_"},
        {"*","_"},
        {"|","_"},
        {"?","_"},
        {"/","_"},
        {"\"","''"},
        {"<","("},
        {">",")"}
    };
    for (int i=0; i<9; ++i) {
        s.replace(params[i].what,params[i].byWhat);
    }
    return s;
}

bool fileExists(const QString &s, const QString &suffix)
{
    QString f = changeFileExt(s, suffix);
    if (suffix != "dfd") return QFile::exists(f);

    QString f1 = changeFileExt(s, "raw");
    return (QFile::exists(f) && QFile::exists(f1));
}

QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep)
{DD;
    QPair<QVector<double>, QVector<double> > result;
    // определяем верхнюю границу данных
    const double F_min = xBegin;
    const double Step_f = xStep;
    const double F_max = F_min + Step_f * spectrum.size();

//    const double twothird = pow(2.0, 1.0/3.0);
//    const double twosixth = pow(2.0, 1.0/6.0);
    const double tenpow = pow(10.0, -1.4);
    const double twothird = pow(10.0, 0.1);
    const double twosixth = pow(10.0, 0.05);

   // double f_lower = 0;
    double f_upper = 0;
    double f_median = 0;
    int k_max = 52;
    for (int k = 52; k>=0; --k) {
        f_median = pow(10.0, 0.1 * k);
        f_upper = f_median * twosixth;
        if (f_upper <= F_max) {
            k_max = k;
            break;
        }
    }
    if (f_median == 1) return result;

    k_max++;

    QVector<double> xValues(k_max);
    for (int k = 0; k < k_max; ++k)
        xValues[k] = pow(10.0, 0.1*k);

    QVector<double> values(k_max);

    for (int k = 0; k < k_max; ++k) {
        // определяем диапазон в данном фильтре и число отсчетов для обработки
        int i_min = -1;
        int i_max = -1;
        f_median = xValues.at(k);

        double f_lower = f_median / twosixth;
        double f_upper = f_median * twosixth;

        for (int i = 0; i<spectrum.size(); ++i) {
            double f = F_min+i*Step_f;
            if (std::abs(f-f_lower) <= Step_f) {
                i_min = i;
                break;
            }
        }
        for (int i = spectrum.size()-1; i>=0; --i) {
            double f = F_min+i*Step_f;
            if (std::abs(f-f_upper) <= Step_f) {
                i_max = i;
                break;
            }
        }

        int steps = i_max - i_min + 1;
        double sum_i = 0.0;
        if (steps >= 5) {
            for (int i = i_min; i <= i_max; ++i) {
                double L = spectrum.at(i);
                double ratio = (F_min+i*Step_f)/f_median;
                double coef = 1.0;
                if (ratio>0.5 && ratio<=(1.0/twothird))
                    coef = tenpow/(1.0/twothird-0.5)*(ratio-0.5);
                else if (ratio>(1.0/twothird) && ratio<=(1.0/twosixth))
                    coef = (1.0-tenpow)/(1.0/twosixth-1.0/twothird)*(ratio - 1.0/twothird)+tenpow;
                else if (ratio>twosixth && ratio<=twothird)
                    coef = (tenpow-1.0)/(twothird-twosixth)*(ratio - twosixth)+1.0;
                else if (ratio>twothird && ratio<=2)
                    coef = tenpow/(twothird-2.0)*(ratio-twothird)+tenpow;
                Q_ASSERT(coef>0);
                L = L*coef;

                sum_i += pow(10.0, (L / 10.0));
            }
        }
        else {
            for (int i = i_min; i <= i_max; ++i) {
                double L = spectrum.at(i);
                sum_i += pow(10.0, (L / 10.0));
            }
        }
        values[k] = 10.0 * log10(sum_i);
    }
    result.first = xValues;
    result.second = values;
    return result;
}

QString createUniqueFileName(const QString &folderName, const QString &fileName, QString constantPart,
                             const QString &ext, bool justified)
{DD;
    QString result;
    QFileInfo fn = QFileInfo(fileName);

    if (folderName.isEmpty())
        result = fn.absolutePath()+"/"+fn.completeBaseName();
    else
        result = folderName+"/"+fn.completeBaseName();
    if (!constantPart.isEmpty()) result.append("_"+constantPart);

    if (!justified && !QFile::exists(result+"."+ext))
        return result+"."+ext;

    int index = justified?0:1;
    QString suffix = QString::number(index);
    if (justified)
        suffix = QString("_")+suffix.rightJustified(3,'0');
    else
        suffix = QString("(%1)").arg(suffix);

    while (QFile::exists(result+suffix+"."+ext)) {
        index++;
        suffix = justified ? QString("_")+QString::number(index).rightJustified(3,'0')
                           : QString("(%1)").arg(index);
    }
    return result+suffix+"."+ext;
}

QString createUniqueFileName(const QString &fileName)
{
    if (QFile::exists(fileName))
        return createUniqueFileName("", fileName, "", QFileInfo(fileName).suffix(), false);
    return fileName;
}

void getUniqueFromToValues(QString &fromString, QString &toString, double from, double to)
{
    int factor = 0; //10, 100, 1000...
    fromString.setNum(from, 'f', factor);
    toString.setNum(to, 'f', factor);

    if (from == to)
        return;

    while ((fromString == toString) && factor<=6) {
        factor++;
        fromString.setNum(from, 'f', factor);
        toString.setNum(to, 'f', factor);
    }
}

QString changeFileExt(const QString &fileName, const QString &ext)
{
    QFileInfo fi(fileName);
    if (fi.exists())
        return fi.canonicalPath()+"/"+fi.completeBaseName()+"."+ext;

    QString result = fileName;
    result.chop(3);
    result.append(ext);
    return result;
}



QVector<double> absolutes(const QVector<cx_double> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = std::abs(values.at(i));
    }

    return result;
}

QVector<double> absolutes(const QVector<double> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = std::abs(values.at(i));
    }

    return result;
}

QVector<double> phases(const QVector<cx_double> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = std::arg(values.at(i));
    }

    return result;
}

QVector<double> reals(const QVector<cx_double> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = values.at(i).real();
    }

    return result;
}




QVector<double> linspace(double begin, double step, int n)
{
    QVector<double> result(n);
    for (int i=0; i<n; ++i) result[i]=begin+i*step;
    return result;
}

QVector<double> imags(const QVector<cx_double> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = values.at(i).imag();
    }

    return result;
}

QVector<cx_double> movingAverage(const QVector<cx_double> &spectrum, int window)
{
    const int numInd = spectrum.size();

    // 1. Извлекаем амплитуды и фазы из комплексного спектра
    QVector<double> amplitudes = absolutes(spectrum);
    QVector<double> phase = phases(spectrum);

    // 2. Сглаживаем получившиеся амплитуды
    amplitudes = movingAverage(amplitudes, window);

    // 3. Слепляем вместе амплитуды и фазы в новый комплекснозначный вектор
    QVector<cx_double> result(numInd, cx_double());
    for(int i=0; i<numInd; ++i)
        result[i] = std::polar(amplitudes.at(i), phase.at(i));

    return result;
}

QVector<double> movingAverage(const QVector<double> &spectrum, int window)
{
    int numInd = spectrum.size();
    QVector<double> result(numInd, 0.0);

    int span = window / 2;

    for (int j=span; j<numInd-span; ++j) {
        double sum = 0.0;
        for (int k=0; k<window;++k) {
            sum += spectrum.at(j-span+k);
        }
        sum /= window;

        result[j] = sum;
    }

    //начало диапазона и конец диапазона
    for (int j=0; j<span; ++j)
        result[j] = spectrum.at(j);
    for (int j=numInd-span; j<numInd; ++j)
        result[j] = spectrum.at(j);

    return result;
}


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

template <typename TFloat, typename TInt, QDataStream::ByteOrder ENDIAN>
TFloat toFloat(const QByteArray &v, size_t offset)
{
    if(offset > v.size() - sizeof(TInt)) {
        qDebug()<<"toFloat() - offset is out of range. Returning 0.";
        return 0.0;
    }

    union {
        TInt   i;
        TFloat f;
    } tmp{};
    ::memcpy(&tmp, v.data() + offset, sizeof(TInt));

    if(ENDIAN != QDataStream::ByteOrder(QSysInfo::ByteOrder))
        tmp.i = qToLittleEndian(tmp.i);

    return tmp.f;
}

double toFloat64LE(const QByteArray &v, size_t offset)
{
  return toFloat<double, quint64, QDataStream::LittleEndian>(v, offset);
}

float toFloat32LE(const QByteArray &v, size_t offset)
{
    return toFloat<float, quint32, QDataStream::LittleEndian>(v, offset);
}

QVector<cx_double> complexes(const QVector<double> &values, bool valuesAreReals)
{
    const int size = values.size();
    QVector<cx_double> result(size);

    for (int i=0; i<size; ++i) {
        if (valuesAreReals)
            result[i] = {values.at(i), 0.0};
        else
            result[i] = {0.0, values.at(i)};
    }

    return result;
}

QVector<cx_double> complexes(const QVector<float> &values, bool valuesAreReals)
{
    const int size = values.size();
    QVector<cx_double> result(size);

    for (int i=0; i<size; ++i) {
        if (valuesAreReals)
            result[i] = {values.at(i), 0.0};
        else
            result[i] = {0.0, values.at(i)};
    }

    return result;
}

QString stripHtml(QString s)
{
    s.remove(QRegularExpression("<[^>]*>"));
    return s;
}

void processDir(const QString &file, QStringList &files, bool includeSubfolders)
{DD;
    if (QFileInfo(file).isDir()) {
        QFileInfoList dirLst = QDir(file).entryInfoList(FormatFactory::allSuffixes(),
                                                        QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
                                                        QDir::DirsFirst);
        for (int i=0; i<dirLst.count(); ++i) {
            if (dirLst.at(i).isDir()) {
                if (includeSubfolders)
                    processDir(dirLst.at(i).absoluteFilePath(),files,includeSubfolders);
            }
            else
                files << dirLst.at(i).absoluteFilePath();
        }
    }
    else {
        files << file;
    }
    files.removeDuplicates();
}

QDateTime dateTimeFromString(QString s)
{
    s = s.trimmed();

    QDateTime dt;
    if (s.isEmpty()) return dt;
    dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid()) dt = QDateTime::fromString(s, "dd.MM.yyyy hh:mm:ss");
    if (!dt.isValid()) dt = QDateTime::fromString(s, "dd.MM.yyyy hh:mm");
    if (!dt.isValid()) dt = QDateTime::fromString(s, "dd.MM.yy hh:mm:ss");
    if (!dt.isValid()) dt = QDateTime::fromString(s, "dd-MMM-yy hh:mm:ss");
    if (!dt.isValid()) {
        QLocale l = QLocale::c();
        dt = l.toDateTime(s, "dd-MMM-yy hh:mm:ss");
    }
    if (dt.date().year()<1950) dt = dt.addYears(100);
    return dt;
}

QDateTime dateTimeFromString(QString date, QString time)
{
    //date
    QDate d;
    date = date.trimmed();
    d = QDate::fromString(date, Qt::ISODateWithMs);
    if (!d.isValid()) d = QDate::fromString(date, "dd.MM.yyyy");
    if (!d.isValid()) d = QDate::fromString(date, "dd.MM.yy");
    if (!d.isValid()) d = QDate::fromString(date, "dd-MMM-yy");
    if (!d.isValid()) {
        QLocale l = QLocale::c();
        d = l.toDate(date, "dd-MMM-yy");
    }
    if (d.year()<1950) d = d.addYears(100);

    //time
    QTime t;
    t = QTime::fromString(time, Qt::ISODateWithMs);
    if (!t.isValid()) t = QTime::fromString(time, "hh:mm:ss");
    if (!t.isValid()) t = QTime::fromString(time, "h:mm:ss");
    if (!t.isValid()) t = QTime::fromString(time, "hh:mm");
    if (!t.isValid()) {
        QLocale l = QLocale::c();
        t = l.toTime(time, "hh:mm:ss");
    }
    return QDateTime(d,t);
}
