#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <QtCore>

/**
 * @brief thirdOctave - вычисляет нечто похожее на третьоктаву
 * @param spectrum - спектральные данные
 * @param xBegin - нижняя граница
 * @param xStep - шаг по частоте
 * @return два вектора - один с центральными частотами полос, другой - с уровнями в полосах
 */
QPair<QVector<double>, QVector<double> > thirdOctave(const QVector<double> &spectrum, double xBegin, double xStep);



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

#endif // ALGORITHMS_H
