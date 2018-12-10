# This file is a part of
# DeepSea base 

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += qtpropertybrowser
qtpropertybrowser.file = $$PWD/3rdParty/qtpropertybrowser/qtpropertybrowser.pro

SUBDIRS += alglib
alglib.file = $$PWD/3rdParty/alglib/alglib.pro

SUBDIRS += DspFilters
DspFilters.file = $$PWD/3rdParty/DspFilters/DspFilters.pro

SUBDIRS += src
src.file = $$PWD/src/src.pro
