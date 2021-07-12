#include "weighting.h"

Weighting::Type Weighting::fromString(QString s)
{
    s = s.toLower();
    if (s == "a") return Type::A;
    if (s == "b") return Type::B;
    if (s == "c") return Type::C;
    if (s == "d") return Type::D;
    return Type::No;
}


QString Weighting::toString(Weighting::Type type)
{
    switch (type) {
        case Type::No: return "no";
        case Type::A: return "A";
        case Type::B: return "B";
        case Type::C: return "C";
        case Type::D: return "D";
    }
}
