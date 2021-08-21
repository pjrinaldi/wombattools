QT += core
QT -= gui network
CONFIG += debug_and_release debug_and_release_target qt build_all console
DEFINES += _FILE_OFFSET_BITS=64
INCLUDEPATH += /usr/local/lib/
INCLUDEPATH += /usr/local/include/
INCLUDEPATH += usr/include/fuse3/
VPATH += /usr/local/lib/
VPATH += /usr/local/include/
VPATH += /usr/include/fuse3/
SOURCES = wombatfuse.cpp
release: DESTDIR = release
debug: DESTDIR = debug
LIBS += -llz4 `pkg-config fuse3 --cflags --libs`
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
}
target.path = /usr/local/bin
INSTALLS += target
