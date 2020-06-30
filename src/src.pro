#-------------------------------------------------
#
# Project created by QtCreator 2013-08-14T13:18:13
#
#-------------------------------------------------

QT += core gui
QT += widgets printsupport axcontainer multimedia

TARGET = DeepSeaBase
TEMPLATE = app

VERSIONJS = $$cat(version.js, lines)
VERSIONJS = $$first(VERSIONJS)
VERSION = $$section(VERSIONJS, \",1,1)
RC_ICONS = icon.ico
#RC_CODEPAGE =
QMAKE_TARGET_COMPANY=Novichkov &
QMAKE_TARGET_COPYRIGHT=Все права принадлежат мне
QMAKE_TARGET_PRODUCT=DeepSeaBase

DEFINES *= DEEPSEABASE_VERSION=\\\"$$VERSION\\\"

CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../bin.debug
        LIBS += -L$$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../bin
        LIBS += -L$$OUT_PWD/../lib
}

CONFIG += c++14 c++17

qtHaveModule(winextras) {
    QT *= winextras
}

SOURCES += main.cpp\
    channelsmimedata.cpp \
    converters/tdmsconverter.cpp \
    filestable.cpp \
    mainwindow.cpp \
    methods/frfalgorithm.cpp \
    methods/frffunction.cpp \
    methods/windowingalgorithm.cpp \
    sortabletreewidgetitem.cpp \
    checkableheaderview.cpp \
    methods/spectremethod.cpp \
    methods/timemethod.cpp \
    curvepropertiesdialog.cpp \
    tabwidget.cpp \
    coloreditdialog.cpp \
    colorselector.cpp \
    correctiondialog.cpp \
    methods/xresponch1.cpp \
    logging.cpp \
    editdescriptionsdialog.cpp \
    iirfilter.cpp \
    methods/windowing.cpp \
    trackingpanel.cpp \
    methods/octavemethod.cpp \
    algorithms.cpp \
    methods/octavefilterbank.cpp \
    axisboundsdialog.cpp \
    calculatespectredialog.cpp \
    averaging.cpp \
    timeslicer.cpp \
    methods/abstractmethod.cpp \
    dataholder.cpp \
    fft.cpp \
    taskbarprogress.cpp \
    model.cpp \
    sortfiltermodel.cpp \
    filterheaderview.cpp \
    channelselector.cpp \
    filtering.cpp \
    filesprocessordialog.cpp \
    methods/abstractfunction.cpp \
    methods/channelfunction.cpp \
    methods/resamplingfunction.cpp \
    methods/filteringfunction.cpp \
    methods/averagingfunction.cpp \
    methods/windowingfunction.cpp \
    methods/fftfunction.cpp \
    methods/framecutterfunction.cpp \
    framecutter.cpp \
    methods/savingfunction.cpp \
    methods/spectrealgorithm.cpp \
    unitsconverter.cpp \
    dataiodevice.cpp \
    playpanel.cpp \
    channeltablemodel.cpp \
    headerview.cpp \
    methods/timealgorithm.cpp \
    htmldelegate.cpp \
    plot/curve.cpp \
    plot/linecurve.cpp \
    plot/barcurve.cpp \
    plot/plot.cpp \
    plot/legend.cpp \
    plot/logscaleengine.cpp \
    plot/pointlabel.cpp \
    plot/chartzoom.cpp \
    plot/plotzoom.cpp \
    plot/axiszoom.cpp \
    plot/dragzoom.cpp \
    plot/wheelzoom.cpp \
    plot/picker.cpp \
    plot/pointmarker.cpp \
    plot/plottracker.cpp \
    wavexporter.cpp \
    plot/spectrocurve.cpp \
    plot/colormapfactory.cpp

