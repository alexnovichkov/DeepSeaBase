#include "unitsconverter.h"

PhysicalUnits::Units::Type PhysicalUnits::Units::unitType(QString unit)
{
    unit = unit.toLower();
    if (unit == "hz" || unit == "гц" || unit == "1/s" || unit == "1/с") return Type::Frequency;
    if (unit == "s" || unit == "с") return Type::Time;
    if (unit == "m/s" || unit == "м/с") return Type::Velocity;
    if (unit == "m/s2" || unit == "m/s^2" || unit == "м/с2"
        || unit == "м/с^2" || unit == "g" || unit == "м/с**2"
        || unit == "m/s**2") return Type::Acceleration;
    if (unit == "n" || unit == "н") return Type::ExcitationForce;
    if (unit == "pa" || unit == "psi" || unit == "па") return Type::Pressure;
    if (unit == "m" || unit == "м" || unit == "cm"
        || unit == "mm" || unit == "см" || unit == "мм") return Type::Displacement;
    if (unit == "kg" || unit == "кг" || unit=="gram" || unit=="г") return Type::Mass;
    if (unit == "rpm" || unit == "rad/s" || unit == "deg/s" || unit == "°/s" || unit == "рад/с" || unit == "°/с"
            || unit == "об/мин" || unit == "об/с") return Type::RPM;
    if (unit == "1/deg" || unit == "1/rad" || unit == "1/рад") return Type::Order;
    if (unit == "w/m^2" || unit == "w/m2" || unit == "вт/м^2" || unit == "вт/м2") return Type::HeatFlux;
    if (unit == "k" || unit == "°c" || unit == "°f") return Type::Temperature;
    if (unit == "a" || unit == "а") return Type::Current;
    if (unit=="v" || unit=="в" || unit=="мв" || unit=="mv") return Type::Voltage;

    return Type::Unknown; //0 - unknown
    //2 - stress - not detectable (= pressure)
    //3 - strain - dimensionless
    //9 - reaction force - non detectable (= force)
}

bool PhysicalUnits::Units::unitsAreSame(const QString &u1, const QString u2)
{
    return unitType(u1) == unitType(u2);
}

QString PhysicalUnits::Units::unitDescription(QString unit)
{
    return unitDescription(unitType(unit));
}

double PhysicalUnits::Units::logref(const QString &name)
{
    return logref(unitType(name));
}

double PhysicalUnits::Units::logref(PhysicalUnits::Units::Type type)
{
    switch (type) {
        case Type::Stress: return 2.0e-5;
        case Type::Strain: return 1.0;
        case Type::Temperature: return 1.0;
        case Type::HeatFlux: return 1.0e-12;
        case Type::Displacement: return 8.0e-14;
        case Type::ReactionForce: return 1.0;
        case Type::Velocity: return 5.0e-8;
        case Type::Acceleration: return 3.16e-4;
        case Type::ExcitationForce: return 1.0;
        case Type::Pressure: return 2.0e-5;
        case Type::Mass: return 1.0;
        case Type::Time: return 1.0;
        case Type::Frequency: return 1.0;
        case Type::RPM: return 5.0e-8;
        case Type::Order: return 1.0;
        case Type::Voltage: return 1e-6;
        case Type::Current: return 1e-9;
        case Type::Unknown:
        case Type::General:
            break;
    }
    return 1.0;
}

double PhysicalUnits::Units::convertFactor(const QString &from)
{
    if (from.compare("g", Qt::CaseInsensitive)) return 9.81;
    if (from.compare("n", Qt::CaseInsensitive)) return 1.0;

    return 1.0;
}

QString PhysicalUnits::Units::unit(PhysicalUnits::Units::Type type)
{
    switch (type) {
        case Type::Stress:
        case Type::Pressure: return "Па";
        case Type::Temperature:  return "°C";
        case Type::HeatFlux:  return "Вт/м^2";
        case Type::Displacement:  return "м";
        case Type::ReactionForce:
        case Type::ExcitationForce: return "Н";
        case Type::Velocity: return "м/с";
        case Type::Acceleration: return "м/с^2";
        case Type::Mass: return "кг";
        case Type::Time: return "с";
        case Type::Frequency: return "Гц";
        case Type::RPM: return "об/мин";
        case Type::Order: return "1/рад";
        case Type::Voltage: return "В";
        case Type::Current: return "А";
        case Type::Unknown:
        case Type::General:
        case Type::Strain:
            break;
    }
    return "";
}

QString PhysicalUnits::Units::unitDescription(PhysicalUnits::Units::Type type)
{
    switch (type) {
        case Type::Unknown: return "Unknown";
        case Type::General: return "General";
        case Type::Stress: return "Stress";
        case Type::Strain: return "Strain";
        case Type::Temperature: return "Temperature";
        case Type::HeatFlux: return "Heat flux";
        case Type::Displacement: return "Displacement";
        case Type::ReactionForce: return "Reaction force";
        case Type::Velocity: return "Velocity";
        case Type::Acceleration: return "Acceleration";
        case Type::ExcitationForce: return "Excitation force";
        case Type::Pressure: return "Pressure";
        case Type::Mass: return "Mass";
        case Type::Time: return "Time";
        case Type::Frequency: return "Frequency";
        case Type::RPM: return "RPM";
        case Type::Order: return "Order";
        case Type::Voltage: return "Voltage";
        case Type::Current: return "Current";
    }
    return "Unknown";
}
