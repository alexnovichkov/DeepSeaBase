// xlsxutility_p.h

#ifndef XLSXUTILITY_H
#define XLSXUTILITY_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QVariant>
#include <QXmlStreamAttributes>

#include "xlsxglobal.h"
#include <algorithm>
#include <type_traits>
#include <string>

QT_BEGIN_NAMESPACE_XLSX

class CellReference;

/**
 * @brief fromST_Percent parses QStringref val and returns percents
 * @param val - either a string "23.5%" or a string "23500"
 * @return
 */
double fromST_Percent(QStringRef val);
bool fromST_Boolean(QStringRef val);
QString toST_Boolean(bool val);
QString toST_Percent(double val);

#define SERIALIZE_ENUM(ENUM_TYPE,...) \
    inline void toString(const ENUM_TYPE &e, QString &s) const { \
        static_assert(std::is_enum<ENUM_TYPE>(), #ENUM_TYPE " must be an enum!"); \
        static const std::pair<ENUM_TYPE, QString> m[] = __VA_ARGS__; \
        auto it = std::find_if(std::begin(m), std::end(m), \
                  [e](const std::pair<ENUM_TYPE, QString> &pair) -> bool \
                  { return pair.first == e;}); \
        s = (it != std::end(m)) ? it->second : std::begin(m)->second; \
    } \
    inline void fromString(const QString &s, ENUM_TYPE &e) const { \
        static_assert(std::is_enum<ENUM_TYPE>(), #ENUM_TYPE " must be an enum!"); \
        static const std::pair<ENUM_TYPE, QString> m[] = __VA_ARGS__; \
        auto it = std::find_if(std::begin(m), std::end(m), \
                  [s](const std::pair<ENUM_TYPE, QString> &pair) -> bool \
                  { return pair.second == s;}); \
        e = (it != std::end(m)) ? it->first : std::begin(m)->first; \
    }

//void parseAttributeBool(const QXmlStreamAttributes &a, const QLatin1String &name, bool &target);
void parseAttributeBool(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<bool> &target);
void parseAttributePercent(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<double> &target);
void parseAttributeInt(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<int> &target);
void parseAttributeInt(const QXmlStreamAttributes &a, const QLatin1String &name, int &target);
void parseAttributeUInt(const QXmlStreamAttributes &a, const QLatin1String &name, uint &target);
void parseAttributeString(const QXmlStreamAttributes &a, const QLatin1String &name, QString &target);

bool parseXsdBoolean(const QString &value, bool defaultValue=false);

QStringList splitPath(const QString &path);
QString getRelFilePath(const QString &filePath);

double datetimeToNumber(const QDateTime &dt, bool is1904=false);
QVariant datetimeFromNumber(double num, bool is1904=false);
double timeToNumber(const QTime &t);

QString createSafeSheetName(const QString &nameProposal);
QString escapeSheetName(const QString &sheetName);
QString unescapeSheetName(const QString &sheetName);

bool isSpaceReserveNeeded(const QString &string);

QString convertSharedFormula(const QString &rootFormula, const CellReference &rootCell, const CellReference &cell);

QT_END_NAMESPACE_XLSX
#endif // XLSXUTILITY_H