HEADERS  += mainwindow.h \
    channelsmimedata.h \
    converters/tdmsconverter.h \
    filestable.h \
    methods/frfalgorithm.h \
    methods/frffunction.h \
    methods/windowingalgorithm.h \
    sortabletreewidgetitem.h \
    checkableheaderview.h \
    methods/abstractmethod.h \
    methods/spectremethod.h \
    methods/timemethod.h \
    curvepropertiesdialog.h \
    tabwidget.h \
    coloreditdialog.h \
    colorselector.h \
    correctiondialog.h \
    methods/xresponch1.h \
    logging.h \
    editdescriptionsdialog.h \
    iirfilter.h \
    methods/windowing.h \
    trackingpanel.h \
    methods/octavemethod.h \
    algorithms.h \
    methods/octavefilterbank.h \
    axisboundsdialog.h \
    calculatespectredialog.h \
    averaging.h \
    timeslicer.h \
    dataholder.h \
    resampler.h \
    fft.h \
    taskbarprogress.h \
    model.h \
    sortfiltermodel.h \
    filterheaderview.h \
    channelselector.h \
    filtering.h \
    filesprocessordialog.h \
    methods/abstractfunction.h \
    methods/channelfunction.h \
    methods/resamplingfunction.h \
    methods/filteringfunction.h \
    methods/averagingfunction.h \
    methods/windowingfunction.h \
    methods/fftfunction.h \
    methods/framecutterfunction.h \
    framecutter.h \
    methods/savingfunction.h \
    methods/spectrealgorithm.h \
    unitsconverter.h \
    dataiodevice.h \
    playpanel.h \
    channeltablemodel.h \
    headerview.h \
    methods/timealgorithm.h \
    htmldelegate.h \
    plot/plot.h \
    plot/legend.h \
    plot/curve.h \
    plot/linecurve.h \
    plot/barcurve.h \
    plot/logscaleengine.h \
    plot/pointlabel.h \
    plot/chartzoom.h \
    plot/plotzoom.h \
    plot/axiszoom.h \
    plot/dragzoom.h \
    plot/wheelzoom.h \
    plot/picker.h \
    plot/pointmarker.h \
    plot/plottracker.h \
    wavexporter.h \
    longoperation.h \
    plot/spectrocurve.h \
    plot/colormapfactory.h


SOURCES +=\
    fileformats/dfdfiledescriptor.cpp \
    fileformats/filedescriptor.cpp \
    fileformats/ufffile.cpp \
    fileformats/dfdsettings.cpp \
    fileformats/fields.cpp \
    fileformats/data94file.cpp \
    fileformats/tdmsfile.cpp \
    fileformats/matfile.cpp

HEADERS +=\
    fileformats/formatfactory.h \
    fileformats/data94file.h \
    fileformats/tdmsfile.h \
    fileformats/matfile.h \
    fileformats/filedescriptor.h \
    fileformats/ufffile.h \
    fileformats/fields.h \
    fileformats/dfdfiledescriptor.h \
    fileformats/uffheaders.h \
    fileformats/dfdsettings.h

SOURCES +=\
    converters/matlabconvertor.cpp \
    converters/converter.cpp \
    converters/matlabconverterdialog.cpp \
    converters/esoconverterdialog.cpp \
    converters/tdmsconverterdialog.cpp \
    converters/converterdialog.cpp

HEADERS +=\
    converters/matlabconvertor.h \
    converters/converterdialog.h \
    converters/converter.h \
    converters/matlabconverterdialog.h \
    converters/esoconverterdialog.h \
    converters/tdmsconverterdialog.h

SOURCES +=\
  ../3rdParty/DspFilters/State.cpp \
  ../3rdParty/DspFilters/Bessel.cpp \
  ../3rdParty/DspFilters/Biquad.cpp \
  ../3rdParty/DspFilters/Butterworth.cpp \
  ../3rdParty/DspFilters/Cascade.cpp \
  ../3rdParty/DspFilters/ChebyshevI.cpp \
  ../3rdParty/DspFilters/ChebyshevII.cpp \
  ../3rdParty/DspFilters/Custom.cpp \
  ../3rdParty/DspFilters/Design.cpp \
  ../3rdParty/DspFilters/Elliptic.cpp \
  ../3rdParty/DspFilters/Filter.cpp \
  ../3rdParty/DspFilters/Legendre.cpp \
  ../3rdParty/DspFilters/Param.cpp \
  ../3rdParty/DspFilters/PoleFilter.cpp \
  ../3rdParty/DspFilters/RBJ.cpp \
  ../3rdParty/DspFilters/RootFinder.cpp


SOURCES +=\
  ../3rdParty/qtpropertybrowser/qtbuttonpropertybrowser.cpp \
  ../3rdParty/qtpropertybrowser/qteditorfactory.cpp \
  ../3rdParty/qtpropertybrowser/qtgroupboxpropertybrowser.cpp \
  ../3rdParty/qtpropertybrowser/qtpropertybrowser.cpp \
  ../3rdParty/qtpropertybrowser/qtpropertymanager.cpp \
  ../3rdParty/qtpropertybrowser/qtpropertybrowserutils.cpp \
  ../3rdParty/qtpropertybrowser/qttreepropertybrowser.cpp \
  ../3rdParty/qtpropertybrowser/qtvariantproperty.cpp


