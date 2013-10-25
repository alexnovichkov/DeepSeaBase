#ifndef CONVERTERS_H
#define CONVERTERS_H

#include <QString>

QString doubletohex(const double d);

double hextodouble(QString hex);



float hextofloat(QString hex);

QString floattohex(const float f);

#endif // CONVERTERS_H
