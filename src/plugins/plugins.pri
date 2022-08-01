TEMPLATE = lib
CONFIG *= plugin
CONFIG *= warn_on
CONFIG *= debug_and_release

QT *= widgets
QT *= concurrent

INCLUDEPATH += ../..
DEPENDPATH += ../..

DESTDIR = $${BUILD_DIR}/plugins