HEADERS +=\
  $$PWD/../3rdParty/DspFilters/Bessel.h\
  $$PWD/../3rdParty/DspFilters/Biquad.h\
  $$PWD/../3rdParty/DspFilters/Butterworth.h\
  $$PWD/../3rdParty/DspFilters/Cascade.h\
  $$PWD/../3rdParty/DspFilters/ChebyshevI.h\
  $$PWD/../3rdParty/DspFilters/ChebyshevII.h\
  $$PWD/../3rdParty/DspFilters/Common.h\
  $$PWD/../3rdParty/DspFilters/Custom.h\
  $$PWD/../3rdParty/DspFilters/Design.h\
  $$PWD/../3rdParty/DspFilters/Dsp.h\
  $$PWD/../3rdParty/DspFilters/Elliptic.h\
  $$PWD/../3rdParty/DspFilters/Filter.h\
  $$PWD/../3rdParty/DspFilters/Layout.h\
  $$PWD/../3rdParty/DspFilters/Legendre.h\
  $$PWD/../3rdParty/DspFilters/MathSupplement.h\
  $$PWD/../3rdParty/DspFilters/Params.h\
  $$PWD/../3rdParty/DspFilters/PoleFilter.h\
  $$PWD/../3rdParty/DspFilters/RBJ.h\
  $$PWD/../3rdParty/DspFilters/RootFinder.h\
  $$PWD/../3rdParty/DspFilters/SmoothedFilter.h\
  $$PWD/../3rdParty/DspFilters/State.h\
  $$PWD/../3rdParty/DspFilters/Types.h\
  $$PWD/../3rdParty/DspFilters/Utilities.h

HEADERS +=\
  $$PWD/../3rdParty/qtpropertybrowser/qtbuttonpropertybrowser.h\
  $$PWD/../3rdParty/qtpropertybrowser/qteditorfactory.h\
  $$PWD/../3rdParty/qtpropertybrowser/qtgroupboxpropertybrowser.h\
  $$PWD/../3rdParty/qtpropertybrowser/qtpropertybrowser.h\
  $$PWD/../3rdParty/qtpropertybrowser/qtpropertybrowserutils_p.h\
  $$PWD/../3rdParty/qtpropertybrowser/qtpropertymanager.h\
  $$PWD/../3rdParty/qtpropertybrowser/qttreepropertybrowser.h\
  $$PWD/../3rdParty/qtpropertybrowser/qtvariantproperty.h


RESOURCES *= src.qrc

#RC_FILE *= src.rc

CONFIG(release, debug|release):QMAKE_CFLAGS  += -O2

# includes & libs
INCLUDEPATH *= $$PWD $$PWD/.. $$PWD/../3rdParty/qtpropertybrowser $$PWD/../3rdParty/DspFilters
INCLUDEPATH *= K:/My/programming/sources/strtk

#PRECOMPILED_HEADER = strtk_.hpp

#libsamplerate
INCLUDEPATH *= K:/My/programming/sources/libsamplerate-0.1.8/src
LIBS *= K:/My/programming/sources/build-libsamplerate-0.1.8-Desktop_Qt_5_12_8_MinGW_32_bit-Release/release/libsamplerate.a
LIBS *= K:/My/programming/sources/build-libsamplerate-0.1.8-Desktop_Qt_5_12_8_MinGW_64_bit-Release/release/libsamplerate.a

#tdm
INCLUDEPATH *= K:/My/programming/sources/TDMS/tdm_dev/dev/include
LIBS *= K:/My/programming/sources/TDMS/tdm_dev/dev/lib/64-bit/msvc64/nilibddc.lib
LIBS *= K:/My/programming/sources/TDMS/tdm_dev/dev/lib/32-bit/msvc/nilibddc.lib

#FFTW
INCLUDEPATH *= K:/My/programming/sources/fftw-3.3.5-dll32
LIBS *= K:/My/programming/sources/fftw-3.3.5-dll32/libfftw3-3.lib
LIBS *= K:/My/programming/sources/fftw-3.3.5-dll64/libfftw3-3.lib

#qwt
CONFIG(release, debug|release):{
    LIBS *= C:/Qwt-6.4.0-svn/lib/libqwt.a
    LIBS *= C:/Qwt-6.4.0-svn/lib64/libqwt.a
}
CONFIG(debug, debug|release):{
    LIBS *= C:/Qwt-6.4.0-svn/lib/libqwtd.a
    LIBS *= C:/Qwt-6.4.0-svn/lib64/libqwtd.a
}
INCLUDEPATH *= C:/Qwt-6.4.0-svn/include

##matio
#INCLUDEPATH *= K:/My/programming/sources/Matlab/matio-1.5.17/src
#LIBS *= K:/My/programming/sources/Matlab/matio-1.5.17/visual_studio/Release/libmatio.lib

#DEFINES += APP_PORTABLE

