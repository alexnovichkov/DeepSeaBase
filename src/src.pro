#-------------------------------------------------
#
# Project created by QtCreator 2013-08-14T13:18:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport axcontainer multimedia

TARGET = DeepSeaBase
TEMPLATE = app

CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../bin.debug
        LIBS += -L$$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../bin
        LIBS += -L$$OUT_PWD/../lib
}

CONFIG += c++17

qtHaveModule(winextras) {
    QT *= winextras
}

SOURCES += main.cpp\
    mainwindow.cpp \
    dfdfiledescriptor.cpp \
    sortabletreewidgetitem.cpp \
    plot.cpp \
    curve.cpp \
    checkableheaderview.cpp \
    methods/spectremethod.cpp \
    methods/timemethod.cpp \
    graphpropertiesdialog.cpp \
    tabwidget.cpp \
    coloreditdialog.cpp \
    colorselector.cpp \
    plotpicker.cpp \
    correctiondialog.cpp \
    methods/xresponch1.cpp \
    pointlabel.cpp \
    legend.cpp \
    legendlabel.cpp \
    logging.cpp \
    filedescriptor.cpp \
    ufffile.cpp \
    editdescriptionsdialog.cpp \
    iirfilter.cpp \
    windowing.cpp \
    converter.cpp \
    matlabfiledescriptor.cpp \
    matlabconverterdialog.cpp \
    esoconverterdialog.cpp \
    uffconverterdialog.cpp \
    trackingpanel.cpp \
    methods/octavemethod.cpp \
    algorithms.cpp \
    dfdsettings.cpp \
    octavefilterbank.cpp \
    axisboundsdialog.cpp \
    qmainzoomsvc.cpp \
    chartzoom.cpp \
    qdragzoomsvc.cpp \
    spectre94.cpp \
    calculatespectredialog.cpp \
    averaging.cpp \
    timeslicer.cpp \
    fields.cpp \
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
    qwheelzoomsvc.cpp \
    qaxiszoomsvc.cpp \
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
    dataiodevice.cpp

HEADERS  += mainwindow.h \
    dfdfiledescriptor.h \
    sortabletreewidgetitem.h \
    plot.h \
    curve.h \
    checkableheaderview.h \
    methods/abstractmethod.h \
    methods/spectremethod.h \
    methods/timemethod.h \
    graphpropertiesdialog.h \
    tabwidget.h \
    coloreditdialog.h \
    colorselector.h \
    plotpicker.h \
    correctiondialog.h \
    methods/xresponch1.h \
    pointlabel.h \
    legend.h \
    legendlabel.h \
    logging.h \
    filedescriptor.h \
    ufffile.h \
    fields.h \
    editdescriptionsdialog.h \
    iirfilter.h \
    windowing.h \
    converter.h \
    matlabfiledescriptor.h \
    matlabconverterdialog.h \
    esoconverterdialog.h \
    uffconverterdialog.h \
    trackingpanel.h \
    uffheaders.h \
    methods/octavemethod.h \
    algorithms.h \
    dfdsettings.h \
    octavefilterbank.h \
    axisboundsdialog.h \
    qmainzoomsvc.h \
    chartzoom.h \
    qdragzoomsvc.h \
    psimpl.h \
    spectre94.h \
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
    qwheelzoomsvc.h \
    qaxiszoomsvc.h \
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
    dataiodevice.h

SOURCES +=\
  $$PWD/../3rdParty/DspFilters/*.cpp \
  $$PWD/../3rdParty/alglib/*.cpp \
  $$PWD/../3rdParty/qtpropertybrowser/*.cpp \

HEADERS +=\
  $$PWD/../3rdParty/DspFilters/*.h \
  $$PWD/../3rdParty/alglib/*.h \
  $$PWD/../3rdParty/qtpropertybrowser/*.h \

RESOURCES *= src.qrc

RC_FILE *= src.rc

CONFIG(release, debug|release):LIBS *= C:/Qwt-6.1.2/lib/libqwt.a
CONFIG(debug, debug|release):  LIBS *= C:/Qwt-6.1.2/lib/libqwtd.a

INCLUDEPATH *= K:/My/programming/sources/libsamplerate-0.1.8/src

# includes & libs
INCLUDEPATH *= $$PWD $$PWD/.. $$PWD/../3rdParty/qtpropertybrowser $$PWD/../3rdParty/alglib $$PWD/../3rdParty/DspFilters
INCLUDEPATH *= C:/Qwt-6.1.2/include

LIBS *= K:/My/programming/sources/libsamplerate-0.1.8/release/libsamplerate.a


