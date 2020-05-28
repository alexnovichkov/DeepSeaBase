#include "uffdescriptor.h"

#include "strtk.hpp"
#include <QtDebug>

class Type151
{
public:
    int type = 151;
    std::string modelFileName = "NONE";
    std::string modelFileDescription = "NONE";
    std::string programDatabaseCreated = "NONE"; //8-9 program which created db
    std::string dateTimeCreated = "NONE"; //10-11 date time database created DD.MM.YY HH:MM:SS
    std::string dateTimeSaved = "NONE"; //12-13 date time database last saved DD.MM.YY HH:MM:SS
    std::string programFileCreated = "DeepSea Database"; //14-15 program which created uff
    std::string dateTimeFileSaved = "NONE"; //16-17 date time uff written DD.MM.YY HH:MM:SS
};

class Type164
{
public:
    int type = 164;
    int unitCode = 1; //4-5  = 1 - SI: Meter (newton)
                      //     = 2 - BG: Foot (pound f)
                      //     = 3 - MG: Meter (kilogram f)
                      //     = 4 - BA: Foot (poundal)
                      //     = 5 - MM: mm (milli newton)
                      //     = 6 - CM: cm (centi newton)
                      //     = 7 - IN: Inch (pound f)
                      //     = 8 - GM: mm (kilogram f)
                      //     = 9 - US: USER_DEFINED
    double lengthFactor = 1.0;
    double forceFactor = 1.0;
    double temperatureFactor = 1.0;
    double temperatureOffset = 2.73160000000000030E+002; //10-11
};

class Type1858
{
public:
    int type = 1858;
    //1-я строчка
    int setRecordNumber = 1;
    int octaveFormat = 0; //0=not in octave format(default), 1=octave, n=1/n oct
    int measurementRunNumber = 0;
    int notUsed1 = 0;
    int notUsed2 = 0;
    int notUsed3 = 0;
    //2-я строчка
    int weightingType = 0; //11 weighting type, 0=no, 1=A wei, 2=B wei, 3=C wei, 4=D wei
    int windowType = 0; // 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
                        // 4=exponential, 5=impact, 6=impact and exponential
    int amplitudeUnits = 0;// 0=unknown, 1=half-peak, 2=peak, 3=RMS
    int normalizationMethod = 0;// 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                                // 3=Units squared seconds per Hz (ESD)
    int abscissaDataTypeQualifier = 0; //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    int ordinate1DataTypeQualifier = 0; //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    int ordinate2DataTypeQualifier = 0; //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    int applicateDataTypeQualifier = 0; //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
    int samplingType = 0; //0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
    int notUsed4 = 0;
    int notUsed5 = 0;
    int notUsed6 = 0;
    //3-я строчка
    double zRPMValue = 0.0; //24 Z RPM value
    double zTimeValue = 0.0; //25 Z time value
    double zOrderValue = 0.0; //26 Z order value
    double numberOfSamples = 0.0; //27 number of samples
    double notUsed7 = 0.0;
    //4-я строчка
    double userValue1 = 0.0;
    double userValue2 = 0.0;
    double userValue3 = 0.0;
    double userValue4 = 0.0;
    double exponentialWindowDamping = 0.0; //34-35 Exponential window damping factor
    //5-я строчка
    double notUsed8 = 0.0;
    double notUsed9 = 0.0;
    double notUsed10 = 0.0;
    double notUsed11 = 0.0;
    double notUsed12 = 0.0;
    std::string responseDirection = "NONE";
    std::string referenceDirection = "NONE";
    std::string notUsed13 = "NONE";
};

