#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>

#include <complex>
typedef std::complex<double> cx_double;

QDebug operator <<(QDebug debug, const std::complex<double> &val);

/* возвращает значение функции-члена function первого элемента последовательности,
 * если все элементы последовательности равны
 * или значение по умолчанию, если в последовательности есть разные элементы
 *
 * QList<Class* c> list; T Class::function();
 * T t = find_first_unique(list.begin(), list.end(), Class::function, T defVal);
 * */

/**
 * @brief absolutes возвращает вектор модулей комплексных чисел
 * @param values вектор комплексных чисел
 * @return  вектор модулей комплексных чисел
 */
QVector<double> absolutes(const QVector<cx_double> &values);
QVector<double> absolutes(const QVector<double> &values);
// возвращает вектор фаз комплексных чисел
QVector<double> phases(const QVector<cx_double> &values);
// возвращает вектор действительных частей комплексных чисел
QVector<double> reals(const QVector<cx_double> &values);
// возвращает вектор действительных частей комплексных чисел
QVector<double> imags(const QVector<cx_double> &values);
// возвращает вектор комплексных чисел с действительными частями из values
QVector<cx_double> complexes(const QVector<float> &values, bool valuesAreReals = true);
QVector<cx_double> complexes(const QVector<double> &values, bool valuesAreReals = true);

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

int uffWindowType(int dfdWindowType);
int uffMethodFromDfdMethod(int methodId);

/**
 * @brief createUniqueFileName возвращает уникальное имя файла в виде
 *                             folderName/fileName_constantPart_000.ext
 * @param folderName - имя папки, в которой будет содержаться файл
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

QString stripHtml(const QString &s);

QString doubletohex(const double d);

double hextodouble(QString hex);


float hextofloat(QString hex);

QString floattohex(const float f);

#endif // ALGORITHMS_H
