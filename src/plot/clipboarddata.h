#ifndef CLIPBOARDDATA_H
#define CLIPBOARDDATA_H

#include <QString>
#include <QMap>

class ClipboardData
{
public:
    ClipboardData();
    void write(int row, int col, const QString &val);
    void write(int row, int col, double val);
    QString read(int row, int col) const;
    /**
     * @brief преобразует данные в строку:
     * ряды разделяются переносом строки, ячейки разделяются разделителем #separator
     * @return
     */
    QString toString(QChar separator = ';') const;
private:
    QMap<int, QMap<int, QString>> data;
};

#endif // CLIPBOARDDATA_H