class Type58
{
public:
    int type = 58;
    std::string functionDescription = "NONE"; // Точка 21/Сила/[(m/s2)/N]
    std::string idLine2 = "NONE";
    std::string dateTimeCreated = "     NONE     NONE";
    std::string idLine4 = "Record 1";
    std::string idLine5 = "NONE";
    // 6-я строка
    int functionType = 0;   // 0 - General or Unknown
                            // 1 - Time Response
                            // 2 - Auto Spectrum
                            // 3 - Cross Spectrum
                            // 4 - Frequency Response Function
                            // 5 - Transmissibility
                            // 6 - Coherence
                            // 7 - Auto Correlation
                            // 8 - Cross Correlation
                            // 9 - Power Spectral Density (PSD)
                            // 10 - Energy Spectral Density (ESD)
                            // 11 - Probability Density Function
                            // 12 - Spectrum
                            // 13 - Cumulative Frequency Distribution
                            // 14 - Peaks Valley
                            // 15 - Stress/Cycles
                            // 16 - Strain/Cycles
                            // 17 - Orbit
                            // 18 - Mode Indicator Function
                            // 19 - Force Pattern
                            // 20 - Partial Power
                            // 21 - Partial Coherence
                            // 22 - Eigenvalue
                            // 23 - Eigenvector
                            // 24 - Shock Response Spectrum
                            // 25 - Finite Impulse Response Filter
                            // 26 - Multiple Coherence
                            // 27 - Order Function
    int functionIDNumber = 1;
    int sequenceNumber = 1;
    int loadCaseNumber = 0;
    std::string responseEntityName = "NONE";
    int responseNode = 0;
    int responseDirection = 0;  // 0 - Scalar
                                // 1 - +X Translation       4 - +X Rotation
                                //-1 - -X Translation      -4 - -X Rotation
                                // 2 - +Y Translation       5 - +Y Rotation
                                //-2 - -Y Translation      -5 - -Y Rotation
                                // 3 - +Z Translation       6 - +Z Rotation
                                //-3 - -Z Translation      -6 - -Z Rotation
    std::string referenceEntityName = "NONE";
    int referenceNode = 0;
    int referenceDirection = 0;  // 0 - Scalar
                                // 1 - +X Translation       4 - +X Rotation
                                //-1 - -X Translation      -4 - -X Rotation
                                // 2 - +Y Translation       5 - +Y Rotation
                                //-2 - -Y Translation      -5 - -Y Rotation
                                // 3 - +Z Translation       6 - +Z Rotation
                                //-3 - -Z Translation      -6 - -Z Rotation
    // 7-я строка
    int ordinateDataFormat = 4; // 2 - real, single precision
                                // 4 - real, double precision
                                // 5 - complex, single precision
                                // 6 - complex, double precision
    int samplesCount = 0; // Number of data pairs for uneven abscissa spacing,
                          // or number of data values for even abscissa spacing
    int abscissaSpacing = 1; // 1=even, 0=uneven
    double xMin = 0.0;
    double xStep = 0.0;
    double zAxisValue = 0.0;
    // 8-я строка
    int xDataType = 1;  // 0 - unknown
                        // 1 - general
                        // 2 - stress
                        // 3 - strain
                        // 5 - temperature
                        // 6 - heat flux
                        // 8 - displacement
                        // 9 - reaction force
                        // 11 - velocity
                        // 12 - acceleration
                        // 13 - excitation force
                        // 15 - pressure
                        // 16 - mass
                        // 17 - time
                        // 18 - frequency
                        // 19 - rpm
                        // 20 - order
    int xLengthUnitsExponent = 0;
    int xForceUnitsExponent = 0;
    int xTemperatureUnitsExponent = 0;
    std::string xLabel = "NONE";
    std::string xUnitsLabel = "NONE";
    // 9-я строка
    int yDataType = 1;  // 0 - unknown
                        // 1 - general
                        // 2 - stress
                        // 3 - strain
                        // 5 - temperature
                        // 6 - heat flux
                        // 8 - displacement
                        // 9 - reaction force
                        // 11 - velocity
                        // 12 - acceleration
                        // 13 - excitation force
                        // 15 - pressure
                        // 16 - mass
                        // 17 - time
                        // 18 - frequency
                        // 19 - rpm
                        // 20 - order
    int yLengthUnitsExponent = 0;
    int yForceUnitsExponent = 0;
    int yTemperatureUnitsExponent = 0;
    std::string yLabel = "NONE";
    std::string yUnitsLabel = "NONE";
    //10-я строка
    int ydDataType = 1;  // 0 - unknown
                        // 1 - general
                        // 2 - stress
                        // 3 - strain
                        // 5 - temperature
                        // 6 - heat flux
                        // 8 - displacement
                        // 9 - reaction force
                        // 11 - velocity
                        // 12 - acceleration
                        // 13 - excitation force
                        // 15 - pressure
                        // 16 - mass
                        // 17 - time
                        // 18 - frequency
                        // 19 - rpm
                        // 20 - order
    int ydLengthUnitsExponent = 0;
    int ydForceUnitsExponent = 0;
    int ydTemperatureUnitsExponent = 0;
    std::string ydLabel = "NONE";
    std::string ydUnitsLabel = "NONE";
    //11-я строка
    int zDataType = 1;  // 0 - unknown
                        // 1 - general
                        // 2 - stress
                        // 3 - strain
                        // 5 - temperature
                        // 6 - heat flux
                        // 8 - displacement
                        // 9 - reaction force
                        // 11 - velocity
                        // 12 - acceleration
                        // 13 - excitation force
                        // 15 - pressure
                        // 16 - mass
                        // 17 - time
                        // 18 - frequency
                        // 19 - rpm
                        // 20 - order
    int zLengthUnitsExponent = 0;
    int zForceUnitsExponent = 0;
    int zTemperatureUnitsExponent = 0;
    std::string zLabel = "NONE";
    std::string zUnitsLabel = "NONE";
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

UffDescriptor::UffDescriptor(const QString &fileName) : FileDescriptor(fileName)
{

}

void UffDescriptor::read()
{
//    inline std::size_t for_each_line(std::istream& stream,
//                                     Function function,
//                                     const std::size_t& buffer_size = one_kilobyte);
//    inline std::size_t for_each_line_n(std::istream& stream,
//                                       const std::size_t& n,
//                                       Function function,
//                                       const std::size_t& buffer_size = one_kilobyte);
//    inline std::size_t for_each_line(const std::string& file_name,
//                                     Function function,
//                                     const std::size_t& buffer_size = one_kilobyte);
//    inline std::size_t for_each_line_n_conditional(std::istream& stream,
//                                                   const std::size_t& n,
//                                                   Function function,
//                                                   const std::size_t& buffer_size = one_kilobyte);
    std::ifstream stream(fileName().toStdString().c_str());

    //reading type151
//    int index = 0;
//    std::string buffer;
//    buffer.reserve(1024);

//    for (; index < 10; ++index) {
//        std::getline(stream,buffer);
//        if (index == 0 && buffer != "    -1") break;
//        if (index == 9 && buffer != "    -1") break;

//    }


//    std::size_t result = strtk::for_each_line_n(stream, 10, [=](std::string line){
//        strtk::ignore_token ignore;
//        strtk::remove_leading_trailing(" \t\n\r",line);
//        const bool result = strtk::parse(line,
//                                         " ",
//                                         ignore,
//                                         temp.symbol,
//                                         ignore,
//                                         ignore,
//                                         ignore,
//                                         ignore,
//                                         temp.total_volume);
//    });
//    qDebug()<<result;
}

void UffDescriptor::write()
{
}

void UffDescriptor::fillPreliminary(Descriptor::DataType)
{
}

void UffDescriptor::fillRest()
{
}

void UffDescriptor::writeRawFile()
{
}

void UffDescriptor::updateDateTimeGUID()
{
}

Descriptor::DataType UffDescriptor::type() const
{
}

QString UffDescriptor::typeDisplay() const
{
}

DescriptionList UffDescriptor::dataDescriptor() const
{
}

void UffDescriptor::setDataDescriptor(const DescriptionList &data)
{
}

QString UffDescriptor::dataDescriptorAsString() const
{
}

QDateTime UffDescriptor::dateTime() const
{
}

void UffDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{
}

void UffDescriptor::copyChannelsFrom(FileDescriptor *, const QVector<int> &)
{
}

void UffDescriptor::calculateMean(const QList<Channel *> &channels)
{
}

QString UffDescriptor::calculateThirdOctave()
{
}

void UffDescriptor::calculateMovingAvg(const QList<Channel *> &channels, int windowSize)
{
}

QString UffDescriptor::saveTimeSegment(double from, double to)
{
}

int UffDescriptor::channelsCount() const
{
}

void UffDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
}

