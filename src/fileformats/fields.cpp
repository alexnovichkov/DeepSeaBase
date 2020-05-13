#include "fields.h"

QDataStream &operator>>(QDataStream &stream, FieldDescription &field)
{
    int type;
    stream >> type;
    stream >> field.value;
    field.type = FieldType(type);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const FieldDescription &field)
{
    stream << int(field.type);
    stream << field.value;
    return stream;
}

void setType151(QVector<FieldDescription> &type151)
{//Uff header
    type151  = { // header
       {FTDelimiter, "-1"}, {FTEmpty, ""}, //0-1
       {FTInteger6, 151}, {FTEmpty, ""}, //2-3
       {FTString80, "NONE"}, {FTEmpty, ""},//4-5 model file name
       {FTString80, "NONE"}, {FTEmpty, ""}, //6-7 model file description
       {FTString80, "NONE"}, {FTEmpty, ""}, //8-9 program which created db
       {FTTimeDate, ""}, {FTEmpty, ""}, //10-11 date time database created DD.MM.YY HH:MM:SS
       {FTTimeDate, ""}, {FTEmpty, ""}, //12-13 date time database last saved DD.MM.YY HH:MM:SS
       {FTString80, "DeepSea Base"}, {FTEmpty, ""}, //14-15 program which created uff
       {FTTimeDate, ""}, {FTEmpty, ""}, //16-17 date time uff written DD.MM.YY HH:MM:SS
       {FTDelimiter, "-1"}, {FTEmpty, ""} //18-19
               };
}

void setType164(QVector<FieldDescription> &type164)
{//Uff units
    type164 = {
        {FTDelimiter, ""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 164}, {FTEmpty, ""}, //2-3
        {FTInteger10, 1}, {FTEmpty, ""}, //4-5 1=SI,
          //   length              force               temperature
        {FTFloat25_17, 1.0}, {FTFloat25_17, 1.0}, {FTFloat25_17, 1.0}, {FTEmpty, ""}, //6-9
          //   temperature offset
        {FTFloat25_17, 2.73160000000000030E+002}, {FTEmpty, ""}, //10-11
        {FTDelimiter, ""}, {FTEmpty, ""}, //12-13
    };
}

void setType1858(QVector<FieldDescription> &type1858)
{//Function header
    type1858  = {
        {FTDelimiter, ""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 1858}, {FTEmpty, ""}, //2-3
//1-я строчка
        {FTInteger12, 1}, //4 set record number
        {FTInteger12, 0}, //5 octave format, 0=not in octave format(default), 1=octave, n=1/n oct
        {FTInteger12, 0}, //6 measurement run number
        {FTInteger12, 0}, {FTInteger12, 0}, {FTInteger12, 0}, {FTEmpty, ""}, //7-10, not used
//2-я строчка
        {FTInteger6, 0}, //11 weighting type, 0=no, 1=A wei, 2=B wei, 3=C wei, 4=D wei
        {FTInteger6, 0}, //12 window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
                         //4=exponential, 5=impact, 6=impact and exponential
        {FTInteger6, 0}, //13 amplitude units, 0=unknown, 1=half-peak, 2=peak, 3=RMS
        {FTInteger6, 0}, //14 normalization method, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                               //3=Units squared seconds per Hz (ESD)
        {FTInteger6, 0}, //15  Abscissa Data Type Qualifier,
                          //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
        {FTInteger6, 0}, //16 Ordinate Numerator Data Type Qualifier, see 15
        {FTInteger6, 0}, //17 Ordinate Denominator Data Type Qualifier, see 15
        {FTInteger6, 0}, //18 Z-axis Data Type Qualifier, see 15
        {FTInteger6, 0}, //19 sampling type, 0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
        {FTInteger6, 0}, {FTInteger6, 0}, {FTInteger6, 0}, {FTEmpty, ""}, //20-23 not used
//3-я строчка
        {FTFloat15_7, 0.0}, //24 Z RPM value
        {FTFloat15_7, 0.0}, //25 Z time value
        {FTFloat15_7, 0.0}, //26 Z order value
        {FTFloat15_7, 0.0}, //27 number of samples
        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //28-29 not used
//4-я строчка
        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, //30-33 user values
        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //34-35 Exponential window damping factor
        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTEmpty, ""}, //36-41 not used
        {FTString10a,"NONE  NONE"}, //42 response direction, reference direction
        {FTEmpty, ""},//43
        {FTString80,"NONE"}, {FTEmpty, ""},//44-45 not used
        {FTDelimiter,""}, {FTEmpty, ""} //46-47
    };
}

