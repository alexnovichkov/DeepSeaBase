#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>
#include <vector>
#include <complex>
#include "logging.h"
typedef std::complex<double> cx_double;

class Channel;
class DataHolder;

enum class DataPrecision {
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    LongDouble
};

void initRandomGenerator();
int getRandom(int min, int max);

DataPrecision fromDfdDataPrecision(uint precision);
uint toDfdDataPrecision(DataPrecision precision);

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
        LOG(ERROR)<<"could not allocate"<<length*blocks<<"elements";
    }
    return result;
}

QString smartDouble(double v, double tickDistance=0.0);

double rounded(double val);

template <typename T, typename D>
QVector<D> readChunk(QDataStream &readStream, quint64 blockSize, qint64 *actuallyRead)
{
    //LOG(DEBUG)<<"trying to allocate "<<blockSize<<" elements";
    QVector<D> result;
    try {
        result.resize(blockSize);
        T v;
        if (actuallyRead) *actuallyRead = 0;
        for (quint64 i=0; i<blockSize; ++i) {
            if (readStream.atEnd())
                break;
            readStream >> v;
            result[i] = D(v);
            if (actuallyRead) (*actuallyRead)++;
        }
    } catch (const std::bad_alloc &bad) {
        LOG(ERROR)<<"could not allocate"<<blockSize<<"elements";
        if (actuallyRead) *actuallyRead = 0;
    }

    return result;
}

template <typename T, typename D>
D readValue(QDataStream &readStream, qint64 *actuallyRead)
{
    //LOG(DEBUG)<<"trying to allocate "<<blockSize<<" elements";
    D result;

    T v;
    if (actuallyRead) *actuallyRead = 0;
    if (!readStream.atEnd()) {
        readStream >> v;
        result = D(v);
        if (actuallyRead) (*actuallyRead)++;
    }
    else
        if (actuallyRead) *actuallyRead = 0;

    return result;
}