QVariant UffDescriptor::channelHeader(int column) const
{
}

int UffDescriptor::columnsCount() const
{
}

Channel *UffDescriptor::channel(int index) const
{
}

QString UffDescriptor::legend() const
{
}

bool UffDescriptor::setLegend(const QString &legend)
{
}

double UffDescriptor::xStep() const
{
}

void UffDescriptor::setXStep(const double xStep)
{
}

double UffDescriptor::xBegin() const
{
}

int UffDescriptor::samplesCount() const
{
}

void UffDescriptor::setSamplesCount(int count)
{
}

QString UffDescriptor::xName() const
{
}

bool UffDescriptor::setDateTime(QDateTime dt)
{
}

bool UffDescriptor::dataTypeEquals(FileDescriptor *other) const
{
}

UffChannel::UffChannel()
{

}


QVariant UffChannel::info(int column, bool edit) const
{
}

int UffChannel::columnsCount() const
{
}

QVariant UffChannel::channelHeader(int column) const
{
}

Descriptor::DataType UffChannel::type() const
{
}

int UffChannel::octaveType() const
{
}

void UffChannel::populate()
{
}

QString UffChannel::name() const
{
}

void UffChannel::setName(const QString &name)
{
}

QString UffChannel::description() const
{
}

void UffChannel::setDescription(const QString &description)
{
}

QString UffChannel::xName() const
{
}

QString UffChannel::yName() const
{
}

QString UffChannel::zName() const
{
}

void UffChannel::setYName(const QString &yName)
{
}

QString UffChannel::legendName() const
{
}

FileDescriptor *UffChannel::descriptor()
{
}

int UffChannel::index() const
{
}

QString UffChannel::correction() const
{
}

void UffChannel::setCorrection(const QString &s)
{
}
