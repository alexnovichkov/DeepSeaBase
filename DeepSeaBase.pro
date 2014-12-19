#-------------------------------------------------
#
# Project created by QtCreator 2013-08-14T13:18:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport axcontainer

TARGET = DeepSeaBase
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
    mainwindow.cpp \
    dfdfiledescriptor.cpp \
    convertdialog.cpp \
    sortabletreewidgetitem.cpp \
    plot.cpp \
    curve.cpp \
    checkableheaderview.cpp \
    methods/spectremethod.cpp \
    methods/timemethod.cpp \
    graphpropertiesdialog.cpp \
    tabwidget.cpp \
    converters.cpp \
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
    windowing.cpp

HEADERS  += mainwindow.h \
    dfdfiledescriptor.h \
    convertdialog.h \
    sortabletreewidgetitem.h \
    plot.h \
    curve.h \
    checkableheaderview.h \
    methods/abstractmethod.h \
    methods/spectremethod.h \
    methods/timemethod.h \
    graphpropertiesdialog.h \
    tabwidget.h \
    converters.h \
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
    windowing.h

SOURCES += qwtchartzoom.cpp\
  qwheelzoomsvc.cpp\
  qaxiszoomsvc.cpp

HEADERS +=qwtchartzoom.h\
  qwheelzoomsvc.h\
  qaxiszoomsvc.h

RESOURCES *= DeepSeaBase.qrc

RC_FILE *= DeepSeaBase.rc

CONFIG(release, debug|release):LIBS *= C:/Qwt-6.1.0/lib/libqwt.a
CONFIG(debug, debug|release):  LIBS *= C:/Qwt-6.1.0/lib/libqwtd.a

INCLUDEPATH *= K:/My/programming/sources/libsamplerate-0.1.8/src
LIBS *= K:/My/programming/sources/libsamplerate-0.1.8/release/libsamplerate.a

INCLUDEPATH *= C:/Qwt-6.1.0/include

