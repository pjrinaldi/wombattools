QT += core
QT -= gui network
CONFIG += debug_and_release debug_and_release_target qt build_all console
#INCLUDEPATH += /usr/local/lib/
#INCLUDEPATH += /usr/local/include/
#VPATH += /usr/local/lib/
#VPATH += /usr/local/include/
HEADERS = ../blake3.h
SOURCES = wombatimager.cpp
release: DESTDIR = release
debug: DESTDIR = debug
LIBS += -ludev -llz4 ../libblake3.a
#LIBS += -lblake3 -ludev -llz4 #-lzstd -lsnappy
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
}
target.path = /usr/local/bin
INSTALLS += target
