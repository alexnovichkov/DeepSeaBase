include(../plugins.pri)

PVERSION = 1.0.0
DEFINES += PLUGIN_VERSION=\\\"$$PVERSION\\\"

OTHER_FILES += tdms.json

CONFIG *= c++14 c++17

HEADERS += tdmsplugin.h \
           tdmsconverter.h \
           tdmsconverterdialog.h \
           tdmsfile.h \
           ../convertplugin.h \
           ../../fileformats/abstractformatfactory.h \
           ../../checkableheaderview.h \
           ../../logging.h \
           ../../fileformats/filedescriptor.h \
           ../../dataholder.h \
           ../../settings.h \
           ../../algorithms.h

SOURCES += tdmsplugin.cpp \
           tdmsconverter.cpp \
           tdmsconverterdialog.cpp \
           tdmsfile.cpp \
           ../../checkableheaderview.cpp \
           ../../logging.cpp \
           ../../fileformats/filedescriptor.cpp \
           ../../dataholder.cpp \
           ../../settings.cpp \
           ../../algorithms.cpp

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


INCLUDEPATH *= E:/My/programming/sources/boost_1_73_0

#include(../../3rdparty/libz.pri)
