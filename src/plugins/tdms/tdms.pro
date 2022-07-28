include(../plugins.pri)

PVERSION = 1.0.0
DEFINES += PLUGIN_VERSION=\\\"$$PVERSION\\\"

HEADERS = tdmsplugin.h \
    ../convertplugin.h
SOURCES = tdmsplugin.cpp
OTHER_FILES += tdms.json

HEADERS +=

#HEADERS += ../../qoobar_app/tagger.h \
#           ../../qoobar_app/iqoobarplugin.h \
#           ../../qoobar_app/corenetworksearch.h \
#           ../../qoobar_app/checkabletablemodel.h \
#           ../../qoobar_app/tagparser.h \
#           ../../qoobar_app/placeholders.h

#SOURCES += ../../qoobar_app/tagger.cpp \
#           ../../qoobar_app/coverimage.cpp \
#           ../../qoobar_app/corenetworksearch.cpp \
#           ../../qoobar_app/checkabletablemodel.cpp \
#           ../../qoobar_app/tagparser.cpp \
#           ../../qoobar_app/placeholders.cpp

TARGET = tdms

#include(../../3rdparty/libz.pri)
