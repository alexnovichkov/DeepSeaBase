TEMPLATE=lib
CONFIG+=dll

win32-msvc*{
  QMAKE_CXXFLAGS += /MP
}