void setType58(QVector<FieldDescription> &type58)
{//Function description
    type58 = {
        {FTDelimiter,""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 58}, {FTEmpty, ""}, //2-3
        {FTString80, "NONE"}, {FTEmpty, ""},//4-5 Function description, Точка 21/Сила/[(m/s2)/N]
        {FTString80, "NONE"}, {FTEmpty,""}, //6-7 ID line 2
        {FTTimeDate80, ""}, {FTEmpty,""}, //8-9 Time date of function creation
        {FTString80, "Record 1" }, {FTEmpty,""}, //10-11 ID line 4,
        {FTString80,"NONE" }, {FTEmpty,""},  //12-13 ID line 5

        /// 1-я строка
        {FTInteger5, 0}, //14 Function Type
    //                                       0 - General or Unknown
    //                                       1 - Time Response
    //                                       2 - Auto Spectrum
    //                                       3 - Cross Spectrum
    //                                       4 - Frequency Response Function
    //                                       5 - Transmissibility
    //                                       6 - Coherence
    //                                       7 - Auto Correlation
    //                                       8 - Cross Correlation
    //                                       9 - Power Spectral Density (PSD)
    //                                       10 - Energy Spectral Density (ESD)
    //                                       11 - Probability Density Function
    //                                       12 - Spectrum
    //                                       13 - Cumulative Frequency Distribution
    //                                       14 - Peaks Valley
    //                                       15 - Stress/Cycles
    //                                       16 - Strain/Cycles
    //                                       17 - Orbit
    //                                       18 - Mode Indicator Function
    //                                       19 - Force Pattern
    //                                       20 - Partial Power
    //                                       21 - Partial Coherence
    //                                       22 - Eigenvalue
    //                                       23 - Eigenvector
    //                                       24 - Shock Response Spectrum
    //                                       25 - Finite Impulse Response Filter
    //                                       26 - Multiple Coherence
    //                                       27 - Order Function

        {FTInteger10, 1}, //15 Function Identification Number
        {FTInteger5, 1}, //16 Version Number, or sequence number
        {FTInteger10, 0},//17 Load Case Identification Number
        {FTString10, "NONE"},//18  Response Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //19 Response Node
        {FTInteger4, 0},//20 Response Direction +Z
        {FTString10, "NONE"},//21 Reference Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //22 Reference Node
        {FTInteger4, 0}, //23 Reference Direction +Z
        {FTEmpty, ""}, //24

        /// 2-я строка
        {FTInteger10, 2}, //25 Ordinate Data Type
    //                                       2 - real, single precision
    //                                       4 - real, double precision
    //                                       5 - complex, single precision
    //                                       6 - complex, double precision
        {FTInteger10, 0}, //26   Number of data pairs for uneven abscissa spacing,
                             //  or number of data values for even abscissa spacing
        {FTInteger10, 1}, //27 Abscissa Spacing (1=even, 0=uneven,
        {FTFloat13_5, 0.0},//28 Abscissa minimum
        {FTFloat13_5, 0.0}, //29 Abscissa increment
        {FTFloat13_5, 0.0}, //30 Z-axis value
        {FTEmpty,""},  //31

        /// 3-я строка
        {FTInteger10, 1}, //32 Abscissa Data Characteristics
    //                                       0 - unknown
    //                                       1 - general
    //                                       2 - stress
    //                                       3 - strain
    //                                       5 - temperature
    //                                       6 - heat flux
    //                                       8 - displacement
    //                                       9 - reaction force
    //                                       11 - velocity
    //                                       12 - acceleration
    //                                       13 - excitation force
    //                                       15 - pressure
    //                                       16 - mass
    //                                       17 - time
    //                                       18 - frequency
    //                                       19 - rpm
    //                                       20 - order
        {FTInteger5, 0}, //33 Length units exponent
        {FTInteger5,0}, //34 Force units exponent
        {FTInteger5, 0}, //35 Temperature units exponent
        {FTString20, "NONE"}, //36 Axis label ("NONE" if not used)
        {FTString20, "NONE"}, //37 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //38

        /// 4-я строка
        {FTInteger10, 1}, //39 Ordinate (or ordinate numerator) Data Characteristics
        {FTInteger5,0}, //40 Length units exponent
        {FTInteger5,0}, //41 Force units exponent
        {FTInteger5, 0}, //42 Temperature units exponent
        {FTString20, "NONE"}, //43  Axis label ("NONE" if not used)
        {FTString20, "NONE"}, //44 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //45

        {FTInteger10, 0}, //46 Ordinate Denominator Data Characteristics
        {FTInteger5,0}, //47
        {FTInteger5,0}, //48
        {FTInteger5,0}, //49
        {FTString20, "NONE"}, //50
        {FTString20, "NONE"}, //51
        {FTEmpty,""},  //52

        {FTInteger10, 0}, //53 Z-axis Data Characteristics
        {FTInteger5, 0}, //54
        {FTInteger5, 0}, //55
        {FTInteger5, 0}, //56
        {FTString20, "NONE"}, //57
        {FTString20, "NONE"}, //58
        {FTEmpty,""} //59

    //                                    Data Values
    //                            Ordinate            Abscissa
    //                Case     Type     Precision     Spacing       Format
    //              -------------------------------------------------------------
    //                  1      real      single        even         6E13.5
    //                  2      real      single       uneven        6E13.5
    //                  3     complex    single        even         6E13.5
    //                  4     complex    single       uneven        6E13.5
    //                  5      real      double        even         4E20.12
    //                  6      real      double       uneven     2(E13.5,E20.12)
    //                  7     complex    double        even         4E20.12
    //                  8     complex    double       uneven      E13.5,2E20.12
    //              --------------------------------------------------------------

    };
}


