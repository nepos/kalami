QT += core websockets dbus
QT -= gui

CONFIG += c++11

TARGET = hal-qt
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    daemon.cpp \
    reduxproxy.cpp \
    inputdevice.cpp \
    udevdevice.cpp \
    udevmonitor.cpp \
    connman.cpp \
    linuxled.cpp \
    alsamixer.cpp

HEADERS += \
    daemon.h \
    reduxproxy.h \
    inputdevice.h \
    udevdevice.h \
    udevmonitor.h \
    types.h \
    connman.h \
    linuxled.h \
    alsamixer.h

LIBS += -ludev
LIBS += -lqconnman
LIBS += -lasound

DISTFILES += \
    io.nepos.connman.Agent
