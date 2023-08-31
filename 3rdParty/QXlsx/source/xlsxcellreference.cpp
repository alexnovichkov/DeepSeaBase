// xlsxcellreference.cpp

#include "xlsxcellreference.h"
#include <QStringList>
#include <QMap>

#include <QRegularExpression>

QT_BEGIN_NAMESPACE_XLSX

namespace {

int intPow(int x, int p)
{
  if (p == 0) return 1;
  if (p == 1) return x;

  int tmp = intPow(x, p/2);
  if (p%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}

QString col_to_name(int col_num)
{
    static thread_local QMap<int, QString> col_cache;

    auto it = col_cache.find(col_num);
    if (it == col_cache.end()) {
        QString col_str;
        int remainder;
        while (col_num) {
            remainder = col_num % 26;
            if (remainder == 0)
                remainder = 26;
            col_str.prepend(QChar('A'+remainder-1));
            col_num = (col_num - 1) / 26;
        }
        it = col_cache.insert(col_num, col_str);
    }

    return it.value();
}

int col_from_name(const QString &col_str)
{
    int col = 0;
    int expn = 0;
    for (int i=col_str.size()-1; i>-1; --i) {
        col += (col_str[i].unicode() - 'A' + 1) * intPow(26, expn);
        expn++;
    }

    return col;
}
} //namespace

CellReference::CellReference()
{
}

CellReference::CellReference(int row, int column)
    : _row(row), _column(column)
{
}

CellReference::CellReference(const QString &cell)
{
    init(cell);
}

CellReference::CellReference(const char *cell)
{
    init(QString::fromLatin1(cell));
}

void CellReference::init(const QString &cell_str)
{
    static thread_local QRegularExpression re(QStringLiteral("^\\$?([A-Z]{1,3})\\$?(\\d+)$"));
    QRegularExpressionMatch match = re.match(cell_str);
    if (match.hasMatch()) {
        const QString col_str = match.captured(1);
        const QString row_str = match.captured(2);
        _row = row_str.toInt();
        _column = col_from_name(col_str);
    }
}

CellReference::CellReference(const CellReference &other)
    : _row(other._row), _column(other._column)
{
}

CellReference::CellReference(int rowOffset, int columnOffset, const CellReference &other)
{
    _row = other._row + rowOffset;
    _column = other._column + columnOffset;
}

CellReference::~CellReference()
{
}

QString CellReference::toString(bool rowFixed, bool colFixed) const
{
    if (!isValid())
        return QString();

    QString cell_str;
    if (colFixed)
        cell_str.append(QLatin1Char('$'));
    cell_str.append(col_to_name(_column));
    if (rowFixed)
        cell_str.append(QLatin1Char('$'));
    cell_str.append(QString::number(_row));
    return cell_str;
}

bool CellReference::isValid() const
{
    return _row > 0 && _column > 0;
}

QT_END_NAMESPACE_XLSX
