#include "clipboarddata.h"

ClipboardData::ClipboardData()
{

}

void ClipboardData::write(int row, int col, const QString &val)
{
    if (!data.contains(row)) data.insert(row, {});
    auto &r = data[row];
    r.insert(col, val);
}

void ClipboardData::write(int row, int col, double val)
{
    if (!data.contains(row)) data.insert(row, {});
    auto &r = data[row];
    QString s = QString::number(val);
    s.replace('.', ',');
    r.insert(col, s);
}

QString ClipboardData::read(int row, int col) const
{
    return data.value(row).value(col);
}

QString ClipboardData::toString(QChar separator) const
{
    if (data.isEmpty()) return {};
    const auto &rowNumbers = data.keys();
    QStringList rows;
    for (int i = 0; i <= rowNumbers.last(); ++i) rows << QString();
    for (int rowNumber: rowNumbers) {
        const auto &colNumbers = data.value(rowNumber).keys();
        QStringList row;
        for (int i = 0; i <= colNumbers.last(); ++i) row << QString();
        for (int colNumber: colNumbers) row[colNumber] = data.value(rowNumber).value(colNumber);
        rows[rowNumber] = row.join(separator);
    }
    return rows.join('\n');
}
