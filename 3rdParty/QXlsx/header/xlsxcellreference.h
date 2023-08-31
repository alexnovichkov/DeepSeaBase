// xlsxcellreference.h

#ifndef QXLSX_XLSXCELLREFERENCE_H
#define QXLSX_XLSXCELLREFERENCE_H

#include <QtGlobal>

#include "xlsxglobal.h"

namespace QXlsx {

/**
 * @brief The CellReference class represents a reference to one cell location in a worksheet.
 * @note The numeration of rows and columns starts from 1.
 */
class QXLSX_EXPORT CellReference
{
public:
    /**
     * @brief creates an invalid CellReference.
     */
    CellReference();
    /**
        @brief creates a CellReference from the given @a row, and @a column.
    */
    CellReference(int row, int column);
    /**
     * @brief creats a CellReference from the given string representation of a cell location.
     * @param cell string like "A1" or "$A$1".
     * @note this constructor discards $s and does not understand relative locations like "R[-1]C2".
     */
    CellReference(const QString &cell);
    /**
     * @brief creats a CellReference from the given string representation of a cell location.
     * @param cell null-terminated string like "A1" or "$A$1".
     * @note This constructor discards $s and does not understands relative locations like "R[-1]C2".
     */
    CellReference(const char *cell);
    CellReference(const CellReference &other);
    /**
     * @brief creats a CellReference from the given cell location and offsets.
     * @param rowOffset row offset to be applied to the @a other row.
     * @param columnOffset column offset to be applied to the @a other column.
     * @param other other cell location.
     * @note If the resulting CellReference has non-positive row or column, it is invalid.
     */
    CellReference(int rowOffset, int columnOffset, const CellReference &other);
    ~CellReference();
    /**
     * @brief returns the string representation of a cell location.
     * @param rowFixed If true, the row number will be prepended with $.
     * @param colFixed If true, the column will be prepended with $.
     * @return The string representation of a cell location if the location is valid,
     * empty string otherwise.
     */
    QString toString(bool rowFixed=false, bool colFixed=false) const;
    /**
     * @brief parses @a cell and creates a CellReference object.
     * @param cell the string representation of a cell location.
     * @return The valid CellReference object if @a cell is successfully parsed and
     * its row and column are valid (> 0).
     * @note This method discards $s and does not understand relative locations like "R[-1]C2".
     */
    static CellReference fromString(const QString &cell);
    bool isValid() const;
    inline void setRow(int row) { _row = row; }
    inline void setColumn(int col) { _column = col; }
    inline int row() const { return _row; }
    inline int column() const { return _column; }
    /**
     * @brief applies the given offsets and returns the reference to the result.
     * @param rowOffset row offset to be applied to the row value.
     * @param columnOffset column offset to be applied to the column value.
     * @return reference to the resulting CellReference (the result cal be invalid.)
     */
    inline CellReference &offset(int rowOffset, int columnOffset) {
        _row += rowOffset; _column += columnOffset; return *this;
    }

    inline bool operator ==(const CellReference &other) const
    {
        return _row==other._row && _column==other._column;
    }
    inline bool operator !=(const CellReference &other) const
    {
        return _row!=other._row || _column!=other._column;
    }
private:
    void init(const QString &cell);
    int _row = 0, _column = 0;
};

}

Q_DECLARE_TYPEINFO(QXlsx::CellReference, Q_MOVABLE_TYPE);

#endif // QXLSX_XLSXCELLREFERENCE_H
