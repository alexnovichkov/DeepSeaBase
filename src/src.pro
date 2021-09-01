#-------------------------------------------------
#
# Project created by QtCreator 2013-08-14T13:18:13
#
#-------------------------------------------------

QT += core gui
QT += widgets printsupport
QT += axcontainer multimedia

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
message($$VERSION)

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
    app.cpp \
    channelpropertiesdialog.cpp \
    channelsmimedata.cpp \
    channelstable.cpp \
    converters/tdmsconverter.cpp \
    descriptorpropertiesdialog.cpp \
    filestable.cpp \
    mainwindow.cpp \
    methods/apsfunction.cpp \
    methods/filteringalgorithm.cpp \
    methods/frfalgorithm.cpp \
    methods/frffunction.cpp \
    methods/gxyfunction.cpp \
    methods/octavealgorithm.cpp \
    methods/octavefunction.cpp \
    methods/resamplingalgorithm.cpp \
    methods/weighting.cpp \
    methods/windowingalgorithm.cpp \
    plot/canvaseventfilter.cpp \
    plot/imagerenderdialog.cpp \
    plot/trackingcursor.cpp \
    plot/zoomstack.cpp \
    sortabletreewidgetitem.cpp \
    checkableheaderview.cpp \
    methods/dfdmethods/spectremethod.cpp \
    methods/dfdmethods/timemethod.cpp \
    curvepropertiesdialog.cpp \
    tabwidget.cpp \
    coloreditdialog.cpp \
    colorselector.cpp \
    correctiondialog.cpp \
    methods/dfdmethods/xresponch1.cpp \
    logging.cpp \
    editdescriptionsdialog.cpp \
    methods/windowing.cpp \
    trackingpanel.cpp \
    methods/dfdmethods/octavemethod.cpp \
    algorithms.cpp \
    methods/octavefilterbank.cpp \
    axisboundsdialog.cpp \
    calculatespectredialog.cpp \
    methods/averaging.cpp \
    timeslicer.cpp \
    methods/dfdmethods/abstractmethod.cpp \
    dataholder.cpp \
    methods/fft.cpp \
    taskbarprogress.cpp \
    model.cpp \
    sortfiltermodel.cpp \
    filterheaderview.cpp \
    methods/channelselector.cpp \
    methods/filtering.cpp \
    filesprocessordialog.cpp \
    methods/abstractfunction.cpp \
    methods/channelfunction.cpp \
    methods/resamplingfunction.cpp \
    methods/filteringfunction.cpp \
    methods/averagingfunction.cpp \
    methods/windowingfunction.cpp \
    methods/fftfunction.cpp \
    methods/psfunction.cpp \
    methods/psdfunction.cpp \
    methods/framecutterfunction.cpp \
    methods/framecutter.cpp \
    methods/savingfunction.cpp \
    methods/spectrealgorithm.cpp \
    methods/psalgorithm.cpp \
    methods/psdalgorithm.cpp \
    unitsconverter.cpp \
    dataiodevice.cpp \
    playpanel.cpp \
    channeltablemodel.cpp \
    headerview.cpp \
    htmldelegate.cpp \
    plot/curve.cpp \
    plot/linecurve.cpp \
    plot/barcurve.cpp \
    plot/plot.cpp \
    plot/legend.cpp \
    plot/logscaleengine.cpp \
    plot/pointlabel.cpp \
    plot/plotzoom.cpp \
    plot/axiszoom.cpp \
    plot/dragzoom.cpp \
    plot/wheelzoom.cpp \
    plot/picker.cpp \
    plot/pointmarker.cpp \
    plot/plottracker.cpp \
    wavexportdialog.cpp \
    wavexporter.cpp \
    plot/spectrocurve.cpp \
    plot/colormapfactory.cpp \
    fancylineedit.cpp

