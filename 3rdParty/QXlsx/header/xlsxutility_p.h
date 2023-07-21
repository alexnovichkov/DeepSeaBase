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

//void parseAttributeBool(const QXmlStreamAttributes &a, const QLatin1String &name, bool &target);
void parseAttributeBool(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<bool> &target);
void parseAttributePercent(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<double> &target);
void parseAttributeInt(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<int> &target);
void parseAttributeInt(const QXmlStreamAttributes &a, const QLatin1String &name, int &target);


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
