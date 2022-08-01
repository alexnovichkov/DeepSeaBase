#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>
#include <vector>
#include <complex>
typedef std::complex<double> cx_double;

class Channel;
class DataHolder;

QDebug operator <<(QDebug debug, const std::complex<double> &val);

template <typename T>
class asKeyValueRange
{
public:
    asKeyValueRange(T& data) : m_data{data} {}
    auto begin() {return m_data.constKeyValueBegin();}
    auto end() {return m_data.constKeyValueEnd();}
private:
    T &m_data;
};

template<typename InputIterator, typename ValueType>
InputIterator closest(InputIterator first, InputIterator last, ValueType value)
{
    return std::min_element(first, last, [&](ValueType x, ValueType y)
    {
        return std::abs(x - value) < std::abs(y - value);
    });
}

double closest(double begin, double step, double value);
double closest(Channel *c, double val, bool xAxis = true, int delta = 0);

template<typename T>
QVector<T> segment(const QVector<T> &values, int from, int to, int blockSize, int blocks)
{
    QVector<T> result;
    if (values.isEmpty()) return result;
    if (blockSize == 0) return result;
    if (blocks <= 0) return result;

    if (from > to) std::swap(from, to);
    if (from < 0) from = 0;
    if (to >= blockSize) to = blockSize-1;

    const int length = to-from+1;

    try {
        result.reserve(length*blocks);
        for (int i=0; i<blocks; ++i) {
            auto d = values.mid(i*blockSize + from, length);
            d.resize(length);
            result.append(d);
        }
    } catch (const std::bad_alloc &bad) {
        qDebug()<<"could not allocate"<<length*blocks<<"elements";
    }
    return result;
}

QString smartDouble(double v);

template <typename T, typename D>
QVector<D> readChunk(QDataStream &readStream, quint64 blockSize, qint64 *actuallyRead)
{
    //qDebug()<<"trying to allocate"<<blockSize<<"elements";
    QVector<D> result;
    try {
        result.resize(blockSize);
        //qDebug()<<"allocated";

        T v;

        if (actuallyRead) *actuallyRead = 0;

        for (quint64 i=0; i<blockSize; ++i) {
            if (readStream.atEnd()) {
                break;
            }

            readStream >> v;
            result[i] = D(v);
            if (actuallyRead) (*actuallyRead)++;
        }
    } catch (const std::bad_alloc &bad) {
        qDebug()<<"could not allocate"<<blockSize<<"elements";
        if (actuallyRead) *actuallyRead = 0;
    }

    return result;
}