HEADERS  += mainwindow.h \
    app.h \
    channelpropertiesdialog.h \
    channelsmimedata.h \
    channelstable.h \
    converters/tdmsconverter.h \
    descriptorpropertiesdialog.h \
    enums.h \
    filestable.h \
    methods/apsfunction.h \
    methods/filteringalgorithm.h \
    methods/frfalgorithm.h \
    methods/frffunction.h \
    methods/gxyfunction.h \
    methods/octavealgorithm.h \
    methods/octavefunction.h \
    methods/resamplingalgorithm.h \
    methods/weighting.h \
    methods/windowingalgorithm.h \
    plot/canvaseventfilter.h \
    plot/imagerenderdialog.h \
    plot/trackingcursor.h \
    plot/zoomstack.h \
    sortabletreewidgetitem.h \
    checkableheaderview.h \
    methods/dfdmethods/abstractmethod.h \
    methods/dfdmethods/spectremethod.h \
    methods/dfdmethods/timemethod.h \
    curvepropertiesdialog.h \
    tabwidget.h \
    coloreditdialog.h \
    colorselector.h \
    correctiondialog.h \
    methods/dfdmethods/xresponch1.h \
    logging.h \
    editdescriptionsdialog.h \
    methods/windowing.h \
    trackingpanel.h \
    methods/dfdmethods/octavemethod.h \
    algorithms.h \
    methods/octavefilterbank.h \
    axisboundsdialog.h \
    calculatespectredialog.h \
    methods/averaging.h \
    timeslicer.h \
    dataholder.h \
    methods/resampler.h \
    methods/fft.h \
    taskbarprogress.h \
    model.h \
    sortfiltermodel.h \
    filterheaderview.h \
    methods/channelselector.h \
    methods/filtering.h \
    filesprocessordialog.h \
    methods/abstractfunction.h \
    methods/channelfunction.h \
    methods/resamplingfunction.h \
    methods/filteringfunction.h \
    methods/averagingfunction.h \
    methods/windowingfunction.h \
    methods/fftfunction.h \
    methods/psfunction.h \
    methods/psdfunction.h \
    methods/framecutterfunction.h \
    methods/framecutter.h \
    methods/savingfunction.h \
    methods/spectrealgorithm.h \
    methods/psalgorithm.h \
    methods/psdalgorithm.h \
    unitsconverter.h \
    dataiodevice.h \
    playpanel.h \
    channeltablemodel.h \
    headerview.h \
    htmldelegate.h \
    plot/plot.h \
    plot/legend.h \
    plot/curve.h \
    plot/linecurve.h \
    plot/barcurve.h \
    plot/logscaleengine.h \
    plot/pointlabel.h \
    plot/plotzoom.h \
    plot/axiszoom.h \
    plot/dragzoom.h \
    plot/wheelzoom.h \
    plot/picker.h \
    plot/pointmarker.h \
    plot/plottracker.h \
    wavexportdialog.h \
    wavexporter.h \
    longoperation.h \
    plot/spectrocurve.h \
    plot/colormapfactory.h \
    fancylineedit.h


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

CONFIG(release, debug|release):{
  QMAKE_CFLAGS  += -O2
}

# includes & libs
INCLUDEPATH *= $$PWD $$PWD/.. $$PWD/../3rdParty/qtpropertybrowser $$PWD/../3rdParty/DspFilters
INCLUDEPATH *= E:/My/programming/sources/strtk
INCLUDEPATH *= E:/My/programming/sources/boost_1_73_0

#PRECOMPILED_HEADER = strtk_.hpp

#libsamplerate
INCLUDEPATH *= E:/My/programming/sources/libsamplerate-0.1.8/src
LIBS *= E:/My/programming/sources/build-libsamplerate-0.1.8-Desktop_Qt_5_12_8_MinGW_32_bit-Release/release/libsamplerate.a
LIBS *= E:/My/programming/sources/build-libsamplerate-0.1.8-Desktop_Qt_5_12_8_MinGW_64_bit-Release/release/libsamplerate.a

#tdm
INCLUDEPATH *= E:/My/programming/sources/TDMS/tdm_dev/dev/include
LIBS *= E:/My/programming/sources/TDMS/tdm_dev/dev/lib/64-bit/msvc64/nilibddc.lib
LIBS *= E:/My/programming/sources/TDMS/tdm_dev/dev/lib/32-bit/msvc/nilibddc.lib

#FFTW
INCLUDEPATH *= E:/My/programming/sources/fftw-3.3.5-dll32
LIBS *= E:/My/programming/sources/fftw-3.3.5-dll32/libfftw3-3.lib
LIBS *= E:/My/programming/sources/fftw-3.3.5-dll64/libfftw3-3.lib

#qwt
CONFIG(release, debug|release):{
    LIBS *= E:/My/programming/sources/Qwt-6.4.0-svn/lib/libqwt.a
    LIBS *= E:/My/programming/sources/Qwt-6.4.0-svn/lib64/libqwt.a
}
CONFIG(debug, debug|release):{
    LIBS *= E:/My/programming/sources/Qwt-6.4.0-svn/lib/libqwtd.a
    LIBS *= E:/My/programming/sources/Qwt-6.4.0-svn/lib64/libqwtd.a
}
INCLUDEPATH *= E:/My/programming/sources/Qwt-6.4.0-svn/include

##matio
#INCLUDEPATH *= K:/My/programming/sources/Matlab/matio-1.5.17/src
#LIBS *= K:/My/programming/sources/Matlab/matio-1.5.17/visual_studio/Release/libmatio.lib

#DEFINES += APP_PORTABLE

