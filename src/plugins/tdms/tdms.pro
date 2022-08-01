include(../plugins.pri)

PVERSION = 1.0.0
DEFINES += PLUGIN_VERSION=\\\"$$PVERSION\\\"

HEADERS = tdmsplugin.h \
    ../convertplugin.h
SOURCES = tdmsplugin.cpp
OTHER_FILES += tdms.json

CONFIG *= c++14 c++17

HEADERS += tdmsconverter.h \
           tdmsconverterdialog.h \
           ../../fileformats/formatfactory.h \
           ../../checkableheaderview.h \
           ../../logging.h \
           ../../algorithms.h \
           tdmsfile.h \
           ../../fileformats/filedescriptor.h \
           ../../dataholder.h \
           ../../settings.h \
           ../../methods/averaging.h

SOURCES += tdmsconverter.cpp \
           tdmsconverterdialog.cpp \
           tdmsfile.cpp \
           ../../checkableheaderview.cpp \
           ../../logging.cpp \
           ../../algorithms.cpp \
           ../../fileformats/formatfactory.cpp \
           ../../fileformats/filedescriptor.cpp \
           ../../dataholder.cpp \
           ../../settings.cpp \
           ../../methods/averaging.cpp

HEADERS += ../../fileformats/dfdfiledescriptor.h \
           ../../fileformats/ufffile.h \
           ../../fileformats/data94file.h \
           ../../fileformats/dfdsettings.h \
           ../../fileformats/fields.h \
           ../../fileformats/uffheaders.h \
           ../../methods/octavefilterbank.h

SOURCES += ../../fileformats/dfdfiledescriptor.cpp \
           ../../fileformats/ufffile.cpp \
           ../../fileformats/data94file.cpp \
           ../../fileformats/dfdsettings.cpp \
           ../../fileformats/fields.cpp \
           ../../methods/octavefilterbank.cpp

TARGET = tdms

message(-- Searching for tdms --)
message(tdms include path is E:/My/programming/sources/TDMS/tdm_dev/dev/include)
INCLUDEPATH *= E:/My/programming/sources/TDMS/tdm_dev/dev/include
equals(QT_ARCH,"x86_64") {
  LIBS *= E:/My/programming/sources/TDMS/tdm_dev/dev/lib/64-bit/msvc64/nilibddc.lib
}
equals(QT_ARCH,"i386") {
  LIBS *= E:/My/programming/sources/TDMS/tdm_dev/dev/lib/32-bit/msvc/nilibddc.lib
}

message(-- Searching for qwt --)
INCLUDEPATH *= C:/Qwt-6.2.0-dev/x32/include
message(qwt include path is C:/Qwt-6.2.0-dev/x32/include)
CONFIG(release, debug|release):{
  equals(QT_ARCH,"i386") {
    LIBS *= C:/Qwt-6.2.0-dev/x32/lib/libqwt.a
  }
  equals(QT_ARCH,"x86_64") {
    LIBS *= C:/Qwt-6.2.0-dev/x64/lib/libqwt.a
  }
}
CONFIG(debug, debug|release):{
  equals(QT_ARCH,"i386") {
    LIBS *= C:/Qwt-6.2.0-dev/x32/lib/libqwtd.a
  }
  equals(QT_ARCH,"x86_64") {
    LIBS *= C:/Qwt-6.2.0-dev/x64/lib/libqwtd.a
  }
}

INCLUDEPATH *= E:/My/programming/sources/boost_1_73_0
INCLUDEPATH *= E:/My/programming/sources/strtk
#include(../../3rdparty/libz.pri)
