#ifndef UNITSCONVERTER_H
#define UNITSCONVERTER_H

#include <QtCore>

namespace PhysicalUnits {

namespace Quantities {
class Quantity {
public:
    Quantity() {powers = QVector<int>(8,0);}
    virtual ~Quantity() {}
    virtual QString name() {return "?";}
    bool operator ==(const Quantity &other) {return (this->powers == other.powers);}
    Quantity operator *(const Quantity &other) {
        Quantity r;
        for(int i=0; i<8; ++i) r.powers[i] = this->powers[i]+other.powers[i];
        return r;
    }
    Quantity operator /(const Quantity &other) {
        Quantity r;
        for(int i=0; i<8; ++i) r.powers[i] = this->powers[i]/other.powers[i];
        return r;
    }
protected:
    QVector<int> powers;
};

class Length : public Quantity
{
public:
    Length() : Quantity() {powers[0]=1;}
    virtual QString name() {return "Length";}
};
class Mass : public Quantity
{
public:
    Mass() : Quantity() {powers[1]=1;}
    virtual QString name() {return "Mass";}
};
class Time : public Quantity
{
public:
    Time() : Quantity() {powers[2]=1;}
    virtual QString name() {return "Time";}
};
class Angle : public Quantity
{
public:
    Angle() : Quantity() {powers[3]=1;}
    virtual QString name() {return "Angle";}
};
class Temperature : public Quantity
{
public:
    Temperature() : Quantity() {powers[4]=1;}
    virtual QString name() {return "Temperature";}
};
class Current : public Quantity
{
public:
    Current() : Quantity() {powers[5]=1;}
    virtual QString name() {return "Electric Current";}
};
class LuminousIntensity : public Quantity
{
public:
    LuminousIntensity() : Quantity() {powers[6]=1;}
    virtual QString name() {return "Luminous Intensity";}
};
class Mole : public Quantity
{
public:
    Mole() : Quantity() {powers[7]=1;}
    virtual QString name() {return "Mole";}
};

// derived quantities
class Acceleration : public Quantity
{
public:
    Acceleration() : Quantity() {powers[0]=1; powers[2]=-2;}
    virtual QString name() {return "Acceleration";}
};
class AccelerationSquared : public Quantity
{
public:
    AccelerationSquared() : Quantity() {powers[0]=2; powers[2]=-4;}
    virtual QString name() {return "Acceleration^2";}
};
class AccelerationESD : public Quantity
{
public:
    AccelerationESD() : Quantity() {powers[0]=2; powers[2]=-2;}
    virtual QString name() {return "Acceleration ESD";}
};
class AccelerationForce : public Quantity
{
public:
    AccelerationForce() : Quantity() {powers[0]=2; powers[1]=1; powers[2]=-4;}
    virtual QString name() {return "Acceleration*Force";}
};
class AccelerationForcePSD : public Quantity
{
public:
    AccelerationForcePSD() : Quantity() {powers[0]=2; powers[1]=1; powers[2]=-3;}
    virtual QString name() {return "Acceleration*Force PSD";}
};
class AccelerationOverForce : public Quantity
{
public:
    AccelerationOverForce() : Quantity() {powers[1]=-1;}
    virtual QString name() {return "Acceleration over force";}
};
class AccelerationOverPressure : public Quantity
{
public:
    AccelerationOverPressure() : Quantity() {powers[0]=2; powers[1]=-1;}
    virtual QString name() {return "Acceleration over pressure";}
};
class AccelerationOverVoltage : public Quantity
{
public:
    AccelerationOverVoltage() : Quantity() {powers[0]=-1; powers[1]=-1; powers[2]=1; powers[5]=1;}
    virtual QString name() {return "Acceleration over voltage";}
};
class AccelerationPSD : public Quantity
{
public:
    AccelerationPSD() : Quantity() {powers[0]=2; powers[2]=-3;}
    virtual QString name() {return "Acceleration PSD";}
};
class AngleSquared : public Quantity
{
public:
    AngleSquared() : Quantity() {powers[3]=2;}
    virtual QString name() {return "Angle^2";}
};
class AngularAcceleration : public Quantity
{
public:
    AngularAcceleration() : Quantity() {powers[2]=-2; powers[3]=1;}
    virtual QString name() {return "Angular Acceleration";}
};
class AngularAccelerationSquared : public Quantity
{
public:
    AngularAccelerationSquared() : Quantity() {powers[2]=-4; powers[3]=2;}
    virtual QString name() {return "Angular Acceleration^2";}
};
class AngularDisplacement : public Angle
{
public:
    AngularDisplacement() : Angle() {}
    virtual QString name() {return "Angular Displacement";}
};
class AngularVelocity : public Quantity
{
public:
    AngularVelocity() : Quantity() {powers[2]=-1; powers[3]=1;}
    virtual QString name() {return "Angular Velocity";}
};
class AngularVelocitySquared : public Quantity
{
public:
    AngularVelocitySquared() : Quantity() {powers[2]=-2; powers[3]=2;}
    virtual QString name() {return "Angular Velocity^2";}
};
class Pressure : public Quantity
{
public:
    Pressure() : Quantity() {powers[0]=-1; powers[1]=1; powers[2]=-2;}
    virtual QString name() {return "Pressure";}
};
class Charge : public Quantity
{
public:
    Charge() : Quantity() {powers[2]=1; powers[5]=1;}
    virtual QString name() {return "Charge";}
};
class ChargeOverForce : public Quantity
{
public:
    ChargeOverForce() : Quantity() {powers[0]=-1; powers[1]=-1; powers[2]=3; powers[5]=1;}
    virtual QString name() {return "Charge Over Force";}
};
class Count : public Quantity
{
public:
    Count() : Quantity() {}
    virtual QString name() {return "Count";}
};


} // namespace Quantities

namespace Units {

template <typename Quantity>
class Unit
{
public:
    Unit() {}
    virtual ~Unit() {}

    virtual QString name() const {return "?";}
    virtual QString label() const {return "?";}

    virtual double scaleFactor() const {return 1.0;}
    virtual double scaleOffset() const {return 0.0;}

    // по умолчанию величина линейная, с коэффициентом 20:
    // dB = logFactor * lg (L/levelReference)
    // Если linear()==false, то величина - квадратичная, то есть
    // dB = logFactor * lg (L/levelReference^2)
    virtual double levelReference() const {return 1.0;}
    virtual bool linear() const {return true;}
    virtual double logFactor() const {return 20.0;}
protected:
    Quantity quantity;
};

//template <>
class Acceleration : public Unit<typename Quantities::Acceleration>
{
public:
    Acceleration() : Unit() {}
    virtual QString name() const {return "Acceleration";}
    virtual QString label() const {return "m/s^2";}

    virtual double scaleFactor() const {return 1.0;}
    virtual double scaleOffset() const {return 0.0;}

    // по умолчанию величина линейная, с коэффициентом 20:
    // dB = logFactor * lg (L/levelReference)
    // Если linear()==false, то величина - квадратичная, то есть
    // dB = logFactor * lg (L/levelReference^2)
    virtual double levelReference() const {return 1.0;}
    virtual bool linear() const {return true;}
    virtual double logFactor() const {return 20.0;}
private:
    Quantities::Acceleration quantity;
};

} // namespace Units



class Converter
{
public:
    enum Conversion {
        NoConversion = 0,
        Multiply,
        Divide,
        Square
    };
    Converter();
};
}

#endif // UNITSCONVERTER_H