template <typename D>
D getChunkOfData(QDataStream &readStream, DataPrecision precision, qint64 *actuallyRead=0)
{
    D result;

    switch (precision) {
        case DataPrecision::UInt8: {
            result = readValue<quint8,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Int8: {
            result = readValue<qint8,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::UInt16: {
            result = readValue<quint16,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Int16: {
            result = readValue<qint16,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::UInt32: {
            result = readValue<quint32,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Int32: {
            result = readValue<qint32,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::UInt64: {
            result = readValue<quint64,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Int64: {
            result = readValue<qint64,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Float: {
            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            result = readValue<float,D>(readStream, actuallyRead);
            break;
        }
        case DataPrecision::Double: //плавающий 64 бита
        case DataPrecision::LongDouble: //плавающий 80 бит
            readStream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            //не умею читать такие данные
            result = readValue<double, D>(readStream, actuallyRead);
            break;
        default: break;
    }

    return result;
}

template <typename D>
QVector<D> getChunkOfData(QDataStream &readStream, quint64 chunkSize, DataPrecision precision, qint64 *actuallyRead=0)
{
    QVector<D> result;

    switch (precision) {
        case DataPrecision::UInt8: {
            result = readChunk<quint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Int8: {
            result = readChunk<qint8,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::UInt16: {
            result = readChunk<quint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Int16: {
            result = readChunk<qint16,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::UInt32: {
            result = readChunk<quint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Int32: {
            result = readChunk<qint32,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::UInt64: {
            result = readChunk<quint64,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Int64: {
            result = readChunk<qint64,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Float: {
            readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            result = readChunk<float,D>(readStream, chunkSize, actuallyRead);
            break;
        }
        case DataPrecision::Double: //плавающий 64 бита
        case DataPrecision::LongDouble: //плавающий 80 бит
            readStream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            //не умею читать такие данные
            result = readChunk<double, D>(readStream, chunkSize, actuallyRead);
            break;
        default: break;
    }

    return result;
}

template <typename T>
inline T convertFrom(unsigned char *ptr, DataPrecision precision)
{
    switch (precision) {
        case DataPrecision::UInt8: return static_cast<T>(qFromLittleEndian<quint8>(ptr)); break;
        case DataPrecision::Int8: return static_cast<T>(qFromLittleEndian<qint8>(ptr)); break;
        case DataPrecision::UInt16: return static_cast<T>(qFromLittleEndian<quint16>(ptr)); break;
        case DataPrecision::Int16: return static_cast<T>(qFromLittleEndian<qint16>(ptr)); break;
        case DataPrecision::UInt32: return static_cast<T>(qFromLittleEndian<quint32>(ptr)); break;
        case DataPrecision::Int32: return static_cast<T>(qFromLittleEndian<qint32>(ptr)); break;
        case DataPrecision::UInt64: return static_cast<T>(qFromLittleEndian<quint64>(ptr)); break;
        case DataPrecision::Int64: return static_cast<T>(qFromLittleEndian<qint64>(ptr)); break;
        case DataPrecision::Float: return static_cast<T>(qFromLittleEndian<float>(ptr)); break;
        case DataPrecision::Double: return static_cast<T>(qFromLittleEndian<double>(ptr)); break;
        case DataPrecision::LongDouble: return static_cast<T>(qFromLittleEndian<long double>(ptr)); break;
        default: break;
    }
    return T();
}

template <typename T>
inline QVector<T> convertFrom(unsigned char *ptr, quint64 length, DataPrecision precision)
{
    uint step = 4;
    switch (precision) {
        case DataPrecision::UInt8:
        case DataPrecision::Int8: step=1; break;
        case DataPrecision::UInt16:
        case DataPrecision::Int16: step=2; break;
        case DataPrecision::UInt32:
        case DataPrecision::Float:
        case DataPrecision::Int32: step=4; break;
        case DataPrecision::UInt64:
        case DataPrecision::Int64:
        case DataPrecision::Double: step=8; break;
        case DataPrecision::LongDouble: step=10; break;
    }

    QVector<T> temp;
    try {
        temp.resize(length / step);

        int i=0;
        while (length) {
            switch (precision) {
                case DataPrecision::UInt8: temp[i++] = static_cast<T>(qFromLittleEndian<quint8>(ptr)); break;
                case DataPrecision::Int8: temp[i++] = static_cast<T>(qFromLittleEndian<qint8>(ptr)); break;
                case DataPrecision::UInt16: temp[i++] = static_cast<T>(qFromLittleEndian<quint16>(ptr)); break;
                case DataPrecision::Int16: temp[i++] = static_cast<T>(qFromLittleEndian<qint16>(ptr)); break;
                case DataPrecision::UInt32: temp[i++] = static_cast<T>(qFromLittleEndian<quint32>(ptr)); break;
                case DataPrecision::Int32: temp[i++] = static_cast<T>(qFromLittleEndian<qint32>(ptr)); break;
                case DataPrecision::UInt64: temp[i++] = static_cast<T>(qFromLittleEndian<quint64>(ptr)); break;
                case DataPrecision::Int64: temp[i++] = static_cast<T>(qFromLittleEndian<qint64>(ptr)); break;
                case DataPrecision::Float: temp[i++] = static_cast<T>(qFromLittleEndian<float>(ptr)); break;
                case DataPrecision::Double: temp[i++] = static_cast<T>(qFromLittleEndian<double>(ptr)); break;
                case DataPrecision::LongDouble: temp[i++] = static_cast<T>(qFromLittleEndian<long double>(ptr)); break;
                default: break;
            }

            length -= step;
            ptr += step;
        }
    }
    catch (const std::bad_alloc &bad) {
        LOG(ERROR)<<"could not allocate"<<(length / step)<<"elements";
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
QString createUniqueFileName(QString folderName, const QString &fileName, QString constantPart,
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