template <typename D>
QVector<D> getChunkOfData(QDataStream &readStream, quint64 chunkSize, uint IndType, qint64 *actuallyRead=0)
{
    QVector<D> result;

    switch (IndType) {
        case 0x00000001: {
            result = readChunk<quint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000001: {
            result = readChunk<qint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000002: {
            result = readChunk<quint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000002: {
            result = readChunk<qint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x00000004: {
            result = readChunk<quint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000004: {
            result = readChunk<qint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0x80000008: {
            result = readChunk<qint64,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000004: {
            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            result = readChunk<float,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case 0xC0000008: //плавающий 64 бита
        case 0xC000000A: //плавающий 80 бит
            readStream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            result = readChunk<double,D>(readStream, chunkSize, actuallyRead);
            break;
        default: break;
    }

    return result;
}

template <typename T>
inline T convertFrom(unsigned char *ptr, uint IndType)
{
    switch (IndType) {
        case 1:          return static_cast<T>(qFromLittleEndian<quint8>(ptr)); break;
        case 0x80000001: return static_cast<T>(qFromLittleEndian<qint8>(ptr)); break;
        case 2:          return static_cast<T>(qFromLittleEndian<quint16>(ptr)); break;
        case 0x80000002: return static_cast<T>(qFromLittleEndian<qint16>(ptr)); break;
        case 4:          return static_cast<T>(qFromLittleEndian<quint32>(ptr)); break;
        case 0x80000004: return static_cast<T>(qFromLittleEndian<qint32>(ptr)); break;
        case 0x80000008: return static_cast<T>(qFromLittleEndian<qint64>(ptr)); break;
        case 0xc0000004: return static_cast<T>(qFromLittleEndian<float>(ptr)); break;
        case 0xc0000008: return static_cast<T>(qFromLittleEndian<double>(ptr)); break;
        case 0xc000000a: return static_cast<T>(qFromLittleEndian<long double>(ptr)); break;
        default: break;
    }
    return T();
}

template <typename T>
inline QVector<T> convertFrom(unsigned char *ptr, quint64 length, uint IndType)
{
    uint step = IndType % 16;
    QVector<T> temp(length / step, 0.0);

    int i=0;
    while (length) {
        switch (IndType) {
            case 1: temp[i++] = static_cast<T>(qFromLittleEndian<quint8>(ptr)); break;
            case 0x80000001: temp[i++] = static_cast<T>(qFromLittleEndian<qint8>(ptr)); break;
            case 2: temp[i++] = static_cast<T>(qFromLittleEndian<quint16>(ptr)); break;
            case 0x80000002: temp[i++] = static_cast<T>(qFromLittleEndian<qint16>(ptr)); break;
            case 4: temp[i++] = static_cast<T>(qFromLittleEndian<quint32>(ptr)); break;
            case 0x80000004: temp[i++] = static_cast<T>(qFromLittleEndian<qint32>(ptr)); break;
            case 0x80000008: temp[i++] = static_cast<T>(qFromLittleEndian<qint64>(ptr)); break;
            case 0xc0000004: temp[i++] = static_cast<T>(qFromLittleEndian<float>(ptr)); break;
            case 0xc0000008: temp[i++] = static_cast<T>(qFromLittleEndian<double>(ptr)); break;
            case 0xc000000a: temp[i++] = static_cast<T>(qFromLittleEndian<long double>(ptr)); break;
            default: break;
        }

        length -= step;
        ptr += step;
    }
    return temp;
}

QString replaceWinChars(QString s);


/* возвращает значение функции-члена function первого элемента последовательности,
 * если все элементы последовательности равны
 * или значение по умолчанию, если в последовательности есть разные элементы
 *
 * QList<Class* c> list; T Class::function();
 * T t = find_first_unique(list.cbegin(), list.cend(), Class::function, T defVal);
 * */

template <typename T>
QVector<double> absolutes(const QVector<T> &values)
{
    const int size = values.size();
    QVector<double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = std::abs(values.at(i));
    }

    return result;
}
// возвращает вектор фаз комплексных чисел
QVector<double> phases(const QVector<cx_double> &values);
// возвращает вектор действительных частей комплексных чисел
QVector<double> reals(const QVector<cx_double> &values);
// возвращает вектор действительных частей комплексных чисел
QVector<double> imags(const QVector<cx_double> &values);
// возвращает вектор комплексных чисел с действительными частями из values

template <typename T>
QVector<cx_double> complexesFromReals(const QVector<T> &values)
{
    const int size = values.size();
    QVector<cx_double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = {values.at(i), 0.0};
    }

    return result;
}
template <typename T>
QVector<cx_double> complexesFromImags(const QVector<T> &values)
{
    const int size = values.size();
    QVector<cx_double> result(size);

    for (int i=0; i<size; ++i) {
        result[i] = {0.0, values.at(i)};
    }

    return result;
}
template <typename T>
QVector<cx_double> complexesFromInterweaved(const QVector<T> &values)
{
    const int size = values.size();
    QVector<cx_double> result(size/2);

    for (int i=0; i<size/2; ++i) {
        result[i] = {values.at(i*2), values.at(i*2+1)};
    }

    return result;
}
template <typename T>
QVector<T> interweavedFromComplexes(const QVector<cx_double> &values)
{
    const int size = values.size();
    QVector<T> result(size*2);

    for (int i=0; i<size; ++i) {
        result[i*2] = values.at(i).real();
        result[i*2+1] = values.at(i).imag();
    }

    return result;
}

// возвращает вектор линейно возрастающих чисел
QVector<double> linspace(double begin, double step, int n);

/**
 * @brief thirdOctave - вычисляет нечто похожее на третьоктаву
 * @param spectrum - спектральные данные
 * @param xBegin - нижняя граница
 * @param xStep - шаг по частоте
 * @return два вектора - один с центральными частотами полос, другой - с уровнями в полосах
 */
QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep);

//template <typename T>
//QVector<T> movingAverage(const QVector<T> &spectrum, int window);

QVector<cx_double> movingAverage(const QVector<cx_double> &spectrum, int window);
QVector<double> movingAverage(const QVector<double> &spectrum, int window);

/**
 * @brief createUniqueFileName возвращает уникальное имя файла в виде
 *                             folderName/fileName_constantPart_000.ext
 * @param folderName - имя папки, в которой будет содержаться файл
 *        Если folderName пустой, то файл будет находиться в той же папке, что и fileName
 * @param fileName - имя файла, на основе которого нужно создать новое имя
 * @param constantPart - суффикс, который добавляется к fileName ("fileName_constantPart")
 * @param ext - расширение файла
 * @param justified - если true, то к имени добавляется суффикс "_001", если файл уже существует
 *                    если false, то к имени добавляется суффикс "(1)", если файл уже существует
 * @return folderName/fileName_constantPart_000.ext, folderName/fileName_constantPart_001.ext
 */
QString createUniqueFileName(const QString &folderName, const QString &fileName, QString constantPart,
                             const QString &ext, bool justified = false);
QString createUniqueFileName(const QString &fileName);
/**
 * @brief changeFileExt меняет расширение имени файла
 * @param fileName - исходное имя файла
 * @param ext - новое расширение (без точки)
 * @return - полное имя файла с путем и замененным расширением
 */
QString changeFileExt(const QString &fileName, const QString &ext);

// возвращает округленные значения переменных from и to, которые различаются как минимум в первой значащей цифре
void getUniqueFromToValues(QString &fromString, QString &toString, double from, double to);

QString stripHtml(QString s);

QString doubletohex(const double d);

double hextodouble(QString hex);


float hextofloat(QString hex);

QString floattohex(const float f);

float toFloat32LE(const QByteArray &v, size_t offset);
double toFloat64LE(const QByteArray &v, size_t offset);

void processDir(const QString &file, QStringList &files, bool includeSubfolders, const QStringList &filters);

QDateTime dateTimeFromString(QString s);
QDateTime dateTimeFromString(QString date, QString time);

bool channelsFromSameFile(const QVector<Channel *> &source);
QVector<int> channelIndexes(const QVector<Channel *> &source);

#endif // ALGORITHMS_H
