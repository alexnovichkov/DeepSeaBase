TEMPLATE = lib
CONFIG *= plugin
CONFIG *= warn_on
CONFIG *= debug_and_release

QT *= widgets
QT *= concurrent

INCLUDEPATH += ../..
DEPENDPATH += ../..

CONFIG(debug, debug|release){
        DESTDIR = $$OUT_PWD/../../../bin.debug/plugins
#        LIBS += -L$$OUT_PWD/../lib.debug
}
else{
        DESTDIR = $$OUT_PWD/../../../bin/plugins
#        LIBS += -L$$OUT_PWD/../lib
}
